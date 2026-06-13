#include "importers/sff1/sff1_reader.h"
#include <fstream>
#include <cstring>
#include <algorithm>

namespace ai_arranger::importers::sff1 {

Sff1Reader::Sff1Reader() = default;

ParseResult Sff1Reader::parseFile(const std::string& path) noexcept {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        return {false, "Cannot open file: " + path, path};
    }

    size_t fileSize = file.tellg();
    if (fileSize == 0 || fileSize > 10 * 1024 * 1024) {
        return {false, "File size out of range: " + std::to_string(fileSize), path};
    }

    std::vector<uint8_t> buffer(fileSize);
    file.seekg(0);
    file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
    file.close();

    return parseBuffer(buffer, path);
}

ParseResult Sff1Reader::parseBuffer(const std::vector<uint8_t>& buffer,
                                     const std::string& filename) noexcept {
    data_ = buffer.data();
    size_ = buffer.size();
    pos_ = 0;
    result_ = ParseResult{};
    result_.success = false;
    result_.file_path = filename;

    if (size_ < 4) {
        result_.error = "File too small to be SFF1";
        return result_;
    }

    // ── Read all top-level chunks ──────────────────────────────────
    while (pos_ < size_) {
        if (size_ - pos_ < 8) {
            result_.warnings.push_back("Trailing bytes at end of file (" +
                                       std::to_string(size_ - pos_) + " bytes)");
            break;
        }

        SffChunk chunk = readChunk();
        if (!chunk.chunk_id.empty()) {
            result_.chunks.push_back(chunk);

            if (!isKnownChunk(chunk.chunk_id)) {
                result_.unknown_chunks++;
            }

            // Try to parse known chunk types
            if (chunk.chunk_id == "SInt" || chunk.chunk_id == "Sect") {
                auto section = parseSection(chunk);
                result_.sections.push_back(std::move(section));
                result_.parsed_sections++;
            }
        } else {
            // End of readable chunks
            break;
        }
    }

    // ── Build unsupported feature list ─────────────────────────────
    if (result_.unknown_chunks > 0) {
        result_.unsupported_features.push_back(
            "Unknown chunks: " + std::to_string(result_.unknown_chunks));
    }

    // Count total events
    for (const auto& section : result_.sections) {
        for (const auto& track : section.tracks) {
            result_.parsed_events += track.events.size();
        }
    }

    result_.total_chunks = result_.chunks.size();
    result_.success = result_.total_chunks > 0;

    if (!result_.success) {
        result_.error = "No recognizable SFF1 structure found";
    }

    return result_;
}

// ── Chunk Reading ─────────────────────────────────────────────────

SffChunk Sff1Reader::readChunk() noexcept {
    SffChunk chunk;

    // Try to read a 4-byte chunk ID
    if (!ensure(4)) return chunk;
    chunk.chunk_id = readString(4);

    // If the ID is not ASCII-printable, we've likely hit non-chunk data
    for (char c : chunk.chunk_id) {
        if (c < 32 || c > 126) {
            skip(-4); // Go back
            return {};
        }
    }

    // Try to read chunk size (usually big-endian 4 bytes)
    if (!ensure(4)) return chunk;
    chunk.size = readU32BE();

    // Clamp size to remaining buffer
    if (chunk.size > size_ - pos_) {
        chunk.size = static_cast<uint32_t>(size_ - pos_);
    }

    // Read chunk data
    if (!ensure(chunk.size)) {
        chunk.size = static_cast<uint32_t>(size_ - pos_);
    }
    chunk.data.resize(chunk.size);
    std::memcpy(chunk.data.data(), data_ + pos_, chunk.size);
    skip(chunk.size);

    return chunk;
}

bool Sff1Reader::isKnownChunk(const std::string& id) const noexcept {
    for (size_t i = 0; i < kKnownChunkCount; ++i) {
        if (id == kKnownChunks[i]) return true;
    }
    return false;
}

// ── Section Parsing ───────────────────────────────────────────────

SffSection Sff1Reader::parseSection(const SffChunk& chunk) noexcept {
    SffSection section{};
    section.resolution = 480;
    section.bars = 4;
    section.name = chunk.chunk_id;
    section.casm_parsed = false;

    // Store original pos/ptr for nested parsing
    const uint8_t* saved_data = data_;
    size_t saved_size = size_;
    size_t saved_pos = pos_;

    data_ = chunk.data.data();
    size_ = chunk.data.size();
    pos_ = 0;

    // Try to parse tracks from the section data
    uint32_t trackCount = 0;
    if (ensure(1)) {
        trackCount = readU8(); // First byte might be track count
    }

    for (uint32_t t = 0; t < trackCount && t < 12 && pos_ < size_; ++t) {
        SffTrack track = parseTrack(data_, size_);
        section.tracks.push_back(std::move(track));
    }

    // Restore
    data_ = saved_data;
    size_ = saved_size;
    pos_ = saved_pos;

    return section;
}

SffTrack Sff1Reader::parseTrack(const uint8_t* data, size_t size) noexcept {
    SffTrack track{};
    track.midi_channel = 0;
    track.role = SffTrackRole::Phrase1;
    return track;
}

// ── MIDI Event Parsing ────────────────────────────────────────────

std::vector<SffMidiEvent> Sff1Reader::parseMidiEvents(const uint8_t* data,
                                                        size_t size,
                                                        uint32_t& offset) noexcept {
    std::vector<SffMidiEvent> events;
    if (offset >= size) return events;

    const uint8_t* d = data + offset;
    size_t remaining = size - offset;
    uint32_t running_status = 0;
    uint32_t absolute_tick = 0;

    // Try to parse MIDI-like events (variable-length delta + status + data)
    size_t pos = 0;
    while (pos < remaining) {
        // Try variable-length delta
        uint32_t delta = 0;
        uint32_t shift = 0;
        bool hasDelta = false;

        while (pos < remaining && shift < 28) {
            uint8_t byte = d[pos++];
            delta |= (byte & 0x7F) << shift;
            shift += 7;
            if ((byte & 0x80) == 0) {
                hasDelta = true;
                break;
            }
        }

        if (!hasDelta) break;

        absolute_tick += delta;

        // Read status byte
        if (pos >= remaining) break;
        uint8_t status = d[pos];
        if (status & 0x80) {
            running_status = status;
            pos++;
        }
        status = running_status;

        if (status == 0) break; // End of track

        uint8_t data1 = 0, data2 = 0;
        uint8_t eventType = status & 0xF0;

        switch (eventType) {
            case 0x80: // Note Off
            case 0x90: // Note On
            case 0xA0: // Poly Pressure
            case 0xB0: // Control Change
            case 0xE0: // Pitch Bend
                if (pos + 2 > remaining) break;
                data1 = d[pos++];
                data2 = d[pos++];
                break;
            case 0xC0: // Program Change
            case 0xD0: // Channel Pressure
                if (pos + 1 > remaining) break;
                data1 = d[pos++];
                data2 = 0;
                break;
            case 0xF0: // System
                if (status == 0xFF) { // Meta event
                    if (pos + 1 > remaining) break;
                    uint8_t metaType = d[pos++];
                    if (pos >= remaining) break;
                    uint32_t metaLen = 0;
                    shift = 0;
                    while (pos < remaining && shift < 28) {
                        uint8_t mb = d[pos++];
                        metaLen |= (mb & 0x7F) << shift;
                        shift += 7;
                        if ((mb & 0x80) == 0) break;
                    }
                    if (metaType == 0x2F) break; // End of Track
                    pos += metaLen;
                }
                continue; // Don't create an event for system
        }

        SffMidiEvent ev;
        ev.tick = absolute_tick;
        ev.status = status;
        ev.data1 = data1;
        ev.data2 = data2;
        events.push_back(ev);
    }

    offset += pos;
    return events;
}

// ── Reader Helpers ────────────────────────────────────────────────

uint8_t Sff1Reader::readU8() noexcept {
    if (!ensure(1)) return 0;
    return data_[pos_++];
}

uint16_t Sff1Reader::readU16LE() noexcept {
    if (!ensure(2)) return 0;
    uint16_t v = data_[pos_] | (static_cast<uint16_t>(data_[pos_ + 1]) << 8);
    pos_ += 2;
    return v;
}

uint32_t Sff1Reader::readU32LE() noexcept {
    if (!ensure(4)) return 0;
    uint32_t v = data_[pos_] |
                 (static_cast<uint32_t>(data_[pos_ + 1]) << 8) |
                 (static_cast<uint32_t>(data_[pos_ + 2]) << 16) |
                 (static_cast<uint32_t>(data_[pos_ + 3]) << 24);
    pos_ += 4;
    return v;
}

uint32_t Sff1Reader::readU32BE() noexcept {
    if (!ensure(4)) return 0;
    uint32_t v = (static_cast<uint32_t>(data_[pos_]) << 24) |
                 (static_cast<uint32_t>(data_[pos_ + 1]) << 16) |
                 (static_cast<uint32_t>(data_[pos_ + 2]) << 8) |
                 static_cast<uint32_t>(data_[pos_ + 3]);
    pos_ += 4;
    return v;
}

std::string Sff1Reader::readString(size_t len) noexcept {
    if (!ensure(len)) return {};
    std::string s(reinterpret_cast<const char*>(data_ + pos_), len);
    pos_ += len;
    return s;
}

void Sff1Reader::skip(size_t bytes) noexcept {
    if (bytes > size_ - pos_) {
        pos_ = size_;
    } else {
        pos_ += bytes;
    }
}

bool Sff1Reader::ensure(size_t bytes) noexcept {
    return pos_ + bytes <= size_;
}

} // namespace ai_arranger::importers::sff1

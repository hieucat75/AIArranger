#ifndef AI_ARRANGER_SFF1_READER_H
#define AI_ARRANGER_SFF1_READER_H

#include "importers/sff1/sff1_types.h"
#include <vector>
#include <cstdint>

namespace ai_arranger::importers::sff1 {

/**
 * SFF1 binary reader.
 *
 * Reads Yamaha SFF1 (.sty) files, parses known chunks,
 * and extracts MIDI data from recognized sections.
 *
 * Architecture rule: This is importer-layer ONLY.
 * No runtime code touches SFF1 types.
 * No vendor logic leaks into engine.
 */

class Sff1Reader {
public:
    Sff1Reader();

    ParseResult parseFile(const std::string& path) noexcept;
    ParseResult parseBuffer(const std::vector<uint8_t>& buffer,
                            const std::string& filename) noexcept;

private:
    // ── Binary reading helpers ─────────────────────────────────────
    uint8_t  readU8() noexcept;
    uint16_t readU16LE() noexcept;
    uint32_t readU32LE() noexcept;
    uint32_t readU32BE() noexcept;
    std::string readString(size_t len) noexcept;
    void skip(size_t bytes) noexcept;
    bool ensure(size_t bytes) noexcept;

    // ── Chunk detection ────────────────────────────────────────────
    SffChunk readChunk() noexcept;
    bool isKnownChunk(const std::string& id) const noexcept;

    // ── Section parsing ────────────────────────────────────────────
    SffSection parseSection(const SffChunk& chunk) noexcept;
    SffTrack parseTrack(const uint8_t* data, size_t size) noexcept;

    // ── MIDI event parsing ─────────────────────────────────────────
    std::vector<SffMidiEvent> parseMidiEvents(const uint8_t* data,
                                              size_t size,
                                              uint32_t& offset) noexcept;

    // ── SMF/SFF2 helpers ────────────────────────────────────────────
    bool parseMThd(const SffChunk& chunk) noexcept;
    bool parseMTrk(const SffChunk& chunk) noexcept;

    // ── CASM parsing (Claude-generated) ──────────────────────────────
    bool parseCasm(const uint8_t* data, size_t size) noexcept;
    void parseCtb2Block(const uint8_t* data, size_t size) noexcept;

    // ── State ──────────────────────────────────────────────────────
    const uint8_t* data_{nullptr};
    size_t size_{0};
    size_t pos_{0};
    ParseResult result_;
};

} // namespace ai_arranger::importers::sff1
#endif // AI_ARRANGER_SFF1_READER_H

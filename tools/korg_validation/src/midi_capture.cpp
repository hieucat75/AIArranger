#include "korg_validation/midi_capture.h"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <string>

namespace ai_arranger::korg_validation {

double tickToMs(uint64_t tick, uint32_t ppqn, double bpm) noexcept {
    if (ppqn == 0 || bpm <= 0.0) return 0.0;
    const double msPerQuarter = 60000.0 / bpm;
    return (static_cast<double>(tick) / static_cast<double>(ppqn)) * msPerQuarter;
}

namespace {

// Parse a token as decimal or 0x-hex. Returns false on garbage.
bool parseInt(const std::string& tok, long& out) {
    std::string s = tok;
    // trim
    size_t a = s.find_first_not_of(" \t\r\n");
    size_t b = s.find_last_not_of(" \t\r\n");
    if (a == std::string::npos) return false;
    s = s.substr(a, b - a + 1);
    try {
        size_t idx = 0;
        if (s.size() > 2 && (s[0] == '0') && (s[1] == 'x' || s[1] == 'X'))
            out = std::stol(s, &idx, 16);
        else
            out = std::stol(s, &idx, 10);
        return idx == s.size();
    } catch (...) {
        return false;
    }
}

void sortByTick(MidiCapture& cap) {
    std::stable_sort(cap.events.begin(), cap.events.end(),
                     [](const TimedMidiEvent& a, const TimedMidiEvent& b) {
                         return a.tick < b.tick;
                     });
}

} // namespace

MidiCapture loadCsvString(const std::string& text) {
    MidiCapture cap;
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line)) {
        // strip comment
        size_t hash = line.find('#');
        if (hash != std::string::npos) line = line.substr(0, hash);
        // blank?
        if (line.find_first_not_of(" \t\r\n") == std::string::npos) continue;

        // ppqn directive
        size_t eq = line.find('=');
        if (eq != std::string::npos &&
            line.find("ppqn") != std::string::npos) {
            long v = 0;
            if (parseInt(line.substr(eq + 1), v) && v > 0)
                cap.ppqn = static_cast<uint32_t>(v);
            continue;
        }

        // tick,status,data1,data2
        std::vector<std::string> cols;
        std::stringstream ls(line);
        std::string cell;
        while (std::getline(ls, cell, ',')) cols.push_back(cell);
        if (cols.size() < 4) continue;

        long t = 0, st = 0, d1 = 0, d2 = 0;
        if (!parseInt(cols[0], t) || !parseInt(cols[1], st) ||
            !parseInt(cols[2], d1) || !parseInt(cols[3], d2))
            continue;

        TimedMidiEvent ev;
        ev.tick   = static_cast<uint64_t>(t < 0 ? 0 : t);
        ev.status = static_cast<uint8_t>(st & 0xFF);
        ev.data1  = static_cast<uint8_t>(d1 & 0x7F);
        ev.data2  = static_cast<uint8_t>(d2 & 0x7F);
        cap.events.push_back(ev);
    }
    sortByTick(cap);
    return cap;
}

MidiCapture loadCsv(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return {};
    std::stringstream ss;
    ss << f.rdbuf();
    return loadCsvString(ss.str());
}

MidiCapture loadSmfBytes(const std::vector<uint8_t>& b) {
    MidiCapture cap;
    auto be16 = [&](size_t i) -> uint16_t {
        return static_cast<uint16_t>((b[i] << 8) | b[i + 1]);
    };
    auto be32 = [&](size_t i) -> uint32_t {
        return (static_cast<uint32_t>(b[i]) << 24) |
               (static_cast<uint32_t>(b[i + 1]) << 16) |
               (static_cast<uint32_t>(b[i + 2]) << 8) |
               static_cast<uint32_t>(b[i + 3]);
    };

    if (b.size() < 14) return cap;
    if (!(b[0] == 'M' && b[1] == 'T' && b[2] == 'h' && b[3] == 'd')) return cap;
    const uint16_t division = be16(12);          // big-endian per SMF spec
    if (!(division & 0x8000) && division != 0) cap.ppqn = division;

    size_t pos = 8 + be32(4);                     // skip MThd body
    while (pos + 8 <= b.size()) {
        const bool isTrk = (b[pos] == 'M' && b[pos + 1] == 'T' &&
                            b[pos + 2] == 'r' && b[pos + 3] == 'k');
        const uint32_t len = be32(pos + 4);
        const size_t body = pos + 8;
        const size_t end = std::min(body + len, b.size());
        pos = body + len;
        if (!isTrk) continue;

        size_t p = body;
        uint32_t absTick = 0;
        uint8_t running = 0;
        while (p < end) {
            // variable-length delta
            uint32_t delta = 0;
            int guard = 0;
            while (p < end && guard < 5) {
                const uint8_t c = b[p++];
                delta = (delta << 7) | (c & 0x7F);
                ++guard;
                if (!(c & 0x80)) break;
            }
            absTick += delta;
            if (p >= end) break;

            uint8_t status = b[p];
            if (status & 0x80) { running = status; ++p; }
            else status = running;
            const uint8_t type = status & 0xF0;

            if (status == 0xFF) {                 // meta
                if (p >= end) break;
                ++p;                              // meta type
                uint32_t mlen = 0; int g = 0;
                while (p < end && g < 5) {
                    const uint8_t c = b[p++]; mlen = (mlen << 7) | (c & 0x7F);
                    ++g; if (!(c & 0x80)) break;
                }
                p += mlen;
                continue;
            }
            if (status == 0xF0 || status == 0xF7) { // sysex — skip by length
                uint32_t slen = 0; int g = 0;
                while (p < end && g < 5) {
                    const uint8_t c = b[p++]; slen = (slen << 7) | (c & 0x7F);
                    ++g; if (!(c & 0x80)) break;
                }
                p += slen;
                continue;
            }

            uint8_t d1 = 0, d2 = 0;
            if (type == 0xC0 || type == 0xD0) {
                if (p < end) d1 = b[p++];
            } else {
                if (p < end) d1 = b[p++];
                if (p < end) d2 = b[p++];
            }
            TimedMidiEvent ev{absTick, status, static_cast<uint8_t>(d1 & 0x7F),
                              static_cast<uint8_t>(d2 & 0x7F)};
            cap.events.push_back(ev);
        }
    }
    sortByTick(cap);
    return cap;
}

MidiCapture loadSmf(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return {};
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(f)),
                               std::istreambuf_iterator<char>());
    return loadSmfBytes(bytes);
}

} // namespace ai_arranger::korg_validation

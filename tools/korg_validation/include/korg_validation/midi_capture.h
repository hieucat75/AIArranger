#ifndef KORG_VALIDATION_MIDI_CAPTURE_H
#define KORG_VALIDATION_MIDI_CAPTURE_H

#include <cstdint>
#include <string>
#include <vector>

// ── MIDI capture pipeline (Gate 11 Task B) ───────────────────────────
//
// Normalizes a MIDI stream — from a synthetic CSV fixture, a Standard MIDI File
// (SMF, for future real-device captures), or the engine itself — into a single
// in-memory representation the analyzers consume. Synthetic-only in CI.

namespace ai_arranger::korg_validation {

struct TimedMidiEvent {
    uint64_t tick;     // absolute tick (PPQN-relative)
    uint8_t  status;   // full status byte (type | channel)
    uint8_t  data1;
    uint8_t  data2;

    uint8_t type() const noexcept    { return status & 0xF0; }
    uint8_t channel() const noexcept { return status & 0x0F; }
    bool isNoteOn() const noexcept   { return type() == 0x90 && data2 > 0; }
    bool isNoteOff() const noexcept  {
        return type() == 0x80 || (type() == 0x90 && data2 == 0);
    }
};

struct MidiCapture {
    uint32_t ppqn = 480;                 // ticks per quarter note
    std::vector<TimedMidiEvent> events;  // ascending tick order
};

// Convert a tick to milliseconds at a given tempo.
double tickToMs(uint64_t tick, uint32_t ppqn, double bpm) noexcept;

// ── Loaders ───────────────────────────────────────────────────────────
// CSV: lines `tick,status,data1,data2`; `#` comments and blank lines ignored;
// an optional `ppqn=<n>` directive line sets the resolution. Hex (0x90) or
// decimal accepted for status/data.
MidiCapture loadCsvString(const std::string& text);
MidiCapture loadCsv(const std::string& path);

// SMF: minimal MThd(division)+MTrk(delta) parser. Big-endian division per spec.
MidiCapture loadSmfBytes(const std::vector<uint8_t>& bytes);
MidiCapture loadSmf(const std::string& path);

} // namespace ai_arranger::korg_validation

#endif // KORG_VALIDATION_MIDI_CAPTURE_H

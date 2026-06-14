#ifndef KORG_VALIDATION_STUCK_NOTE_DETECTOR_H
#define KORG_VALIDATION_STUCK_NOTE_DETECTOR_H

#include "korg_validation/midi_capture.h"
#include <cstdint>
#include <utility>
#include <vector>

// ── Panic / stuck-note detector (Gate 11 Task E) ─────────────────────
//
// Read-only NoteOn/NoteOff balance check over a captured stream (mirrors the
// engine's note-balance invariant, but as an offline analyzer). A clean stream
// ends with every note released.

namespace ai_arranger::korg_validation {

struct StuckResult {
    bool balanced = true;                          // no notes left sounding
    int hanging = 0;                               // distinct (ch,note) still on
    int double_on = 0;                             // NoteOn while already active
    int orphan_off = 0;                            // NoteOff with no active note
    std::vector<std::pair<int,int>> hanging_notes; // (channel, note)
};

StuckResult detectStuck(const MidiCapture& cap);

} // namespace ai_arranger::korg_validation

#endif // KORG_VALIDATION_STUCK_NOTE_DETECTOR_H

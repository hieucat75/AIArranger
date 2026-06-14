#ifndef KORG_VALIDATION_LATENCY_METER_H
#define KORG_VALIDATION_LATENCY_METER_H

#include "korg_validation/midi_capture.h"
#include <cstdint>
#include <vector>

// ── Chord response latency meter (Gate 11 Task D) ────────────────────
//
// Measures the time from a chord-change event to the first re-voiced NoteOn at
// or after it, in the engine stream.

namespace ai_arranger::korg_validation {

struct LatencyResult {
    int events = 0;                  // chord changes that produced a note
    int missing = 0;                 // chord changes with no following note
    double mean_ms = 0.0;
    double max_ms = 0.0;
    std::vector<double> latencies_ms;
};

LatencyResult measureLatency(const MidiCapture& engine,
                             const std::vector<uint64_t>& chordChangeTicks,
                             double bpm);

} // namespace ai_arranger::korg_validation

#endif // KORG_VALIDATION_LATENCY_METER_H

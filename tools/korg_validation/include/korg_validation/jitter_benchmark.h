#ifndef KORG_VALIDATION_JITTER_BENCHMARK_H
#define KORG_VALIDATION_JITTER_BENCHMARK_H

#include "korg_validation/midi_capture.h"
#include <cstdint>

// ── Jitter benchmark (Gate 11 Task E) ────────────────────────────────
//
// Measures how far NoteOn attacks deviate from an expected timing grid
// (e.g. an 8th-note pulse). Reports the deviation distribution in ms.

namespace ai_arranger::korg_validation {

struct JitterResult {
    int events = 0;
    double mean_abs_ms = 0.0;
    double p95_ms = 0.0;
    double max_ms = 0.0;
};

JitterResult benchmarkJitter(const MidiCapture& cap,
                             uint64_t gridTicks,
                             double bpm);

} // namespace ai_arranger::korg_validation

#endif // KORG_VALIDATION_JITTER_BENCHMARK_H

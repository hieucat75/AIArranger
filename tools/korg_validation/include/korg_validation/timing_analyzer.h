#ifndef KORG_VALIDATION_TIMING_ANALYZER_H
#define KORG_VALIDATION_TIMING_ANALYZER_H

#include "korg_validation/midi_capture.h"
#include <vector>

// ── Timing differential analyzer (Gate 11 Task C) ────────────────────
//
// Aligns an engine stream against a reference stream and reports per-event
// timing deltas (engine − reference, in ms). Matching is order-preserving per
// event key (status byte + data1), so identical streams give all-zero deltas,
// a constant shift surfaces as a constant delta, and dropped/extra events
// surface as orphans.

namespace ai_arranger::korg_validation {

struct TimingSummary {
    int matched = 0;
    int orphan_engine = 0;      // engine events with no reference match
    int orphan_reference = 0;   // reference events with no engine match
    int within_tolerance = 0;   // |delta| <= toleranceMs
    double mean_ms = 0.0;       // signed mean of matched deltas
    double max_abs_ms = 0.0;
    double stddev_ms = 0.0;
    std::vector<double> deltas_ms;
};

// `bpm` converts ticks to ms for both streams (each uses its own ppqn).
TimingSummary analyzeTiming(const MidiCapture& engine,
                            const MidiCapture& reference,
                            double bpm,
                            double toleranceMs = 1.0);

} // namespace ai_arranger::korg_validation

#endif // KORG_VALIDATION_TIMING_ANALYZER_H

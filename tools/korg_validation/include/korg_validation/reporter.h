#ifndef KORG_VALIDATION_REPORTER_H
#define KORG_VALIDATION_REPORTER_H

#include "korg_validation/timing_analyzer.h"
#include "korg_validation/transition_validator.h"
#include "korg_validation/latency_meter.h"
#include "korg_validation/stuck_note_detector.h"
#include "korg_validation/jitter_benchmark.h"
#include <string>
#include <vector>

// ── Reporter (Gate 11 Task F) ────────────────────────────────────────
//
// Aggregates analyzer results into JSON (machine) + Markdown (human). Every
// report hard-codes `hardware_validated: false` — the report itself encodes the
// claims policy; it stays false until committed real-device evidence exists.

namespace ai_arranger::korg_validation {

struct ValidationReport {
    std::string fixture;
    TimingSummary timing;
    std::vector<TransitionCheck> transitions;
    LatencyResult latency;
    StuckResult stuck;
    JitterResult jitter;
};

std::string toJson(const ValidationReport& r);
std::string toMarkdown(const ValidationReport& r);

// Writes JSON and/or Markdown to disk (empty path = skip). Returns true on
// every requested write succeeding.
bool writeReport(const ValidationReport& r,
                 const std::string& jsonPath,
                 const std::string& mdPath);

} // namespace ai_arranger::korg_validation

#endif // KORG_VALIDATION_REPORTER_H

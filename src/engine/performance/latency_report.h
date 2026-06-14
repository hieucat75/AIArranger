#ifndef AI_ARRANGER_PERFORMANCE_LATENCY_REPORT_H
#define AI_ARRANGER_PERFORMANCE_LATENCY_REPORT_H

#include <string>
#include <vector>

// ── Performer latency budget + reporter (Gate 12 Task I) ─────────────
//
// Measures control->effect latency for the performer layer (chord detection,
// transition resolution, fill scheduling, queue resolution) against a budget,
// and emits a JSON + Markdown report. The report ALWAYS carries
// `deterministic: true` (logical-step latency, replayable) and
// `hardware_validated: false` (no real-device measurement) — hard-coded
// constants, encoding the claims policy. No DSP; offline aggregation only.

namespace ai_arranger::performance {

// Claims-policy constants — never flip without committed evidence + policy PR.
inline constexpr bool kDeterministic = true;
inline constexpr bool kHardwareValidated = false;

struct LatencyMetric {
    std::string name;
    int samples = 0;
    double mean_ms = 0.0;
    double max_ms = 0.0;
    double budget_ms = 0.0;
    bool within_budget = true;
};

struct PerformerLatencyReport {
    std::string scenario;
    std::vector<LatencyMetric> metrics;
    bool allWithinBudget() const noexcept;
};

// Aggregate latency samples (ms) into a metric vs a budget.
LatencyMetric measureLatency(const std::string& name,
                             const std::vector<double>& samplesMs,
                             double budgetMs);

std::string toJson(const PerformerLatencyReport& r);
std::string toMarkdown(const PerformerLatencyReport& r);
bool writeReport(const PerformerLatencyReport& r,
                 const std::string& jsonPath, const std::string& mdPath);

} // namespace ai_arranger::performance

#endif // AI_ARRANGER_PERFORMANCE_LATENCY_REPORT_H

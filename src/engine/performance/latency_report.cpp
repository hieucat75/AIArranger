#include "engine/performance/latency_report.h"

#include <cstdio>
#include <fstream>
#include <sstream>

namespace ai_arranger::performance {

bool PerformerLatencyReport::allWithinBudget() const noexcept {
    for (const auto& m : metrics) if (!m.within_budget) return false;
    return true;
}

LatencyMetric measureLatency(const std::string& name,
                             const std::vector<double>& samplesMs,
                             double budgetMs) {
    LatencyMetric m;
    m.name = name;
    m.budget_ms = budgetMs;
    m.samples = static_cast<int>(samplesMs.size());
    double sum = 0.0;
    for (double s : samplesMs) { sum += s; if (s > m.max_ms) m.max_ms = s; }
    if (m.samples > 0) m.mean_ms = sum / m.samples;
    m.within_budget = m.max_ms <= budgetMs;
    return m;
}

namespace {
std::string num(double v) { char b[48]; std::snprintf(b, sizeof(b), "%.3f", v); return b; }
}

std::string toJson(const PerformerLatencyReport& r) {
    std::ostringstream o;
    o << "{\n";
    o << "  \"deterministic\": " << (kDeterministic ? "true" : "false") << ",\n";
    o << "  \"hardware_validated\": " << (kHardwareValidated ? "true" : "false") << ",\n";
    o << "  \"scenario\": \"" << r.scenario << "\",\n";
    o << "  \"all_within_budget\": " << (r.allWithinBudget() ? "true" : "false") << ",\n";
    o << "  \"metrics\": [";
    for (size_t i = 0; i < r.metrics.size(); ++i) {
        const auto& m = r.metrics[i];
        if (i) o << ",";
        o << "\n    { \"name\": \"" << m.name << "\", \"samples\": " << m.samples
          << ", \"mean_ms\": " << num(m.mean_ms)
          << ", \"max_ms\": " << num(m.max_ms)
          << ", \"budget_ms\": " << num(m.budget_ms)
          << ", \"within_budget\": " << (m.within_budget ? "true" : "false") << " }";
    }
    o << (r.metrics.empty() ? "" : "\n  ") << "]\n}\n";
    return o.str();
}

std::string toMarkdown(const PerformerLatencyReport& r) {
    std::ostringstream o;
    o << "# Performer Latency Report — " << r.scenario << "\n\n";
    o << "> **deterministic: " << (kDeterministic ? "true" : "false")
      << "** · **hardware_validated: " << (kHardwareValidated ? "true" : "false")
      << "** (logical-step latency; no real-device measurement)\n\n";
    o << "| metric | samples | mean ms | max ms | budget ms | within budget |\n";
    o << "|---|--:|--:|--:|--:|:--:|\n";
    for (const auto& m : r.metrics) {
        o << "| " << m.name << " | " << m.samples << " | " << num(m.mean_ms)
          << " | " << num(m.max_ms) << " | " << num(m.budget_ms) << " | "
          << (m.within_budget ? "yes" : "no") << " |\n";
    }
    o << "\n**All within budget:** " << (r.allWithinBudget() ? "yes" : "no") << "\n";
    return o.str();
}

bool writeReport(const PerformerLatencyReport& r,
                 const std::string& jsonPath, const std::string& mdPath) {
    bool ok = true;
    if (!jsonPath.empty()) { std::ofstream f(jsonPath); if (f) f << toJson(r); else ok = false; }
    if (!mdPath.empty())   { std::ofstream f(mdPath);   if (f) f << toMarkdown(r); else ok = false; }
    return ok;
}

} // namespace ai_arranger::performance

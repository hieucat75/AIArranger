#include "korg_validation/reporter.h"
#include "korg_validation/harness.h"

#include <cstdio>
#include <fstream>
#include <sstream>

namespace ai_arranger::korg_validation {

namespace {

std::string num(double v) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.3f", v);
    return buf;
}

std::string jstr(const std::string& s) {
    std::string out = "\"";
    for (char c : s) {
        if (c == '"' || c == '\\') { out += '\\'; out += c; }
        else if (c == '\n') out += "\\n";
        else out += c;
    }
    out += '"';
    return out;
}

} // namespace

std::string toJson(const ValidationReport& r) {
    std::ostringstream o;
    o << "{\n";
    o << "  \"harness_version\": " << kHarnessVersion << ",\n";
    // Claims policy: ALWAYS false until committed real-device evidence exists.
    o << "  \"hardware_validated\": " << (kHardwareValidated ? "true" : "false") << ",\n";
    o << "  \"fixture\": " << jstr(r.fixture) << ",\n";

    o << "  \"timing\": { \"matched\": " << r.timing.matched
      << ", \"orphan_engine\": " << r.timing.orphan_engine
      << ", \"orphan_reference\": " << r.timing.orphan_reference
      << ", \"within_tolerance\": " << r.timing.within_tolerance
      << ", \"mean_ms\": " << num(r.timing.mean_ms)
      << ", \"max_abs_ms\": " << num(r.timing.max_abs_ms)
      << ", \"stddev_ms\": " << num(r.timing.stddev_ms) << " },\n";

    o << "  \"transitions\": [";
    for (size_t i = 0; i < r.transitions.size(); ++i) {
        const auto& t = r.transitions[i];
        if (i) o << ",";
        o << "\n    { \"label\": " << jstr(t.label)
          << ", \"expected_tick\": " << t.expected_tick
          << ", \"actual_tick\": " << t.actual_tick
          << ", \"delta\": " << t.delta
          << ", \"grid_offset\": " << t.grid_offset
          << ", \"on_boundary\": " << (t.on_boundary ? "true" : "false")
          << ", \"on_time\": " << (t.on_time ? "true" : "false") << " }";
    }
    o << (r.transitions.empty() ? "" : "\n  ") << "],\n";

    o << "  \"latency\": { \"events\": " << r.latency.events
      << ", \"missing\": " << r.latency.missing
      << ", \"mean_ms\": " << num(r.latency.mean_ms)
      << ", \"max_ms\": " << num(r.latency.max_ms) << " },\n";

    o << "  \"stuck\": { \"balanced\": " << (r.stuck.balanced ? "true" : "false")
      << ", \"hanging\": " << r.stuck.hanging
      << ", \"double_on\": " << r.stuck.double_on
      << ", \"orphan_off\": " << r.stuck.orphan_off << " },\n";

    o << "  \"jitter\": { \"events\": " << r.jitter.events
      << ", \"mean_abs_ms\": " << num(r.jitter.mean_abs_ms)
      << ", \"p95_ms\": " << num(r.jitter.p95_ms)
      << ", \"max_ms\": " << num(r.jitter.max_ms) << " }\n";

    o << "}\n";
    return o.str();
}

std::string toMarkdown(const ValidationReport& r) {
    std::ostringstream o;
    o << "# KORG Validation Report — " << r.fixture << "\n\n";
    o << "> **hardware_validated: " << (kHardwareValidated ? "true" : "false")
      << "** (synthetic fixture; no real-device parity claimed) · harness v"
      << kHarnessVersion << "\n\n";

    o << "## Timing\n\n";
    o << "| matched | orphan(eng) | orphan(ref) | within tol | mean ms | max|abs| ms | stddev ms |\n";
    o << "|--:|--:|--:|--:|--:|--:|--:|\n";
    o << "| " << r.timing.matched << " | " << r.timing.orphan_engine << " | "
      << r.timing.orphan_reference << " | " << r.timing.within_tolerance << " | "
      << num(r.timing.mean_ms) << " | " << num(r.timing.max_abs_ms) << " | "
      << num(r.timing.stddev_ms) << " |\n\n";

    o << "## Transitions\n\n";
    if (r.transitions.empty()) {
        o << "_none_\n\n";
    } else {
        o << "| label | expected | actual | delta | grid off | on boundary | on time |\n";
        o << "|---|--:|--:|--:|--:|:--:|:--:|\n";
        for (const auto& t : r.transitions) {
            o << "| " << t.label << " | " << t.expected_tick << " | "
              << t.actual_tick << " | " << t.delta << " | " << t.grid_offset
              << " | " << (t.on_boundary ? "yes" : "no") << " | "
              << (t.on_time ? "yes" : "no") << " |\n";
        }
        o << "\n";
    }

    o << "## Chord latency\n\n";
    o << "| events | missing | mean ms | max ms |\n|--:|--:|--:|--:|\n";
    o << "| " << r.latency.events << " | " << r.latency.missing << " | "
      << num(r.latency.mean_ms) << " | " << num(r.latency.max_ms) << " |\n\n";

    o << "## Stuck-note\n\n";
    o << "| balanced | hanging | double-on | orphan-off |\n|:--:|--:|--:|--:|\n";
    o << "| " << (r.stuck.balanced ? "yes" : "no") << " | " << r.stuck.hanging
      << " | " << r.stuck.double_on << " | " << r.stuck.orphan_off << " |\n\n";

    o << "## Jitter\n\n";
    o << "| events | mean abs ms | p95 ms | max ms |\n|--:|--:|--:|--:|\n";
    o << "| " << r.jitter.events << " | " << num(r.jitter.mean_abs_ms) << " | "
      << num(r.jitter.p95_ms) << " | " << num(r.jitter.max_ms) << " |\n";

    return o.str();
}

bool writeReport(const ValidationReport& r,
                 const std::string& jsonPath,
                 const std::string& mdPath) {
    bool ok = true;
    if (!jsonPath.empty()) {
        std::ofstream f(jsonPath, std::ios::binary);
        if (f) f << toJson(r); else ok = false;
    }
    if (!mdPath.empty()) {
        std::ofstream f(mdPath, std::ios::binary);
        if (f) f << toMarkdown(r); else ok = false;
    }
    return ok;
}

} // namespace ai_arranger::korg_validation

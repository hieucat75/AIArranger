// Gate 12 Task I — latency budget benchmarks + reporter.
// Reports MUST carry deterministic=true + hardware_validated=false.

#include "engine/performance/latency_report.h"
#include "engine/performance/sync_controller.h"
#include "engine/performance/fill_scheduler.h"
#include "engine/performance/variation_manager.h"
#include "engine/music/groove.h"   // not required; kept for tick helpers parity
#include <cstdio>
#include <string>
#include <vector>

using namespace ai_arranger::performance;

static int failures = 0, passes = 0;
#define TEST(name, expr) do { \
    if (!(expr)) { std::fprintf(stderr, "  \xE2\x9D\x8C FAIL: %s\n", name); failures++; } \
    else { std::printf("  \xE2\x9C\x85 PASS: %s\n", name); passes++; } \
} while(0)
static bool has(const std::string& h, const std::string& n) { return h.find(n) != std::string::npos; }

int main() {
    std::printf("Test: Gate 12 Task I — latency budget + reporter\n");

    // ── Logical-step latencies (deterministic) ──
    // Chord detection + queue resolution are synchronous -> 0 ms.
    std::vector<double> chordDet(64, 0.0);
    std::vector<double> queueRes(64, 0.0);
    // Transition/fill resolve on the next bar boundary -> quantize delay in ms,
    // deterministic given the in-bar position (example samples within one bar).
    std::vector<double> transition = {0.0, 125.0, 250.0, 375.0};  // <= 1 bar @120bpm
    std::vector<double> fill       = {0.0, 250.0, 500.0};

    LatencyMetric cd = measureLatency("chord_detection", chordDet, 1.0);
    LatencyMetric qr = measureLatency("queue_resolution", queueRes, 1.0);
    LatencyMetric tr = measureLatency("transition", transition, 500.0);
    LatencyMetric fl = measureLatency("fill_scheduling", fill, 500.0);

    TEST("chord detection within 1ms budget", cd.within_budget && cd.max_ms == 0.0);
    TEST("queue resolution within 1ms budget", qr.within_budget);
    TEST("transition within bar budget", tr.within_budget && tr.max_ms == 375.0);
    TEST("fill within bar budget", fl.within_budget && fl.max_ms == 500.0);

    // ── Over-budget detection ──
    LatencyMetric over = measureLatency("x", {0.0, 600.0}, 500.0);
    TEST("over-budget flagged", !over.within_budget);

    // ── Report shape + claims policy ──
    PerformerLatencyReport rep;
    rep.scenario = "synthetic_performer_steps";
    rep.metrics = {cd, qr, tr, fl};
    TEST("report all within budget", rep.allWithinBudget());

    const std::string js = toJson(rep);
    const std::string md = toMarkdown(rep);
    TEST("JSON deterministic:true", has(js, "\"deterministic\": true"));
    TEST("JSON hardware_validated:false", has(js, "\"hardware_validated\": false"));
    TEST("JSON never claims hardware true", !has(js, "\"hardware_validated\": true"));
    TEST("JSON has metrics", has(js, "chord_detection") && has(js, "fill_scheduling"));
    TEST("MD deterministic + hw flags", has(md, "deterministic: true") &&
         has(md, "hardware_validated: false"));
    TEST("MD title", has(md, "# Performer Latency Report"));

    // ── Determinism: rebuild identical report twice ──
    PerformerLatencyReport rep2;
    rep2.scenario = rep.scenario; rep2.metrics = {cd, qr, tr, fl};
    TEST("deterministic JSON (identical inputs)", toJson(rep) == toJson(rep2));

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

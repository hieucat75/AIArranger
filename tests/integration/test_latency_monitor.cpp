// Gate 14 Task D — latency monitor (rolling stats, headless).

#include "ui/latency_monitor.h"
#include <cmath>
#include <cstdio>

using namespace ai_arranger::ui;

static int failures = 0, passes = 0;
#define TEST(name, expr) do { \
    if (!(expr)) { std::fprintf(stderr, "  \xE2\x9D\x8C FAIL: %s\n", name); failures++; } \
    else { std::printf("  \xE2\x9C\x85 PASS: %s\n", name); passes++; } \
} while(0)
static bool near(double a, double b) { return std::fabs(a - b) < 1e-6; }

int main() {
    std::printf("Test: Gate 14 Task D — latency monitor\n");

    LatencyMonitor m;
    TEST("empty: avg 0", near(m.average(), 0.0) && m.samples() == 0);

    m.addSample(10.0); m.addSample(20.0); m.addSample(30.0);
    TEST("avg of 10/20/30 = 20", near(m.average(), 20.0));
    TEST("max = 30", near(m.maximum(), 30.0));
    TEST("jitter = 20 (30-10)", near(m.jitter(), 20.0));
    TEST("samples = 3", m.samples() == 3);

    // ── Window wrap: only the last kWindow samples count ──
    m.reset();
    for (int i = 0; i < (int)LatencyMonitor::kWindow + 50; ++i) m.addSample(5.0);
    TEST("window capped at kWindow", m.samples() == (int)LatencyMonitor::kWindow);
    TEST("constant samples -> jitter 0", near(m.jitter(), 0.0));
    TEST("constant samples -> avg 5", near(m.average(), 5.0));

    m.reset();
    TEST("reset clears", m.samples() == 0);

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

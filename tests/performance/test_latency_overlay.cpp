// Gate 15 Task E — latency overlay rolling stats + histogram (deterministic).

#include "performance/telemetry/latency_overlay.h"
#include <cmath>
#include <cstdio>

using namespace ai_arranger::performance;

static int failures = 0, passes = 0;
#define TEST(name, expr) do { \
    if (!(expr)) { std::fprintf(stderr, "  \xE2\x9D\x8C FAIL: %s\n", name); failures++; } \
    else { std::printf("  \xE2\x9C\x85 PASS: %s\n", name); passes++; } \
} while(0)
static bool near(double a, double b) { return std::fabs(a - b) < 1e-9; }

int main() {
    std::printf("Test: Gate 15 Task E — latency overlay\n");

    LatencyOverlay o;
    TEST("empty avg 0", near(o.average(), 0.0) && o.samples() == 0);

    o.addSample(2.0); o.addSample(4.0); o.addSample(6.0);
    TEST("avg 4", near(o.average(), 4.0));
    TEST("max 6", near(o.maximum(), 6.0));
    TEST("min 2", near(o.minimum(), 2.0));
    TEST("jitter 4", near(o.jitter(), 4.0));

    // ── Histogram (bucket 1ms): 2.0->b2, 4.0->b4, 6.0->b6 ──
    {
        int h[LatencyOverlay::kBuckets];
        o.histogram(1.0, h);
        TEST("hist b2=1", h[2] == 1);
        TEST("hist b4=1", h[4] == 1);
        TEST("hist b6=1", h[6] == 1);
        TEST("hist b0=0", h[0] == 0);
    }

    // ── Overflow into last bucket ──
    {
        LatencyOverlay o2;
        o2.addSample(999.0);
        int h[LatencyOverlay::kBuckets];
        o2.histogram(1.0, h);
        TEST("overflow -> last bucket", h[LatencyOverlay::kBuckets - 1] == 1);
    }

    // ── Window wrap ──
    {
        LatencyOverlay o3;
        for (int i = 0; i < LatencyOverlay::kWindow + 100; ++i) o3.addSample(3.0);
        TEST("window capped", o3.samples() == LatencyOverlay::kWindow);
        TEST("constant -> jitter 0", near(o3.jitter(), 0.0) && near(o3.average(), 3.0));
    }

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

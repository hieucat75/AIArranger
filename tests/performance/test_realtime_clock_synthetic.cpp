// Gate 15 Task A — synthetic realtime clock + jitter helper (deterministic).

#include "performance/clock/synthetic_clock.h"
#include "performance/clock/realtime_clock.h"
#include <cmath>
#include <cstdio>
#include <vector>

using namespace ai_arranger::performance;

static int failures = 0, passes = 0;
#define TEST(name, expr) do { \
    if (!(expr)) { std::fprintf(stderr, "  \xE2\x9D\x8C FAIL: %s\n", name); failures++; } \
    else { std::printf("  \xE2\x9C\x85 PASS: %s\n", name); passes++; } \
} while(0)
static bool near(double a, double b) { return std::fabs(a - b) < 1e-9; }

int main() {
    std::printf("Test: Gate 15 Task A — synthetic clock\n");

    // ── Running gate + advance/poll ──
    {
        SyntheticClock c(48000);
        TEST("starts stopped", !c.isRunning());
        c.advance(512);
        TEST("advance ignored while stopped", c.pollElapsedSamples() == 0);
        c.start();
        TEST("running", c.isRunning());
        c.advance(512); c.advance(256);
        TEST("poll returns accumulated 768", c.pollElapsedSamples() == 768);
        TEST("poll drains to 0", c.pollElapsedSamples() == 0);
        TEST("sample rate", c.sampleRate() == 48000);
        c.stop(); TEST("stopped", !c.isRunning());
    }

    // ── Determinism: two clocks, same advances -> identical poll sequence ──
    {
        SyntheticClock a, b; a.start(); b.start();
        bool same = true;
        for (int i = 0; i < 1000; ++i) {
            a.advance(64 + (i % 7)); b.advance(64 + (i % 7));
            if (a.pollElapsedSamples() != b.pollElapsedSamples()) same = false;
        }
        TEST("deterministic replay (two clocks agree)", same);
    }

    // ── Jitter helper ──
    {
        double flat[5] = {10,10,10,10,10};
        JitterStats j = computeJitter(flat, 5);
        TEST("flat intervals -> 0 jitter", near(j.maxAbsDev, 0.0) && near(j.stddev, 0.0));
        double vary[4] = {8,12,8,12};
        JitterStats j2 = computeJitter(vary, 4);
        TEST("varying -> mean 10", near(j2.mean, 10.0));
        TEST("varying -> maxAbsDev 2", near(j2.maxAbsDev, 2.0));
        TEST("empty safe", computeJitter(nullptr, 0).samples == 0);
    }

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

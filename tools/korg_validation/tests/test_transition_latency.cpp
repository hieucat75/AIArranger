// Gate 11 Task D — transition validator + chord latency meter (synthetic).

#include "korg_validation/transition_validator.h"
#include "korg_validation/latency_meter.h"
#include "korg_validation/midi_capture.h"
#include <cmath>
#include <cstdio>

using namespace ai_arranger::korg_validation;

static int failures = 0, passes = 0;
#define TEST(name, expr) do { \
    if (!(expr)) { std::fprintf(stderr, "  \xE2\x9D\x8C FAIL: %s\n", name); failures++; } \
    else { std::printf("  \xE2\x9C\x85 PASS: %s\n", name); passes++; } \
} while(0)
static bool near(double a, double b) { return std::fabs(a - b) < 1e-6; }

int main() {
    std::printf("Test: Gate 11 Task D — transitions + latency\n");
    const uint64_t bar = 1920; // 4/4 @480ppqn

    // ── Transition validator ──
    {
        std::vector<TransitionInput> in = {
            {"Main A->B (on grid)", 1920, 1920},  // exact bar 2
            {"Fill in (late)",      2000, 1920},  // 80 ticks late, off grid
            {"Ending (early grid)", 3840, 3840},  // exact bar 3
        };
        auto checks = validateTransitions(in, bar, /*tol*/0);
        TEST("3 transitions checked", checks.size() == 3);
        TEST("on-grid exact -> on_boundary & on_time",
             checks[0].on_boundary && checks[0].on_time && checks[0].delta == 0);
        TEST("late+offgrid -> not on_boundary, not on_time",
             !checks[1].on_boundary && !checks[1].on_time && checks[1].delta == 80);
        TEST("late grid_offset = 80", checks[1].grid_offset == 80);
        TEST("ending on grid", checks[2].on_boundary && checks[2].delta == 0);

        // tolerance absorbs small slack
        auto tol = validateTransitions({{"x", 1925, 1920}}, bar, /*tol*/8);
        TEST("tolerance 8 absorbs +5", tol[0].on_boundary && tol[0].on_time);
    }

    // ── Latency meter ──
    {
        // chord change at tick 0 -> first NoteOn at 96 ticks; change at 1920 ->
        // first NoteOn at 1968 (48 ticks).
        MidiCapture e = loadCsvString(
            "ppqn=480\n96,0x90,60,90\n480,0x80,60,0\n1968,0x90,62,90\n");
        LatencyResult r = measureLatency(e, {0, 1920}, 120.0);
        TEST("latency: 2 events", r.events == 2);
        TEST("latency: 0 missing", r.missing == 0);
        // 96 ticks @120bpm/480ppqn = 100ms ; 48 ticks = 50ms ; mean = 75ms
        TEST("latency: first = 100ms", near(r.latencies_ms[0], 100.0));
        TEST("latency: second = 50ms", near(r.latencies_ms[1], 50.0));
        TEST("latency: mean 75ms", near(r.mean_ms, 75.0));
        TEST("latency: max 100ms", near(r.max_ms, 100.0));
    }
    {
        // chord change after the last note -> missing
        MidiCapture e = loadCsvString("ppqn=480\n0,0x90,60,90\n");
        LatencyResult r = measureLatency(e, {500}, 120.0);
        TEST("latency: no note after change -> missing", r.missing == 1 && r.events == 0);
    }

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

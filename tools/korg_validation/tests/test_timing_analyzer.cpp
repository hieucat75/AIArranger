// Gate 11 Task C — timing differential analyzer tests (synthetic).

#include "korg_validation/timing_analyzer.h"
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
    std::printf("Test: Gate 11 Task C — timing analyzer\n");
    const double bpm = 120.0; // @480ppqn: 1 tick = 1.041666.. ms? no: quarter=500ms -> 480 tick=500ms

    const char* base = "ppqn=480\n0,0x90,36,100\n480,0x80,36,0\n960,0x90,43,90\n";

    // ── Identical streams -> all deltas 0 ──
    {
        MidiCapture e = loadCsvString(base);
        MidiCapture r = loadCsvString(base);
        TimingSummary s = analyzeTiming(e, r, bpm);
        TEST("identical: matched=3", s.matched == 3);
        TEST("identical: no orphans", s.orphan_engine == 0 && s.orphan_reference == 0);
        TEST("identical: mean 0", near(s.mean_ms, 0.0));
        TEST("identical: max_abs 0", near(s.max_abs_ms, 0.0));
        TEST("identical: all within tolerance", s.within_tolerance == 3);
    }

    // ── Engine shifted +48 ticks (=50ms @120bpm/480ppqn) ──
    {
        MidiCapture r = loadCsvString(base);
        MidiCapture e = loadCsvString(
            "ppqn=480\n48,0x90,36,100\n528,0x80,36,0\n1008,0x90,43,90\n");
        TimingSummary s = analyzeTiming(e, r, bpm);
        TEST("shifted: matched=3", s.matched == 3);
        TEST("shifted: mean ~ +50ms", near(s.mean_ms, 50.0));
        TEST("shifted: stddev ~0 (constant shift)", s.stddev_ms < 1e-6);
        TEST("shifted: none within 1ms tolerance", s.within_tolerance == 0);
    }

    // ── Engine missing an event -> orphan_reference ──
    {
        MidiCapture r = loadCsvString(base);
        MidiCapture e = loadCsvString("ppqn=480\n0,0x90,36,100\n480,0x80,36,0\n");
        TimingSummary s = analyzeTiming(e, r, bpm);
        TEST("missing: matched=2", s.matched == 2);
        TEST("missing: 1 orphan reference", s.orphan_reference == 1);
        TEST("missing: 0 orphan engine", s.orphan_engine == 0);
    }

    // ── Engine extra event -> orphan_engine ──
    {
        MidiCapture r = loadCsvString("ppqn=480\n0,0x90,36,100\n");
        MidiCapture e = loadCsvString("ppqn=480\n0,0x90,36,100\n10,0x90,40,90\n");
        TimingSummary s = analyzeTiming(e, r, bpm);
        TEST("extra: matched=1", s.matched == 1);
        TEST("extra: 1 orphan engine", s.orphan_engine == 1);
    }

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

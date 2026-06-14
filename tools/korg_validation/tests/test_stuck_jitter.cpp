// Gate 11 Task E — stuck-note detector + jitter benchmark (synthetic).

#include "korg_validation/stuck_note_detector.h"
#include "korg_validation/jitter_benchmark.h"
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
    std::printf("Test: Gate 11 Task E — stuck-note + jitter\n");

    // ── Stuck-note: balanced stream ──
    {
        MidiCapture c = loadCsvString(
            "ppqn=480\n0,0x90,36,100\n240,0x80,36,0\n480,0x90,43,90\n720,0x80,43,0\n");
        StuckResult r = detectStuck(c);
        TEST("balanced: clean", r.balanced && r.hanging == 0);
        TEST("balanced: no double-on", r.double_on == 0);
        TEST("balanced: no orphan-off", r.orphan_off == 0);
    }
    // ── Stuck-note: missing NoteOff -> hanging ──
    {
        MidiCapture c = loadCsvString("ppqn=480\n0,0x90,36,100\n480,0x90,43,90\n720,0x80,43,0\n");
        StuckResult r = detectStuck(c);
        TEST("hanging: not balanced", !r.balanced);
        TEST("hanging: 1 hanging note (36)", r.hanging == 1 &&
             r.hanging_notes.size() == 1 && r.hanging_notes[0].second == 36);
    }
    // ── Stuck-note: double NoteOn + orphan NoteOff ──
    {
        MidiCapture c = loadCsvString(
            "ppqn=480\n0,0x90,36,100\n10,0x90,36,100\n20,0x80,36,0\n30,0x80,99,0\n");
        StuckResult r = detectStuck(c);
        TEST("double-on detected", r.double_on == 1);
        TEST("orphan-off detected", r.orphan_off == 1);
    }

    // ── Jitter: perfectly quantized -> ~0 ──
    {
        // 8th-note grid = 240 ticks. Notes exactly on grid.
        MidiCapture c = loadCsvString(
            "ppqn=480\n0,0x90,60,90\n240,0x90,60,90\n480,0x90,60,90\n720,0x90,60,90\n");
        JitterResult r = benchmarkJitter(c, 240, 120.0);
        TEST("jitter: 4 attacks", r.events == 4);
        TEST("jitter: quantized -> mean 0", near(r.mean_abs_ms, 0.0));
        TEST("jitter: quantized -> max 0", near(r.max_ms, 0.0));
    }
    // ── Jitter: detuned -> bounded non-zero ──
    {
        // each +10 ticks off the 240 grid; 10 ticks @120/480 = 10.4166..ms
        MidiCapture c = loadCsvString(
            "ppqn=480\n10,0x90,60,90\n250,0x90,60,90\n490,0x90,60,90\n");
        JitterResult r = benchmarkJitter(c, 240, 120.0);
        double tenTicksMs = tickToMs(10, 480, 120.0);
        TEST("jitter: detuned mean ~10 ticks", near(r.mean_abs_ms, tenTicksMs));
        TEST("jitter: detuned max ~10 ticks", near(r.max_ms, tenTicksMs));
    }

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

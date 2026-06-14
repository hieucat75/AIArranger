// Gate 10B Task B — swing / shuffle groove model.
//
// Unit tests for engine/music/groove::applySwing. Asserts off-beat 8th notes
// slide later by (swing% - 50%) * ticksPerBeat while on-beat notes stay put,
// across swing factors 0/50/67/75 (and the clamp ceiling).

#include "engine/music/groove.h"
#include <cstdio>

using namespace ai_arranger::music;

static int failures = 0;
static int passes = 0;

#define TEST_EQ(name, got, want) do { \
    unsigned long long g = (unsigned long long)(got), w = (unsigned long long)(want); \
    if (g != w) { std::fprintf(stderr, "  \xE2\x9D\x8C FAIL: %s (got %llu want %llu)\n", name, g, w); failures++; } \
    else { std::printf("  \xE2\x9C\x85 PASS: %s\n", name); passes++; } \
} while(0)

int main() {
    std::printf("Test: Gate 10B Task B — swing/shuffle groove\n");

    const uint32_t beat = 480;     // ticks per quarter
    const uint32_t eighth = 240;   // off-beat 8th position

    // ── Straight (50%) and below: no shift ──
    TEST_EQ("swing 50 on-beat unchanged",  applySwing(0,   beat, 50), 0);
    TEST_EQ("swing 50 off-beat unchanged", applySwing(eighth, beat, 50), eighth);
    TEST_EQ("swing 0 (straight) unchanged", applySwing(eighth, beat, 0), eighth);

    // ── Hard shuffle (75%): off-beat delayed by 120, on-beat unchanged ──
    // delay = (75-50)*480/100 = 120
    TEST_EQ("swing 75 on-beat (tick 0) unchanged",   applySwing(0,   beat, 75), 0);
    TEST_EQ("swing 75 on-beat (tick 480) unchanged", applySwing(480, beat, 75), 480);
    TEST_EQ("swing 75 off-beat (240 -> 360)",        applySwing(eighth, beat, 75), 360);
    TEST_EQ("swing 75 off-beat next beat (720 -> 840)", applySwing(720, beat, 75), 840);
    // A note before the 8th (on-beat half) is not shifted.
    TEST_EQ("swing 75 early-in-beat (120) unchanged", applySwing(120, beat, 75), 120);

    // ── Triplet shuffle (67%): delay = 17*480/100 = 81 ──
    TEST_EQ("swing 67 off-beat (240 -> 321)", applySwing(eighth, beat, 67), 321);
    TEST_EQ("swing 67 on-beat unchanged",     applySwing(0, beat, 67), 0);

    // ── Clamp: > 75 is capped at 75 (delay 120) ──
    TEST_EQ("swing 90 clamps to 75 (240 -> 360)", applySwing(eighth, beat, 90), 360);

    // ── Degenerate resolution is safe ──
    TEST_EQ("zero ticksPerBeat returns tick", applySwing(240, 0, 75), 240);

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

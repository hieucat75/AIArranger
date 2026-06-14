// Gate 12 Task F — split router tests (zones, manual bass, no collision).

#include "engine/performance/split_router.h"
#include <cstdio>
#include <vector>

using namespace ai_arranger::performance;

static int failures = 0, passes = 0;
#define TEST(name, expr) do { \
    if (!(expr)) { std::fprintf(stderr, "  \xE2\x9D\x8C FAIL: %s\n", name); failures++; } \
    else { std::printf("  \xE2\x9C\x85 PASS: %s\n", name); passes++; } \
} while(0)

int main() {
    std::printf("Test: Gate 12 Task F — split router\n");

    SplitRouter r;
    r.setSplitPoint(60);  // C4

    // ── Zone classification ──
    TEST("below split -> Lower", r.zoneOf(48) == Zone::Lower);
    TEST("just below split -> Lower", r.zoneOf(59) == Zone::Lower);
    TEST("split note -> Upper", r.zoneOf(60) == Zone::Upper);
    TEST("above split -> Upper", r.zoneOf(72) == Zone::Upper);

    // ── Configurable split point ──
    r.setSplitPoint(54);
    TEST("reconfig: 54 boundary, 53 Lower", r.zoneOf(53) == Zone::Lower);
    TEST("reconfig: 54 Upper", r.zoneOf(54) == Zone::Upper);
    r.setSplitPoint(60);

    // ── Manual bass override ──
    {
        std::vector<uint8_t> lower = {43, 48, 52};  // ascending lower-zone notes
        TEST("manual bass off -> none",
             r.manualBassNote(lower.data(), lower.size()) == kNoManualBass);
        r.setManualBass(true);
        TEST("manual bass on -> lowest lower note (43)",
             r.manualBassNote(lower.data(), lower.size()) == 43);
        TEST("manual bass on, empty -> none",
             r.manualBassNote(nullptr, 0) == kNoManualBass);
        r.setManualBass(false);
    }

    // ── No collision: an upper-zone note is never a chord/bass source ──
    {
        // Caller filters by zone; verify a melody note classifies Upper so it is
        // excluded from chord/bass routing.
        TEST("upper note excluded from lower routing", r.zoneOf(76) == Zone::Upper);
    }

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

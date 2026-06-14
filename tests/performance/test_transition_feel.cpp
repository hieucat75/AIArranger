// Gate 15 Task D — transition feel / pre-roll viz (deterministic).

#include "performance/telemetry/transition_state.h"
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
    std::printf("Test: Gate 15 Task D — transition feel\n");
    const int64_t bar = 1920;

    // ── Bar progress + ticks-to-fire ──
    {
        TransitionFeel f;
        auto v = f.update(0, bar);
        TEST("bar progress 0 at boundary", near(v.bar_progress, 0.0));
        TEST("ticks-to-fire = bar at start", v.ticks_to_fire == 1920);
        v = f.update(960, bar);
        TEST("mid-bar progress 0.5", near(v.bar_progress, 0.5));
        TEST("mid-bar ticks-to-fire 960", v.ticks_to_fire == 960);
        TEST("not pending -> no arming", !v.pending && !v.arming);
    }

    // ── Pending + pre-roll arming + fire at boundary ──
    {
        TransitionFeel f;
        f.setPreRollTicks(240);
        f.update(0, bar);            // establish last_bar at 0
        f.queue(TransitionKind::Fill);
        auto early = f.update(960, bar);   // mid-bar, far from boundary
        TEST("pending Fill", early.pending && early.kind == TransitionKind::Fill);
        TEST("not arming far from boundary", !early.arming);
        auto armed = f.update(1800, bar);  // 120 ticks to boundary <= 240 preroll
        TEST("arming within pre-roll", armed.arming && armed.ticks_to_fire == 120);
        auto fired = f.update(1920, bar);  // cross into bar 1
        TEST("fires at boundary", fired.pending && fired.ticks_to_fire == 0 &&
             fired.kind == TransitionKind::Fill);
        TEST("pending cleared after fire", !f.isPending());
    }

    // ── Variation kind + clear ──
    {
        TransitionFeel f;
        f.update(0, bar);
        f.queue(TransitionKind::Variation);
        TEST("variation pending", f.isPending());
        f.clear();
        TEST("clear cancels pending", !f.isPending());
        auto v = f.update(500, bar);
        TEST("cleared -> kind None", v.kind == TransitionKind::None && !v.pending);
    }

    // ── Determinism: same inputs -> same viz ──
    {
        TransitionFeel a, b;
        a.update(0, bar); b.update(0, bar);
        a.queue(TransitionKind::Fill); b.queue(TransitionKind::Fill);
        auto va = a.update(1800, bar); auto vb = b.update(1800, bar);
        TEST("deterministic viz", va.arming == vb.arming &&
             va.ticks_to_fire == vb.ticks_to_fire && va.kind == vb.kind);
    }

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

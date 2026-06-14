// Gate 12 Task C — fill scheduler tests (collapse, single-fire, no skip).

#include "engine/performance/fill_scheduler.h"
#include <atomic>
#include <cstdio>
#include <thread>
#include <vector>

using namespace ai_arranger::performance;

static int failures = 0, passes = 0;
#define TEST(name, expr) do { \
    if (!(expr)) { std::fprintf(stderr, "  \xE2\x9D\x8C FAIL: %s\n", name); failures++; } \
    else { std::printf("  \xE2\x9C\x85 PASS: %s\n", name); passes++; } \
} while(0)

int main() {
    std::printf("Test: Gate 12 Task C — fill scheduler\n");

    // ── Basic queue + fire ──
    {
        FillScheduler f;
        TEST("idle: nothing pending", !f.hasPending());
        f.queueFill();
        TEST("queued: pending", f.hasPending());
        FillFire a = f.onBarBoundary();
        TEST("boundary fires once", a.fired && a.target_variation == kFillReturn);
        FillFire b = f.onBarBoundary();
        TEST("next boundary: nothing", !b.fired && !f.hasPending());
    }

    // ── Rapid spam collapses to one fill ──
    {
        FillScheduler f;
        for (int i = 0; i < 500; ++i) f.queueFill();
        int fires = 0;
        for (int i = 0; i < 5; ++i) if (f.onBarBoundary().fired) ++fires;
        TEST("spam 500 -> exactly one fill fires", fires == 1);
    }

    // ── Last-queued target wins (no skipped variation) ──
    {
        FillScheduler f;
        f.queueFill(1);
        f.queueFill(3);   // re-target before the boundary
        FillFire a = f.onBarBoundary();
        TEST("target = last queued (3)", a.fired && a.target_variation == 3);
    }

    // ── Concurrent spam: at most one fire per boundary ──
    {
        FillScheduler f;
        std::atomic<bool> go{false};
        std::vector<std::thread> ts;
        for (int i = 0; i < 8; ++i)
            ts.emplace_back([&] {
                while (!go.load()) {}
                for (int k = 0; k < 500; ++k) f.queueFill(2);
            });
        go.store(true);
        for (auto& t : ts) t.join();
        FillFire a = f.onBarBoundary();
        FillFire b = f.onBarBoundary();
        TEST("concurrent: one fire then none",
             a.fired && a.target_variation == 2 && !b.fired);
    }

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

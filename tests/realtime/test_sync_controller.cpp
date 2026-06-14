// Gate 12 Task B — sync controller tests (single-fire, race-safe).

#include "engine/performance/sync_controller.h"
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
    std::printf("Test: Gate 12 Task B — sync controller\n");

    // ── Arm then chord starts once ──
    {
        SyncController c;
        TEST("arm -> Armed", c.arm() && c.isArmed());
        TEST("first chord fires start", c.onChordPresent() && c.isPlaying());
        TEST("second chord does NOT re-fire", !c.onChordPresent() && c.isPlaying());
    }

    // ── Rapid single-thread chord spam fires exactly once ──
    {
        SyncController c; c.arm();
        int fired = 0;
        for (int i = 0; i < 1000; ++i) if (c.onChordPresent()) ++fired;
        TEST("rapid spam: start fired exactly once", fired == 1);
        TEST("rapid spam: ends Playing", c.isPlaying());
    }

    // ── Stop path ──
    {
        SyncController c; c.arm(); c.onChordPresent();
        TEST("requestStop -> Stopping", c.requestStop() &&
             c.state() == PerformerState::Stopping);
        TEST("notifyStopped -> Idle", c.notifyStopped() &&
             c.state() == PerformerState::Idle);
    }

    // ── Concurrent chord race: exactly one thread fires start ──
    {
        SyncController c; c.arm();
        std::atomic<int> fired{0};
        std::atomic<bool> go{false};
        std::vector<std::thread> ts;
        for (int i = 0; i < 8; ++i) {
            ts.emplace_back([&] {
                while (!go.load()) { /* spin to maximize overlap */ }
                for (int k = 0; k < 200; ++k)
                    if (c.onChordPresent()) fired.fetch_add(1);
            });
        }
        go.store(true);
        for (auto& t : ts) t.join();
        TEST("concurrent race: start fired exactly once", fired.load() == 1);
        TEST("concurrent race: ends Playing", c.isPlaying());
    }

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

// Gate 12 Task G — control surface abstraction (SPSC queue + fan-out).

#include "engine/control/control_surface.h"
#include "engine/control/control_events.h"
#include "engine/performance/sync_controller.h"
#include "engine/performance/fill_scheduler.h"
#include "engine/performance/variation_manager.h"
#include <atomic>
#include <cstdio>
#include <thread>

using namespace ai_arranger::control;
namespace perf = ai_arranger::performance;

static int failures = 0, passes = 0;
#define TEST(name, expr) do { \
    if (!(expr)) { std::fprintf(stderr, "  \xE2\x9D\x8C FAIL: %s\n", name); failures++; } \
    else { std::printf("  \xE2\x9C\x85 PASS: %s\n", name); passes++; } \
} while(0)

// A synthetic front-end: scripts events into the queue, polls them out.
struct SyntheticSurface final : IControlSurface {
    ControlEventQueue<256> q;
    bool poll(ControlEvent& out) noexcept override { return q.pop(out); }
    void script(ControlAction a, int32_t p = 0) { q.push({a, p, 0}); }
};

int main() {
    std::printf("Test: Gate 12 Task G — control surface\n");

    // ── FIFO order + empty ──
    {
        SyntheticSurface s;
        s.script(ControlAction::SyncArm);
        s.script(ControlAction::Fill, 2);
        ControlEvent e;
        TEST("poll #1 = SyncArm", s.poll(e) && e.action == ControlAction::SyncArm);
        TEST("poll #2 = Fill param 2",
             s.poll(e) && e.action == ControlAction::Fill && e.param == 2);
        TEST("poll empty -> false", !s.poll(e));
    }

    // ── Fan-out: events drive the performer modules identically ──
    {
        SyntheticSurface s;
        perf::SyncController sync;
        perf::FillScheduler fill;
        perf::VariationManager var;

        s.script(ControlAction::SyncArm);
        s.script(ControlAction::Start);
        s.script(ControlAction::Fill);
        s.script(ControlAction::VariationC);

        ControlEvent e;
        while (s.poll(e)) {
            switch (e.action) {
                case ControlAction::SyncArm:    sync.arm(); break;
                case ControlAction::Start:      sync.start(); break;
                case ControlAction::Fill:       fill.queueFill(); break;
                case ControlAction::VariationC: var.queue(perf::Variation::C); break;
                default: break;
            }
        }
        TEST("fan-out: sync playing", sync.isPlaying());
        TEST("fan-out: fill pending", fill.hasPending());
        TEST("fan-out: variation pending C",
             var.hasPending() && var.pending() == perf::Variation::C);
    }

    // ── Lock-free SPSC across threads (producer/consumer) ──
    {
        ControlEventQueue<1024> q;
        const int N = 5000;
        std::atomic<int> consumed{0};
        std::thread prod([&] {
            for (int i = 0; i < N; ++i)
                while (!q.push({ControlAction::Tap, i, 0})) { /* spin if full */ }
        });
        std::thread cons([&] {
            ControlEvent e; int got = 0, expect = 0; bool ordered = true;
            while (got < N) {
                if (q.pop(e)) {
                    if (e.param != expect) ordered = false;
                    ++expect; ++got;
                }
            }
            if (ordered) consumed.store(got);
        });
        prod.join(); cons.join();
        TEST("SPSC: all events consumed in order", consumed.load() == N);
    }

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

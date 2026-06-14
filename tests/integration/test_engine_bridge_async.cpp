// Gate 14 Task C — async UI<->engine bridge (lock-free SPSC, headless).

#include "control/engine_bridge.h"
#include "engine/control/control_events.h"
#include <atomic>
#include <cstdio>
#include <thread>

using namespace ai_arranger::control;

static int failures = 0, passes = 0;
#define TEST(name, expr) do { \
    if (!(expr)) { std::fprintf(stderr, "  \xE2\x9D\x8C FAIL: %s\n", name); failures++; } \
    else { std::printf("  \xE2\x9C\x85 PASS: %s\n", name); passes++; } \
} while(0)

int main() {
    std::printf("Test: Gate 14 Task C — engine bridge async\n");

    // ── Control round-trip + FIFO ──
    {
        EngineBridge b;
        b.sendControl({ControlAction::Start, 0, 0});
        b.sendControl({ControlAction::Fill, 7, 0});
        ControlEvent e;
        TEST("engine reads #1 Start", b.nextControl(e) && e.action == ControlAction::Start);
        TEST("engine reads #2 Fill param 7",
             b.nextControl(e) && e.action == ControlAction::Fill && e.param == 7);
        TEST("queue drained", !b.nextControl(e));
    }

    // ── Telemetry round-trip ──
    {
        EngineBridge b;
        EngineTelemetry t; t.playing = true; t.section = 2; t.chord_root = 65;
        b.publishTelemetry(t);
        EngineTelemetry got;
        TEST("UI polls telemetry", b.pollTelemetry(got) && got.playing &&
             got.section == 2 && got.chord_root == 65);
        TEST("telemetry drained", !b.pollTelemetry(got));
    }

    // ── Bounded: overflow returns false (no alloc, no block) ──
    {
        EngineBridge b;
        int pushed = 0;
        for (int i = 0; i < 5000; ++i)
            if (b.sendControl({ControlAction::Tap, i, 0})) ++pushed;
        TEST("control queue bounded", pushed == (int)EngineBridge::controlCapacity());
    }

    // ── Concurrent flood: UI producer, engine consumer, in order ──
    {
        EngineBridge b;
        const int N = 100000;
        std::atomic<bool> go{false};
        std::thread ui([&] {
            while (!go.load()) {}
            for (int i = 0; i < N; ++i)
                while (!b.sendControl({ControlAction::Tap, i, 0})) { /* spin if full */ }
        });
        int got = 0, expect = 0; bool ordered = true;
        go.store(true);
        ControlEvent e;
        while (got < N) {
            if (b.nextControl(e)) {
                if (e.param != expect) ordered = false;
                ++expect; ++got;
            }
        }
        ui.join();
        TEST("100k events delivered in order, no loss", got == N && ordered);
    }

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

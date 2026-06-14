// Gate 14 Task B — engine session lifecycle (headless).

#include "session/engine_session.h"
#include "engine/control/control_events.h"
#include <cstdio>

using namespace ai_arranger;
using namespace ai_arranger::session;
using control::ControlAction;

static int failures = 0, passes = 0;
#define TEST(name, expr) do { \
    if (!(expr)) { std::fprintf(stderr, "  \xE2\x9D\x8C FAIL: %s\n", name); failures++; } \
    else { std::printf("  \xE2\x9C\x85 PASS: %s\n", name); passes++; } \
} while(0)

static void run(EngineSession& s, int n) {
    for (int i = 0; i < n; ++i) {
        s.tick();
        s.clock().advance(2048);
        s.scheduler().advanceTo(s.clock().getPosition());
    }
}

int main() {
    std::printf("Test: Gate 14 Task B — engine session lifecycle\n");

    // ── Boot / idempotent boot ──
    {
        EngineSession s;
        TEST("not booted initially", !s.isBooted());
        TEST("boot succeeds", s.boot() && s.isBooted());
        TEST("boot is idempotent", s.boot() && s.isBooted());
    }

    // ── Boot -> play -> shutdown leaves no hanging notes ──
    {
        EngineSession s; s.boot();
        s.adapter().postEvent({ControlAction::Start, 0, 0});
        run(s, 30);
        TEST("playing after Start", s.player().isPlaying());
        s.shutdown();
        TEST("not booted after shutdown", !s.isBooted());
        TEST("no hanging notes after shutdown", !s.panic().hasActiveNotes());
        TEST("not playing after shutdown", !s.player().isPlaying());
    }

    // ── Repeated boot/shutdown cycles are stable ──
    {
        EngineSession s;
        bool ok = true;
        for (int i = 0; i < 50; ++i) {
            s.boot();
            s.adapter().postEvent({ControlAction::Start, 0, 0});
            run(s, 5);
            s.shutdown();
            if (s.isBooted() || s.panic().hasActiveNotes()) ok = false;
        }
        TEST("50x boot/shutdown stable, no leaks", ok);
    }

    // ── Panic during playback, then reset ──
    {
        EngineSession s; s.boot();
        s.adapter().postEvent({ControlAction::Start, 0, 0});
        run(s, 10);
        s.adapter().postEvent({ControlAction::Panic, 0, 0});
        run(s, 3);
        TEST("panic clears hanging notes", !s.panic().hasActiveNotes());
        s.reset();
        TEST("reset re-boots cleanly", s.isBooted());
        s.adapter().postEvent({ControlAction::Start, 0, 0});
        run(s, 10);
        TEST("plays again after reset", s.player().isPlaying());
        s.shutdown();
    }

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

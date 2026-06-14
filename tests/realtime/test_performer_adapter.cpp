// Gate 13 Task A — performer adapter skeleton (control routing into StylePlayer).

#include "engine/integration/performer_adapter.h"
#include "engine/arranger/style_player.h"
#include "engine/realtime/clock.h"
#include "engine/midi/scheduler.h"
#include "engine/midi/panic.h"
#include "engine/control/control_events.h"
#include <cstdio>

using namespace ai_arranger;
using namespace ai_arranger::integration;
using control::ControlAction;
using control::ControlEvent;

static int failures = 0, passes = 0;
#define TEST(name, expr) do { \
    if (!(expr)) { std::fprintf(stderr, "  \xE2\x9D\x8C FAIL: %s\n", name); failures++; } \
    else { std::printf("  \xE2\x9C\x85 PASS: %s\n", name); passes++; } \
} while(0)

struct Rig {
    realtime::RealtimeClock clock;
    midi::MidiScheduler scheduler;
    midi::PanicHandler panic;
    arranger::StylePlayer player{clock, scheduler, panic};
    PerformerAdapter adapter{clock, player};
    Rig() {
        player.loadStyle(arranger::buildDemoStyle());
        clock.setSampleRate(48000);
        clock.setResolution(480);
        clock.setTempo(120);
    }
    void run(int iters) {
        for (int i = 0; i < iters; ++i) {
            adapter.tick();
            clock.advance(2048);
            scheduler.advanceTo(clock.getPosition());
        }
    }
};

int main() {
    std::printf("Test: Gate 13 Task A — performer adapter\n");

    // ── Start event drives StylePlayer ──
    {
        Rig r;
        r.adapter.postEvent({ControlAction::Start, 0, 0});
        r.run(20);
        TEST("Start event -> player playing", r.adapter.isPlaying());
    }

    // ── Stop event stops playback ──
    {
        Rig r;
        r.adapter.postEvent({ControlAction::Start, 0, 0});
        r.run(10);
        r.adapter.postEvent({ControlAction::Stop, 0, 0});
        r.run(5);
        TEST("Stop event -> not playing", !r.adapter.isPlaying());
        TEST("Stop -> state Idle", r.adapter.state() == performance::PerformerState::Idle);
    }

    // ── Queue drains in tick (event before any tick still applies) ──
    {
        Rig r;
        TEST("not playing before tick", !r.adapter.isPlaying());
        r.adapter.postEvent({ControlAction::Start, 0, 0});
        // not yet ticked
        TEST("event queued, not yet applied", !r.adapter.isPlaying());
        r.run(5);
        TEST("after tick: applied", r.adapter.isPlaying());
    }

    // ── Full queue returns false (bounded, no alloc) ──
    {
        Rig r;
        int rejected = 0;
        for (int i = 0; i < 1000; ++i)
            if (!r.adapter.postEvent({ControlAction::Tap, i, 0})) ++rejected;
        TEST("bounded queue rejects overflow", rejected > 0);
    }

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

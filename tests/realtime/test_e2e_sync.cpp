// Gate 13 Task B — SyncController <-> StylePlayer end-to-end.

#include "engine/integration/performer_adapter.h"
#include "engine/arranger/style_player.h"
#include "engine/realtime/clock.h"
#include "engine/midi/scheduler.h"
#include "engine/midi/panic.h"
#include "engine/chord/fingered_detector.h"
#include "engine/control/control_events.h"
#include <cstdio>

using namespace ai_arranger;
using namespace ai_arranger::integration;
using control::ControlAction;

static int failures = 0, passes = 0;
#define TEST(name, expr) do { \
    if (!(expr)) { std::fprintf(stderr, "  \xE2\x9D\x8C FAIL: %s\n", name); failures++; } \
    else { std::printf("  \xE2\x9C\x85 PASS: %s\n", name); passes++; } \
} while(0)

struct Rig {
    realtime::RealtimeClock clock; midi::MidiScheduler scheduler; midi::PanicHandler panic;
    arranger::StylePlayer player{clock, scheduler, panic};
    PerformerAdapter adapter{clock, player};
    chord::FingeredDetector fingered;
    Rig() {
        player.loadStyle(arranger::buildDemoStyle());
        clock.setSampleRate(48000); clock.setResolution(480); clock.setTempo(120);
        adapter.setChordScanMode(&fingered);
        adapter.setSplitPoint(127); // whole keyboard is lower zone for the test
    }
    void run(int n){ for(int i=0;i<n;++i){adapter.tick();clock.advance(2048);scheduler.advanceTo(clock.getPosition());} }
};

int main() {
    std::printf("Test: Gate 13 Task B — sync E2E\n");

    // ── Arm-before-play: first chord auto-starts ──
    {
        Rig r;
        r.adapter.postEvent({ControlAction::SyncArm, 0, 0});
        r.run(3);
        TEST("armed, not yet playing", !r.adapter.isPlaying() &&
             r.adapter.sync().isArmed());
        r.adapter.noteOn(60); r.adapter.noteOn(64); r.adapter.noteOn(67); // C major
        r.run(5);
        TEST("first chord auto-starts playback", r.adapter.isPlaying());
        TEST("sync state Playing", r.adapter.sync().isPlaying());
    }

    // ── No double-trigger: more chord notes don't re-start ──
    {
        Rig r;
        r.adapter.postEvent({ControlAction::SyncArm, 0, 0});
        r.run(2);
        r.adapter.noteOn(60); r.adapter.noteOn(64); r.adapter.noteOn(67);
        r.run(3);
        bool playing1 = r.adapter.isPlaying();
        r.adapter.noteOn(72); // another note while already playing
        r.run(3);
        TEST("still playing, no re-trigger", playing1 && r.adapter.isPlaying());
    }

    // ── Explicit Start also works ──
    {
        Rig r;
        r.adapter.postEvent({ControlAction::Start, 0, 0});
        r.run(5);
        TEST("explicit Start plays", r.adapter.isPlaying());
    }

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

// Gate 13 Task G — control surface E2E (keystroke -> ControlEvent -> adapter ->
// StylePlayer). Headless: no CoreMIDI, scripted key sequence.

#include "engine/integration/performer_adapter.h"
#include "engine/arranger/style_player.h"
#include "engine/realtime/clock.h"
#include "engine/midi/scheduler.h"
#include "engine/midi/panic.h"
#include "engine/control/key_bindings.h"
#include <cstdio>
#include <string>

using namespace ai_arranger;
using namespace ai_arranger::integration;

static int failures = 0, passes = 0;
#define TEST(name, expr) do { \
    if (!(expr)) { std::fprintf(stderr, "  \xE2\x9D\x8C FAIL: %s\n", name); failures++; } \
    else { std::printf("  \xE2\x9C\x85 PASS: %s\n", name); passes++; } \
} while(0)

struct Rig {
    realtime::RealtimeClock clock; midi::MidiScheduler scheduler; midi::PanicHandler panic;
    arranger::StylePlayer player{clock, scheduler, panic};
    PerformerAdapter adapter{clock, player};
    Rig() {
        player.loadStyle(arranger::buildDemoStyle());
        clock.setSampleRate(48000); clock.setResolution(480); clock.setTempo(120);
        adapter.setVariationSections(0,1,2,3);
    }
    void key(char k) {
        auto ev = control::keyToControlEvent(k);
        if (ev.action != control::ControlAction::None) adapter.postEvent(ev);
    }
    void step(int n){ for(int i=0;i<n;++i){adapter.tick();clock.advance(256);scheduler.advanceTo(clock.getPosition());} }
};

int main() {
    std::printf("Test: Gate 13 Task G — control surface E2E (keys)\n");

    // ── Key map is correct ──
    using control::keyToControlEvent;
    using control::ControlAction;
    TEST("'s' -> Start", keyToControlEvent('s').action == ControlAction::Start);
    TEST("'f' -> Fill", keyToControlEvent('f').action == ControlAction::Fill);
    TEST("'3' -> VariationC", keyToControlEvent('3').action == ControlAction::VariationC);
    TEST("'p' -> Panic", keyToControlEvent('p').action == ControlAction::Panic);
    TEST("unmapped -> None", keyToControlEvent('z').action == ControlAction::None);

    // ── Scripted key sequence drives playback end-to-end ──
    {
        Rig r;
        r.key('s');               // start
        r.step(10);
        TEST("key 's' starts playback", r.adapter.isPlaying());
        r.key('x');               // stop
        r.step(5);
        TEST("key 'x' stops playback", !r.adapter.isPlaying());
    }

    // ── Panic key recovers + re-arm via keys ──
    {
        Rig r;
        r.key('s'); r.step(8);
        r.key('p'); r.step(3);    // panic
        TEST("key 'p' -> Idle", r.adapter.state() == performance::PerformerState::Idle);
        r.key('s'); r.step(5);    // restart without reconstructing
        TEST("re-start after panic via key", r.adapter.isPlaying());
    }

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

// Gate 13 Task D — VariationManager <-> sequencer end-to-end (bar-boundary).

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
        player.loadStyle(arranger::buildDemoStyle());  // [Intro,Main,Fill,Ending]
        clock.setSampleRate(48000); clock.setResolution(480); clock.setTempo(120);
        adapter.setVariationSections(0, 1, 2, 3);
    }
    void step(int n){ for(int i=0;i<n;++i){adapter.tick();clock.advance(256);scheduler.advanceTo(clock.getPosition());} }
    bool reachesSection(int idx, int maxIters) {
        for (int i = 0; i < maxIters; ++i) {
            adapter.tick(); clock.advance(256); scheduler.advanceTo(clock.getPosition());
            if (player.getCurrentSection() == idx) return true;
        }
        return false;
    }
};

int main() {
    std::printf("Test: Gate 13 Task D — variation E2E\n");

    // ── Variation event switches section at boundary ──
    {
        Rig r;
        r.adapter.postEvent({ControlAction::Start, 0, 0});
        r.step(10);
        r.adapter.postEvent({ControlAction::VariationC, 0, 0});  // -> section 2
        TEST("VariationC reaches section 2", r.reachesSection(2, 2000));
    }

    // ── No abrupt mid-bar cut: section unchanged until a boundary ──
    {
        Rig r;
        r.adapter.postEvent({ControlAction::Start, 0, 0});
        r.step(10);
        int before = r.player.getCurrentSection();
        r.adapter.postEvent({ControlAction::VariationD, 0, 0});  // -> section 3
        r.adapter.tick(); r.clock.advance(64); r.scheduler.advanceTo(r.clock.getPosition());
        TEST("no immediate mid-bar switch", r.player.getCurrentSection() == before);
        TEST("switches by next boundary", r.reachesSection(3, 2000));
    }

    // ── Last-queued variation wins ──
    {
        Rig r;
        r.adapter.postEvent({ControlAction::Start, 0, 0});
        r.step(10);
        r.adapter.postEvent({ControlAction::VariationB, 0, 0});  // section 1
        r.adapter.postEvent({ControlAction::VariationD, 0, 0});  // section 3 (last)
        TEST("last variation (D->section 3) wins", r.reachesSection(3, 2000));
    }

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

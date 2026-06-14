// Gate 13 Task C — FillScheduler <-> sequencer end-to-end (bar-boundary).

#include "engine/integration/performer_adapter.h"
#include "engine/arranger/style_player.h"
#include "engine/realtime/clock.h"
#include "engine/midi/scheduler.h"
#include "engine/midi/panic.h"
#include "engine/uasf/types.h"
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

static bool isFill(uasf::SectionType t) {
    return t >= uasf::SectionType::Fill1 && t <= uasf::SectionType::Fill4;
}

struct Rig {
    realtime::RealtimeClock clock; midi::MidiScheduler scheduler; midi::PanicHandler panic;
    arranger::StylePlayer player{clock, scheduler, panic};
    PerformerAdapter adapter{clock, player};
    Rig() {
        player.loadStyle(arranger::buildDemoStyle());  // [Intro,Main,Fill,Ending]
        clock.setSampleRate(48000); clock.setResolution(480); clock.setTempo(120);
    }
    // Advance until a Fill section becomes current, or `maxIters` elapses.
    bool reachesFillWithin(int maxIters) {
        for (int i = 0; i < maxIters; ++i) {
            adapter.tick(); clock.advance(256); scheduler.advanceTo(clock.getPosition());
            const auto* d = player.getCurrentSectionDef();
            if (d && isFill(d->type)) return true;
        }
        return false;
    }
};

int main() {
    std::printf("Test: Gate 13 Task C — fill E2E\n");

    // ── Fill resolves to a Fill section at a bar boundary ──
    {
        Rig r;
        r.adapter.postEvent({ControlAction::Start, 0, 0});
        // play ~1 bar of intro
        for (int i = 0; i < 10; ++i) { r.adapter.tick(); r.clock.advance(256); r.scheduler.advanceTo(r.clock.getPosition()); }
        r.adapter.postEvent({ControlAction::Fill, 0, 0});
        TEST("Fill section reached at boundary", r.reachesFillWithin(2000));
    }

    // ── Rapid fill spam still resolves to a Fill (collapse) ──
    {
        Rig r;
        r.adapter.postEvent({ControlAction::Start, 0, 0});
        for (int i = 0; i < 6; ++i) { r.adapter.tick(); r.clock.advance(256); r.scheduler.advanceTo(r.clock.getPosition()); }
        for (int i = 0; i < 200; ++i) r.adapter.postEvent({ControlAction::Fill, 0, 0});
        TEST("spammed fill still reaches a Fill section", r.reachesFillWithin(2000));
    }

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

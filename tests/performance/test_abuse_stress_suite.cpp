// Gate 15 Task F — stability under abuse (headless stress). Asserts no crash /
// deadlock / stuck-note / runaway state under rapid spam, panic, hot-plug, chord
// floods, and sync abuse. Bounded loops => completes fast (no runaway CPU).

#include "session/engine_session.h"
#include "control/midi_output_bridge.h"
#include "control/engine_midi_route.h"
#include "midi/midi_output_manager.h"
#include "engine/control/control_events.h"
#include "../midi/fake_midi_output_provider.h"
#include <cstdio>

using namespace ai_arranger;
using namespace ai_arranger::control;
using namespace ai_arranger::midi;

static int failures = 0, passes = 0;
#define TEST(name, expr) do { \
    if (!(expr)) { std::fprintf(stderr, "  \xE2\x9D\x8C FAIL: %s\n", name); failures++; } \
    else { std::printf("  \xE2\x9C\x85 PASS: %s\n", name); passes++; } \
} while(0)

struct Rig {
    session::EngineSession session;
    MidiOutputBridge bridge;
    FakeMidiOutputProvider provider;
    MidiOutputManager manager{provider};
    Rig() {
        session.boot();
        attachEngineOutput(session.scheduler(), bridge);
        provider.setDevices({{0,"IAC Bus 1"}}); manager.refresh(); manager.selectDevice(0);
    }
    void tickN(int n) {
        for (int i = 0; i < n; ++i) {
            session.tick(); session.clock().advance(2048);
            session.scheduler().advanceTo(session.clock().getPosition());
            pumpEngineOutput(bridge, provider);
        }
    }
};

int main() {
    std::printf("Test: Gate 15 Task F — abuse stress suite\n");

    // ── 1. Rapid fill spam ──
    {
        Rig r; r.session.adapter().postEvent({ControlAction::Start,0,0});
        for (int i = 0; i < 20000; ++i) r.session.adapter().postEvent({ControlAction::Fill,0,0});
        r.tickN(200);
        TEST("fill spam: engine alive + playing", r.session.player().isPlaying());
    }

    // ── 2. Variation spam (interleaved) ──
    {
        Rig r; r.session.adapter().postEvent({ControlAction::Start,0,0});
        for (int i = 0; i < 20000; ++i) {
            auto a = (ControlAction)((int)ControlAction::VariationA + (i % 4));
            r.session.adapter().postEvent({a,0,0});
        }
        r.tickN(200);
        TEST("variation spam: valid section", r.session.player().getCurrentSection() >= 0);
    }

    // ── 3. Repeated panic during playback ──
    {
        Rig r; r.session.adapter().postEvent({ControlAction::Start,0,0});
        r.tickN(20);
        for (int i = 0; i < 2000; ++i) r.session.adapter().postEvent({ControlAction::Panic,0,0});
        r.tickN(50);
        TEST("panic spam: no hanging notes", !r.session.panic().hasActiveNotes());
        // Re-arm after the storm.
        r.session.adapter().postEvent({ControlAction::Start,0,0});
        r.tickN(20);
        TEST("re-start after panic storm", r.session.player().isPlaying());
    }

    // ── 4. Hot-plug churn during playback ──
    {
        Rig r; r.session.adapter().postEvent({ControlAction::Start,0,0});
        for (int i = 0; i < 2000; ++i) {
            if (i % 2) { r.provider.disconnect(); r.provider.setDevices({}); }
            else       { r.provider.setDevices({{0,"IAC Bus 1"}}); r.provider.reconnect(); }
            r.manager.notifyHotPlug();
            if (i % 100 == 0) r.tickN(1);
        }
        r.tickN(20);
        TEST("hot-plug churn: engine survives", r.session.player().isPlaying());
    }

    // ── 5. Chord flood ──
    {
        Rig r; r.session.adapter().postEvent({ControlAction::Start,0,0});
        for (int i = 0; i < 10000; ++i) {
            int32_t param = (60 + (i % 12)) | ((int)arranger::ChordType::Major << 8);
            r.session.adapter().postEvent({ControlAction::Chord, param, 0});
        }
        r.tickN(100);
        TEST("chord flood: engine alive", r.session.player().isPlaying());
    }

    // ── 6. Sync abuse: many arms + chords -> still coherent ──
    {
        Rig r;
        for (int i = 0; i < 5000; ++i) {
            r.session.adapter().postEvent({ControlAction::SyncArm,0,0});
            int32_t param = 60 | ((int)arranger::ChordType::Major << 8);
            r.session.adapter().postEvent({ControlAction::Chord, param, 0});
        }
        r.tickN(50);
        TEST("sync abuse: ends playing (coherent)", r.session.player().isPlaying());
    }

    // ── 7. After everything: clean stop, no stuck notes ──
    {
        Rig r; r.session.adapter().postEvent({ControlAction::Start,0,0});
        r.tickN(30);
        r.session.adapter().postEvent({ControlAction::Stop,0,0});
        r.tickN(10);
        TEST("clean stop: no stuck notes", !r.session.panic().hasActiveNotes());
    }

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

// Gate 14C Task D — engine -> bridge -> provider wiring (headless E2E).

#include "control/engine_midi_route.h"
#include "control/midi_output_bridge.h"
#include "session/engine_session.h"
#include "fake_midi_output_provider.h"
#include "engine/control/control_events.h"
#include "engine/uasf/types.h"
#include <cstdio>

using namespace ai_arranger;
using namespace ai_arranger::control;
using namespace ai_arranger::midi;

static int failures = 0, passes = 0;
#define TEST(name, expr) do { \
    if (!(expr)) { std::fprintf(stderr, "  \xE2\x9D\x8C FAIL: %s\n", name); failures++; } \
    else { std::printf("  \xE2\x9C\x85 PASS: %s\n", name); passes++; } \
} while(0)

int main() {
    std::printf("Test: Gate 14C Task D — engine MIDI route\n");

    session::EngineSession s; s.boot();
    MidiOutputBridge bridge;
    FakeMidiOutputProvider provider;
    provider.setDevices({{0, "IAC Bus 1"}}); provider.select(0);

    // Wire engine scheduler output -> bridge (no CoreMIDI in the engine path).
    attachEngineOutput(s.scheduler(), bridge);

    // Play, pumping the bridge -> provider each tick (dispatch side).
    s.adapter().postEvent({control::ControlAction::Start, 0, 0});
    int noteEvents = 0;
    for (int i = 0; i < 200; ++i) {
        s.tick();
        s.clock().advance(2048);
        s.scheduler().advanceTo(s.clock().getPosition());
        pumpEngineOutput(bridge, provider);
    }
    for (const auto& e : provider.sent)
        if (e.type == uasf::MidiEventType::NoteOn || e.type == uasf::MidiEventType::NoteOff)
            ++noteEvents;

    TEST("engine output reached provider", !provider.sent.empty());
    TEST("note events delivered to provider", noteEvents > 0);
    TEST("bridge forwarded == provider dispatched",
         bridge.forwarded() == provider.dispatchedCount());

    // No live destination -> events still flow through bridge but provider no-ops
    // (engine keeps running; nothing crashes).
    {
        session::EngineSession s2; s2.boot();
        MidiOutputBridge b2;
        FakeMidiOutputProvider p2;   // no device selected -> not live
        attachEngineOutput(s2.scheduler(), b2);
        s2.adapter().postEvent({control::ControlAction::Start, 0, 0});
        for (int i = 0; i < 50; ++i) {
            s2.tick(); s2.clock().advance(2048);
            s2.scheduler().advanceTo(s2.clock().getPosition());
            pumpEngineOutput(b2, p2);
        }
        TEST("no-device: provider sent nothing", p2.sent.empty());
        TEST("no-device: engine still playing (no crash)", s2.player().isPlaying());
        TEST("no-device: bridge still forwarded events", b2.forwarded() > 0);
        s2.shutdown();
    }

    s.shutdown();
    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

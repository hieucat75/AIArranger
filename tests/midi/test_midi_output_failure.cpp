// Gate 14C Task G — failure handling: disconnect/reconnect during playback.

#include "session/engine_session.h"
#include "control/midi_output_bridge.h"
#include "control/engine_midi_route.h"
#include "midi/midi_output_manager.h"
#include "fake_midi_output_provider.h"
#include "engine/control/control_events.h"
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
    }
    void play(int n) {
        for (int i = 0; i < n; ++i) {
            session.tick(); session.clock().advance(2048);
            session.scheduler().advanceTo(session.clock().getPosition());
            pumpEngineOutput(bridge, provider);
        }
    }
};

int main() {
    std::printf("Test: Gate 14C Task G — MIDI output failure handling\n");

    // ── No device from start: engine runs, nothing sent, no crash ──
    {
        Rig r;
        r.manager.refresh();
        r.session.adapter().postEvent({control::ControlAction::Start, 0, 0});
        r.play(40);
        TEST("no-device: engine playing", r.session.player().isPlaying());
        TEST("no-device: nothing sent", r.provider.sent.empty());
        TEST("no-device: state NO_DEVICE", r.manager.state() == OutputState::NoDevice);
    }

    // ── Connected -> disconnect during playback -> reconnect ──
    {
        Rig r;
        r.provider.setDevices({{0, "IAC Bus 1"}});
        r.manager.refresh();
        r.manager.selectDevice(0);
        TEST("connected", r.manager.state() == OutputState::Connected);

        r.session.adapter().postEvent({control::ControlAction::Start, 0, 0});
        r.play(60);
        const size_t sentWhileConnected = r.provider.sent.size();
        TEST("events sent while connected", sentWhileConnected > 0);

        // Disconnect mid-playback.
        r.provider.disconnect();
        r.provider.setDevices({});           // device vanished
        r.manager.refresh();
        TEST("disconnect -> DISCONNECTED", r.manager.state() == OutputState::Disconnected);
        r.play(60);
        TEST("engine survives disconnect (still playing)", r.session.player().isPlaying());
        TEST("no new events sent while disconnected",
             r.provider.sent.size() == sentWhileConnected);

        // Reconnect via hot-plug.
        r.provider.setDevices({{0, "IAC Bus 1"}});
        r.provider.reconnect();
        r.manager.notifyHotPlug();
        TEST("reconnect -> CONNECTED", r.manager.state() == OutputState::Connected);
        r.play(60);
        TEST("events resume after reconnect", r.provider.sent.size() > sentWhileConnected);
        TEST("no crash through full cycle", true);
    }

    // ── Endpoint switch mid-playback ──
    {
        Rig r;
        r.provider.setDevices({{0, "IAC Bus 1"}, {1, "Synth"}});
        r.manager.refresh(); r.manager.selectDevice(0);
        r.session.adapter().postEvent({control::ControlAction::Start, 0, 0});
        r.play(30);
        TEST("switch endpoint stays connected",
             r.manager.selectDevice(1) && r.manager.state() == OutputState::Connected);
        r.play(30);
        TEST("engine still playing after switch", r.session.player().isPlaying());
    }

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

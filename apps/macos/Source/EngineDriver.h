#pragma once

// EngineDriver (Gate 14) — runs the engine on a dedicated high-resolution timer
// thread, fed only through the lock-free EngineBridge. The UI (message thread)
// never blocks the engine: it pushes ControlEvents and polls telemetry. Engine
// scheduler output is routed to CoreMIDI. No audio/DSP is performed here.

#include <juce_core/juce_core.h>
#include "session/engine_session.h"
#include "control/engine_bridge.h"
#include "engine/midi/coremidi_out.h"

namespace ai_arranger::app {

class EngineDriver : private juce::HighResolutionTimer {
public:
    EngineDriver(session::EngineSession& session, control::EngineBridge& bridge)
        : session_(session), bridge_(bridge) {}

    void start() {
        session_.boot();
        midi_out_.initialize("AI Arranger");
        session_.scheduler().setOutputCallback(
            [this](const uasf::MidiEvent& e) { midi_out_.send(e); });
        startTimer(1); // ~1ms engine tick (no DSP; MIDI scheduling only)
    }

    void stop() {
        stopTimer();
        session_.shutdown();
    }

private:
    // Runs on the timer thread — drains UI control, advances engine, publishes
    // telemetry. Lock-free hand-off via the bridge.
    void hiResTimerCallback() override {
        control::ControlEvent ev;
        while (bridge_.nextControl(ev)) session_.adapter().postEvent(ev);

        session_.clock().advance(48 /* samples ~1ms @48k */);
        session_.tick();
        session_.scheduler().advanceTo(session_.clock().getPosition());

        control::EngineTelemetry t;
        t.playing = session_.player().isPlaying();
        t.section = session_.player().getCurrentSection();
        const auto chord = session_.player().getCurrentChord();
        t.chord_root = chord.root;
        t.chord_type = static_cast<uint8_t>(chord.type);
        t.position_ticks = session_.clock().getPosition();
        t.state = static_cast<int>(session_.adapter().state());
        bridge_.publishTelemetry(t);
    }

    session::EngineSession& session_;
    control::EngineBridge&  bridge_;
    midi::CoreMidiOut       midi_out_;
};

} // namespace ai_arranger::app

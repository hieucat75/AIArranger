#pragma once

// EngineDriver (Gate 14 / 14C) — runs the engine on a dedicated high-resolution
// timer thread, fed only through the lock-free EngineBridge. Engine MIDI output
// is routed engine → MidiOutputBridge → CoreMidiOutputProvider (Gate 14C); the
// engine path never touches CoreMIDI directly. The UI selects the output device
// via the MidiOutputManager. No audio/DSP.

#include <juce_core/juce_core.h>
#include "session/engine_session.h"
#include "control/engine_bridge.h"
#include "control/midi_output_bridge.h"
#include "control/engine_midi_route.h"
#include "midi/coremidi/coremidi_output_provider.h"
#include "midi/midi_output_manager.h"

namespace ai_arranger::app {

class EngineDriver : private juce::HighResolutionTimer {
public:
    EngineDriver(session::EngineSession& session, control::EngineBridge& bridge)
        : session_(session), bridge_(bridge) {}

    void start() {
        session_.boot();
        provider_.initialize("AI Arranger");
        // Engine scheduler output -> lock-free MIDI bridge (no CoreMIDI here).
        control::attachEngineOutput(session_.scheduler(), midi_bridge_);
        manager_.refresh();
        startTimer(1); // ~1ms engine tick (MIDI scheduling only, no DSP)
    }

    void stop() {
        stopTimer();
        session_.shutdown();
        provider_.shutdown();
    }

    midi::MidiOutputManager& midiManager() noexcept { return manager_; }
    uint64_t midiDispatchedCount() const noexcept { return provider_.dispatchedCount(); }

private:
    void hiResTimerCallback() override {
        control::ControlEvent ev;
        while (bridge_.nextControl(ev)) session_.adapter().postEvent(ev);

        session_.clock().advance(48 /* ~1ms @48k */);
        session_.tick();
        session_.scheduler().advanceTo(session_.clock().getPosition());

        // Drain engine MIDI -> output provider (CoreMIDI lives only here).
        control::pumpEngineOutput(midi_bridge_, provider_);

        control::EngineTelemetry t;
        t.playing = session_.player().isPlaying();
        t.section = session_.player().getCurrentSection();
        const auto chord = session_.player().getCurrentChord();
        t.chord_root = chord.root;
        t.chord_type = static_cast<uint8_t>(chord.type);
        t.position_ticks = session_.clock().getPosition();
        t.state = static_cast<int>(session_.adapter().state());
        t.dispatched_notes = static_cast<uint32_t>(provider_.dispatchedCount());
        bridge_.publishTelemetry(t);
    }

    session::EngineSession& session_;
    control::EngineBridge&  bridge_;

    midi::CoreMidiOutputProvider provider_;
    midi::MidiOutputManager      manager_{provider_};
    control::MidiOutputBridge    midi_bridge_;
};

} // namespace ai_arranger::app

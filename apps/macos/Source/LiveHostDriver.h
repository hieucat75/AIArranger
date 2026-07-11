#pragma once

// LiveHostDriver (Gate 2) — the Mac live-performance reference host's engine
// pump. Unlike the output-only Gate 14 EngineDriver, this drives the portable
// LiveEngineFacade end-to-end: a real CoreMIDI keyboard feeds chords in, the
// engine scheduler feeds a CoreMIDI destination out, and the UI polls the
// facade's atomic snapshot. CoreMIDI lives ONLY in the platform adapters
// (CoreMidiIn / CoreMidiOutputProvider); the facade + engine stay portable.
//
// Threading:
//   - hiResTimerCallback() (this timer thread) is the SOLE producer of the
//     engine's control queue. It drains the facade's two SPSC queues (UI + MIDI
//     input) inside tick(), advances the engine, and pumps output.
//   - The UI thread posts commands via facade() (lock-free) and reads snapshot().
//   - The CoreMIDI read thread routes messages onto the facade's input queue.
//
// No malloc / lock / ObjC in the timer callback or the CoreMIDI read callback.

#include <juce_core/juce_core.h>
#include "session/live_engine_facade.h"
#include "engine/midi/coremidi_in.h"
#include "engine/chord/fingered_detector.h"
#include "midi/coremidi/coremidi_output_provider.h"

namespace ai_arranger::app {

class LiveHostDriver : private juce::HighResolutionTimer {
public:
    LiveHostDriver() : facade_(&midi_in_, &midi_out_) {}
    ~LiveHostDriver() override { stop(); }

    void start() {
        if (running_) return;
        midi_in_.initialize("AI Arranger");
        midi_out_.initialize("AI Arranger");
        facade_.setChordScanMode(&detector_);   // fingered chord detection by default
        facade_.start();                         // boots engine + wires input sink
        startTimer(1);                           // ~1 ms engine tick (MIDI only, no DSP)
        running_ = true;
    }

    void stop() {
        if (!running_) return;
        stopTimer();
        // N1 (review #27): disconnect the CoreMIDI source BEFORE the facade clears
        // the input sink, so no read-thread callback races a half-cleared sink.
        midi_in_.selectSource(-1);
        facade_.stop();
        midi_in_.shutdown();
        midi_out_.shutdown();
        running_ = false;
    }

    // ── Accessors for the UI ──────────────────────────────────────────
    // Commands are posted and the snapshot is read through the facade (lock-free);
    // enumeration is read straight off the platform adapters.
    session::LiveEngineFacade&    facade()  noexcept { return facade_; }
    midi::CoreMidiIn&             midiIn()  noexcept { return midi_in_; }
    midi::CoreMidiOutputProvider& midiOut() noexcept { return midi_out_; }
    bool isRunning() const noexcept { return running_; }

private:
    void hiResTimerCallback() override { facade_.tick(48 /* ~1 ms @ 48 kHz */); }

    // Declared before facade_: the facade constructor captures &midi_in_/&midi_out_.
    midi::CoreMidiIn             midi_in_;
    midi::CoreMidiOutputProvider midi_out_;
    chord::FingeredDetector      detector_;
    session::LiveEngineFacade    facade_;
    bool                         running_{false};
};

} // namespace ai_arranger::app

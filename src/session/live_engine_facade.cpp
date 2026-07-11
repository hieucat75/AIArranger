#include "session/live_engine_facade.h"

#include "control/engine_midi_route.h"   // attachEngineOutput / pumpEngineOutput
#include "midi/midi_input_router.h"      // routeMidiInput

namespace ai_arranger::session {

namespace {
// Adapts a lock-free SPSC queue to the router's `postEvent` sink contract.
struct QueueSink {
    control::UiEventQueue<control::ControlEvent, 256>& q;
    bool postEvent(const control::ControlEvent& e) noexcept { return q.push(e); }
};
} // namespace

LiveEngineFacade::LiveEngineFacade(midi::IMidiInputSource* input,
                                   midi::IMidiOutputProvider* output) noexcept
    : input_(input), output_(output) {}

void LiveEngineFacade::start() noexcept {
    if (started_) return;
    session_.boot();
    // Engine scheduler output -> lock-free bridge (no Apple framework here).
    control::attachEngineOutput(session_.scheduler(), out_bridge_);
    // MIDI input read thread -> route -> input_q_ (drained in tick()).
    if (input_) {
        input_->setSink([this](const midi::MidiInputMessage& m) {
            QueueSink sink{input_q_};
            midi::routeMidiInput(m, sink);
        });
    }
    started_ = true;
    lifecycle_.apply(LifecycleEvent::Start);   // Gate 4: track high-level state
    publishSnapshot();
}

void LiveEngineFacade::stop() noexcept {
    if (!started_) return;
    // Flush the input sink so no late callback races teardown.
    if (input_) input_->setSink(nullptr);
    session_.shutdown();   // stops playback + flushes all notes (no hanging notes)
    started_ = false;
    lifecycle_.apply(LifecycleEvent::Stop);    // Gate 4: track high-level state
    publishSnapshot();
}

void LiveEngineFacade::setVariation(int index) noexcept {
    if (index < 0 || index > 3) return;
    const auto action = static_cast<control::ControlAction>(
        static_cast<int>(control::ControlAction::VariationA) + index);
    push(action);
}

void LiveEngineFacade::tick(uint32_t numSamples) noexcept {
    // Apply a pending tempo change (UI thread set it atomically).
    if (uint32_t bpm = pending_tempo_.exchange(0, std::memory_order_relaxed))
        session_.clock().setTempo(bpm);

    // Drain UI commands then MIDI input into the engine. This thread is the SOLE
    // producer of the adapter's SPSC control queue.
    control::ControlEvent ev;
    while (cmd_q_.pop(ev))   session_.adapter().postEvent(ev);
    while (input_q_.pop(ev)) session_.adapter().postEvent(ev);

    // Advance the engine and schedule output (mirrors the macOS EngineDriver).
    session_.clock().advance(numSamples);
    session_.tick();
    session_.scheduler().advanceTo(session_.clock().getPosition());
    if (output_) control::pumpEngineOutput(out_bridge_, *output_);

    publishSnapshot();
}

void LiveEngineFacade::publishSnapshot() noexcept {
    EngineSnapshot s;
    const auto chord = session_.player().getCurrentChord();
    s.playing        = session_.player().isPlaying();
    s.chordRoot      = chord.root;
    s.chordType      = static_cast<uint8_t>(chord.type);
    s.chordBass      = chord.bass;
    s.section        = static_cast<int32_t>(session_.player().getCurrentSection());
    s.tempoBpm       = session_.clock().getTempo();
    s.positionTicks  = session_.clock().getPosition();
    s.performerState = static_cast<int32_t>(session_.adapter().state());
    s.lifecycleState = static_cast<int32_t>(lifecycle_.state());
    s.midiInLive     = input_  ? input_->hasLiveSource() : false;
    s.midiOutLive    = output_ ? output_->hasLiveDestination() : false;
    s.receivedMessages = input_  ? input_->receivedCount() : 0;
    s.dispatchedNotes  = output_ ? output_->dispatchedCount() : 0;
    snapshot_.store(s, std::memory_order_release);
}

} // namespace ai_arranger::session

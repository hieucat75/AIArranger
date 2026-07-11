#ifndef AI_ARRANGER_SESSION_LIVE_ENGINE_FACADE_H
#define AI_ARRANGER_SESSION_LIVE_ENGINE_FACADE_H

#include "session/engine_session.h"
#include "session/engine_snapshot.h"
#include "control/ui_event_queue.h"
#include "control/midi_output_bridge.h"
#include "engine/control/control_events.h"
#include "engine/chord/chord_scan_mode.h"
#include "midi/midi_input_source.h"
#include "midi/midi_output_provider.h"
#include <atomic>
#include <cstdint>

// ── LiveEngineFacade — the single entry point the UI/bridge drives ───────────
//
// Portable (no Apple framework). Owns an EngineSession and wires the full live
// loop around it: an injected MIDI input source feeds chords in; the engine
// scheduler feeds an injected MIDI output provider out. The Swift/C++ bridge
// calls the command methods and polls snapshot(); it never touches the engine
// directly.
//
// Threading (matches the macOS EngineDriver, generalised + testable):
//   - Commands (transport/tempo/variation/panic/select) may be called from any
//     thread — each enqueues onto a lock-free SPSC queue (UI thread = producer).
//   - The MIDI input source's read thread routes messages onto a SECOND SPSC
//     queue (input thread = producer). Both are drained inside tick() so the
//     engine's own SPSC control queue keeps a single producer (this tick thread).
//   - tick() runs on the engine thread; it advances the clock/sequencer, pumps
//     output, and republishes the atomic snapshot the UI polls.

namespace ai_arranger::session {

class LiveEngineFacade {
public:
    // Both pointers may be null (headless / not-yet-selected). The facade does
    // not own them; the platform/app owns their lifetime.
    LiveEngineFacade(midi::IMidiInputSource* input,
                     midi::IMidiOutputProvider* output) noexcept;

    // Boot the engine, attach the output route, and wire the input source's sink.
    void start() noexcept;
    void stop() noexcept;

    // Load a normalized style. Call before start() (boot keeps a pre-loaded
    // style); calling while stopped re-arms the sequencer with a new style.
    void loadStyle(const uasf::StyleDefinition& style) noexcept { session_.loadStyle(style); }

    // ── Commands (any thread, lock-free) ──────────────────────────────
    void transportStart() noexcept { cmd_q_.push({control::ControlAction::Start, 0, 0}); }
    void transportStop()  noexcept { cmd_q_.push({control::ControlAction::Stop, 0, 0}); }
    // Sync Start: arm the engine; the first detected chord fires playback.
    void syncStart()      noexcept { cmd_q_.push({control::ControlAction::SyncArm, 0, 0}); }
    void intro()          noexcept { cmd_q_.push({control::ControlAction::Intro, 0, 0}); }
    void fill()           noexcept { cmd_q_.push({control::ControlAction::Fill, 0, 0}); }
    void breakSection()   noexcept { cmd_q_.push({control::ControlAction::Break, 0, 0}); }
    void ending()         noexcept { cmd_q_.push({control::ControlAction::Ending, 0, 0}); }
    void panic()          noexcept { cmd_q_.push({control::ControlAction::Panic, 0, 0}); }
    // variation 0..3 -> A..D; out-of-range ignored.
    void setVariation(int index) noexcept;
    void setTempo(uint32_t bpm) noexcept { pending_tempo_.store(bpm, std::memory_order_relaxed); }

    // Device selection (delegates to the injected source/provider).
    bool selectMidiInput(int index) noexcept  { return input_  ? input_->selectSource(index) : false; }
    bool selectMidiOutput(int index) noexcept { return output_ ? output_->select(index) : false; }

    // Chord detector (caller owns the instance; Fingered is a sensible default).
    void setChordScanMode(const chord::IChordScanMode* mode) noexcept {
        session_.adapter().setChordScanMode(mode);
    }

    // ── Engine step (engine thread) ───────────────────────────────────
    // Advance the engine by numSamples of wall-clock; drains queues, schedules
    // output, republishes the snapshot.
    void tick(uint32_t numSamples) noexcept;

    // ── UI read ───────────────────────────────────────────────────────
    EngineSnapshot snapshot() const noexcept { return snapshot_.load(std::memory_order_acquire); }

    EngineSession& session() noexcept { return session_; }

private:
    void publishSnapshot() noexcept;

    EngineSession              session_;
    midi::IMidiInputSource*     input_;
    midi::IMidiOutputProvider*  output_;
    control::MidiOutputBridge   out_bridge_;

    control::UiEventQueue<control::ControlEvent, 256> cmd_q_;    // UI thread -> tick
    control::UiEventQueue<control::ControlEvent, 256> input_q_;  // MIDI-in thread -> tick

    std::atomic<uint32_t>       pending_tempo_{0};
    std::atomic<EngineSnapshot> snapshot_{};
    bool started_{false};
};

} // namespace ai_arranger::session

#endif // AI_ARRANGER_SESSION_LIVE_ENGINE_FACADE_H

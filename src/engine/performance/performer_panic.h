#ifndef AI_ARRANGER_PERFORMANCE_PERFORMER_PANIC_H
#define AI_ARRANGER_PERFORMANCE_PERFORMER_PANIC_H

#include "engine/midi/panic.h"
#include "engine/midi/scheduler.h"
#include "engine/performance/realtime_state_machine.h"
#include <atomic>
#include <cstdint>

// ── Performer-safe panic & recovery (Gate 12 Task H) ─────────────────
//
// Wraps the Gate 9 PanicHandler for live use: all-notes-off + queue flush AND
// forces the performer state machine back to Idle, so the player can immediately
// re-arm and start again WITHOUT reconstructing anything. Realtime-safe (reuses
// the lock-free PanicHandler; no alloc/lock). Safe to call repeatedly (spam).

namespace ai_arranger::performance {

class PerformerPanic {
public:
    PerformerPanic(midi::PanicHandler& panic, RealtimeStateMachine& sm) noexcept
        : panic_(panic), sm_(sm) {}

    // Emergency stop: all-sound-off + all-notes-off via the callback, clear the
    // scheduler queue, and drive the state machine to Idle (recovery-ready).
    void panic(midi::MidiScheduler& scheduler,
               midi::MidiEventCallback send) noexcept {
        panic_.panic(scheduler, send);
        sm_.apply(PerformerInput::Panic);
        count_.fetch_add(1, std::memory_order_relaxed);
    }

    // Ready to play again with no restart: Idle state + zero hanging notes.
    bool isRecovered() const noexcept {
        return sm_.state() == PerformerState::Idle && !panic_.hasActiveNotes();
    }

    uint32_t panicCount() const noexcept {
        return count_.load(std::memory_order_relaxed);
    }

private:
    midi::PanicHandler& panic_;
    RealtimeStateMachine& sm_;
    std::atomic<uint32_t> count_{0};
};

} // namespace ai_arranger::performance

#endif // AI_ARRANGER_PERFORMANCE_PERFORMER_PANIC_H

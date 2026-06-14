#ifndef AI_ARRANGER_PERFORMANCE_REALTIME_STATE_MACHINE_H
#define AI_ARRANGER_PERFORMANCE_REALTIME_STATE_MACHINE_H

#include <cstdint>
#include <atomic>

// ── Realtime performer state machine (Gate 12 Task A) ────────────────
//
// Foundation for the performer layer. Deterministic, lock-free: the state is a
// single atomic and every transition is a pure table lookup, so an identical
// input sequence from Idle always replays to the identical state sequence.
// Invalid inputs are no-ops (apply() returns false). Panic/Reset force Idle from
// any state. Realtime-safe: no allocation, no locks, no syscalls.

namespace ai_arranger::performance {

enum class PerformerState : uint8_t {
    Idle,               // nothing armed or playing
    Armed,              // sync-start armed, waiting for a chord/start
    Playing,            // style running
    PendingTransition,  // a section/variation switch is queued for the boundary
    Stopping,           // stop requested, finishing the current boundary
};

enum class PerformerInput : uint8_t {
    Arm,                // arm sync start
    ChordDetected,      // first chord -> sync start
    Start,              // explicit start
    QueueTransition,    // queue a variation/fill/section change
    TransitionResolved, // the queued transition fired on the boundary
    Stop,               // request stop
    Stopped,            // stop completed at the boundary
    Panic,              // emergency -> Idle
    Reset,              // hard reset -> Idle
};

class RealtimeStateMachine {
public:
    PerformerState state() const noexcept {
        return state_.load(std::memory_order_acquire);
    }

    // Apply an input. Returns true if it caused a state change, false if the
    // input is invalid for the current state (no-op). Single-writer (control
    // thread); readers use state().
    bool apply(PerformerInput in) noexcept;

    void reset() noexcept { state_.store(PerformerState::Idle, std::memory_order_release); }

    // Pure transition function (exposed for replay/testing). `changed` is set to
    // whether the state actually moves.
    static PerformerState next(PerformerState s, PerformerInput in,
                               bool& changed) noexcept;

private:
    std::atomic<PerformerState> state_{PerformerState::Idle};
};

} // namespace ai_arranger::performance

#endif // AI_ARRANGER_PERFORMANCE_REALTIME_STATE_MACHINE_H

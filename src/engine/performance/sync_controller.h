#ifndef AI_ARRANGER_PERFORMANCE_SYNC_CONTROLLER_H
#define AI_ARRANGER_PERFORMANCE_SYNC_CONTROLLER_H

#include "engine/performance/realtime_state_machine.h"

// ── Sync Start / Sync Stop controller (Gate 12 Task B) ───────────────
//
// Arm-before-play: arm sync start, then the first detected chord fires the start
// exactly once — even under rapid/concurrent chord triggers (CAS single-fire).
// Decides intent only; the actual boundary/quantize is the sequencer's job. No
// audio, no allocation, lock-free.

namespace ai_arranger::performance {

class SyncController {
public:
    // Arm sync start (Idle -> Armed). Returns true if it armed.
    bool arm() noexcept { return sm_.apply(PerformerInput::Arm); }

    // First chord while Armed fires start once; later/concurrent calls return
    // false (already Playing). Race-safe.
    bool onChordPresent() noexcept { return sm_.tryApply(PerformerInput::ChordDetected); }

    // Explicit start (also single-fire).
    bool start() noexcept { return sm_.tryApply(PerformerInput::Start); }

    // Sync stop request (e.g. lower zone cleared) -> Stopping; then notifyStopped
    // at the boundary -> Idle.
    bool requestStop() noexcept { return sm_.apply(PerformerInput::Stop); }
    bool notifyStopped() noexcept { return sm_.apply(PerformerInput::Stopped); }

    bool isArmed() const noexcept { return sm_.state() == PerformerState::Armed; }
    bool isPlaying() const noexcept { return sm_.state() == PerformerState::Playing; }
    PerformerState state() const noexcept { return sm_.state(); }
    void reset() noexcept { sm_.reset(); }

private:
    RealtimeStateMachine sm_;
};

} // namespace ai_arranger::performance

#endif // AI_ARRANGER_PERFORMANCE_SYNC_CONTROLLER_H

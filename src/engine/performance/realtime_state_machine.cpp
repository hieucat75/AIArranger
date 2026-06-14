#include "engine/performance/realtime_state_machine.h"

namespace ai_arranger::performance {

PerformerState RealtimeStateMachine::next(PerformerState s, PerformerInput in,
                                          bool& changed) noexcept {
    // Panic / Reset force Idle from any state.
    if (in == PerformerInput::Panic || in == PerformerInput::Reset) {
        changed = (s != PerformerState::Idle);
        return PerformerState::Idle;
    }

    PerformerState n = s;  // default: no-op
    switch (s) {
        case PerformerState::Idle:
            if (in == PerformerInput::Arm)   n = PerformerState::Armed;
            else if (in == PerformerInput::Start) n = PerformerState::Playing;
            break;
        case PerformerState::Armed:
            if (in == PerformerInput::ChordDetected ||
                in == PerformerInput::Start) n = PerformerState::Playing;
            else if (in == PerformerInput::Stop) n = PerformerState::Idle;
            break;
        case PerformerState::Playing:
            if (in == PerformerInput::QueueTransition)
                n = PerformerState::PendingTransition;
            else if (in == PerformerInput::Stop) n = PerformerState::Stopping;
            break;
        case PerformerState::PendingTransition:
            if (in == PerformerInput::TransitionResolved)
                n = PerformerState::Playing;
            else if (in == PerformerInput::QueueTransition)
                n = PerformerState::PendingTransition; // re-queue: stays pending
            else if (in == PerformerInput::Stop) n = PerformerState::Stopping;
            break;
        case PerformerState::Stopping:
            if (in == PerformerInput::Stopped) n = PerformerState::Idle;
            break;
    }
    changed = (n != s);
    return n;
}

bool RealtimeStateMachine::apply(PerformerInput in) noexcept {
    bool changed = false;
    const PerformerState n = next(state_.load(std::memory_order_acquire), in, changed);
    state_.store(n, std::memory_order_release);
    return changed;
}

bool RealtimeStateMachine::tryApply(PerformerInput in) noexcept {
    PerformerState cur = state_.load(std::memory_order_acquire);
    for (;;) {
        bool changed = false;
        const PerformerState n = next(cur, in, changed);
        if (!changed) return false;  // no-op for this state
        if (state_.compare_exchange_weak(cur, n, std::memory_order_acq_rel,
                                         std::memory_order_acquire))
            return true;             // we performed the change
        // CAS failed: `cur` reloaded; retry against the new state.
    }
}

} // namespace ai_arranger::performance

#include "session/engine_lifecycle.h"

// Pure transition table for the engine lifecycle contract. See the header for the
// authoritative diagram. Kept as an explicit switch (not a 2-D array) so each
// legal edge and its classifying error is readable and greppable; the compiler
// still lowers it to a jump table. No allocation, no locks, no side effects.

namespace ai_arranger::session {

namespace {

// An illegal edge: stay put, classify the reason.
constexpr LifecycleTransition illegal(LifecycleState s, EngineError e) noexcept {
    return {s, e};
}
constexpr LifecycleTransition go(LifecycleState next) noexcept {
    return {next, EngineError::Ok};
}

} // namespace

LifecycleTransition EngineLifecycle::step(LifecycleState s, LifecycleEvent ev) noexcept {
    using S = LifecycleState;
    using E = LifecycleEvent;

    // Terminal state: nothing is legal once shut down.
    if (s == S::ShutDown)
        return illegal(s, EngineError::InvalidState);

    // Shutdown is legal from every non-terminal state.
    if (ev == E::Shutdown)
        return go(S::ShutDown);

    // Panic is a from-any-active safety valve: always legal, never moves state.
    if (ev == E::Panic)
        return go(s);

    switch (s) {
        case S::Created:
            switch (ev) {
                case E::Configure: return go(S::Configured);
                case E::LoadStyle: return go(S::Ready);
                case E::Start:     return go(S::Running);
                case E::Suspend:   return illegal(s, EngineError::NotStarted);
                case E::Resume:    return illegal(s, EngineError::NotStarted);
                case E::Stop:      return illegal(s, EngineError::NotStarted);
                default:           return illegal(s, EngineError::InvalidState);
            }

        case S::Configured:
            switch (ev) {
                case E::Configure: return go(S::Configured);   // idempotent reconfigure
                case E::LoadStyle: return go(S::Ready);
                case E::Start:     return go(S::Running);
                case E::Suspend:   return illegal(s, EngineError::NotStarted);
                case E::Resume:    return illegal(s, EngineError::NotStarted);
                case E::Stop:      return illegal(s, EngineError::NotStarted);
                default:           return illegal(s, EngineError::InvalidState);
            }

        case S::Ready:
            switch (ev) {
                case E::Configure: return go(S::Configured);
                case E::LoadStyle: return go(S::Ready);         // reload
                case E::Start:     return go(S::Running);
                case E::Suspend:   return illegal(s, EngineError::NotStarted);
                case E::Resume:    return illegal(s, EngineError::NotStarted);
                case E::Stop:      return illegal(s, EngineError::NotStarted);
                default:           return illegal(s, EngineError::InvalidState);
            }

        case S::Running:
            switch (ev) {
                case E::Suspend:   return go(S::Suspended);
                case E::Stop:      return go(S::Stopped);
                case E::Start:     return illegal(s, EngineError::AlreadyStarted);
                case E::Resume:    return illegal(s, EngineError::InvalidState);
                case E::LoadStyle: return illegal(s, EngineError::InvalidState);
                case E::Configure: return illegal(s, EngineError::InvalidState);
                default:           return illegal(s, EngineError::InvalidState);
            }

        case S::Suspended:
            switch (ev) {
                case E::Resume:    return go(S::Running);
                case E::Stop:      return go(S::Stopped);
                case E::Start:     return illegal(s, EngineError::AlreadyStarted);
                case E::Suspend:   return illegal(s, EngineError::InvalidState);
                case E::LoadStyle: return illegal(s, EngineError::InvalidState);
                case E::Configure: return illegal(s, EngineError::InvalidState);
                default:           return illegal(s, EngineError::InvalidState);
            }

        case S::Stopped:
            switch (ev) {
                case E::Configure: return go(S::Configured);
                case E::LoadStyle: return go(S::Ready);
                case E::Start:     return go(S::Running);       // restart
                case E::Suspend:   return illegal(s, EngineError::NotStarted);
                case E::Resume:    return illegal(s, EngineError::NotStarted);
                case E::Stop:      return illegal(s, EngineError::NotStarted);
                default:           return illegal(s, EngineError::InvalidState);
            }

        case S::ShutDown:   // handled above; keeps the switch exhaustive
            return illegal(s, EngineError::InvalidState);
    }

    return illegal(s, EngineError::InvalidState);
}

} // namespace ai_arranger::session

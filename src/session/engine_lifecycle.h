#ifndef AI_ARRANGER_SESSION_ENGINE_LIFECYCLE_H
#define AI_ARRANGER_SESSION_ENGINE_LIFECYCLE_H

#include "session/engine_contract.h"
#include <cstdint>

// ── Engine lifecycle contract (Gate 4 — definition + validation only) ────────
//
// A pure, deterministic state machine that pins the *legal order* of engine
// lifecycle operations. It holds NO engine state and touches NO hardware — it is
// the contract's referee, exercised entirely headless. The LiveEngineFacade keeps
// one of these as a bookkeeping tracker so `lifecycleState()` is always truthful
// and clearly-illegal command orderings are rejected deterministically. The Gate 5
// product host (iPad SwiftUI / C bridge) drives the same machine.
//
// Transition table (Panic is a from-any-active safety valve that never changes
// state; Shutdown is terminal):
//
//   Created    --Configure--> Configured
//   Created    --LoadStyle--> Ready        (construction already wired devices)
//   Created    --Start-----> Running       (boot() loads a default style)
//   Configured --LoadStyle--> Ready
//   Configured --Start-----> Running
//   Ready      --LoadStyle--> Ready         (reload while not running)
//   Ready      --Start-----> Running
//   Running    --Suspend---> Suspended
//   Suspended  --Resume----> Running
//   Running    --Stop------> Stopped
//   Suspended  --Stop------> Stopped
//   Stopped    --Start-----> Running        (restart)
//   Stopped    --LoadStyle--> Ready
//   Stopped    --Configure--> Configured
//   <any but ShutDown> --Panic----> (unchanged)
//   <any but ShutDown> --Shutdown--> ShutDown
//   ShutDown   --<anything>--> InvalidState (terminal)
//
// Everything else is an illegal transition: apply() returns a non-Ok EngineError
// and leaves the state unchanged. The transition function is a pure static (step)
// so an identical event sequence from Created always replays to the identical
// state sequence — the same determinism guarantee as RealtimeStateMachine.

namespace ai_arranger::session {

enum class LifecycleState : uint8_t {
    Created = 0,   // constructed; nothing configured or loaded
    Configured,    // devices/params set (explicit configure)
    Ready,         // a style is loaded; armed to start
    Running,       // live loop running
    Suspended,     // paused; devices retained, resumable
    Stopped,       // stopped after running; restartable
    ShutDown,      // terminal; the handle must not be reused
};

enum class LifecycleEvent : uint8_t {
    Configure = 0,
    LoadStyle,
    Start,
    Suspend,
    Resume,
    Stop,
    Panic,
    Shutdown,
};

inline const char* toString(LifecycleState s) noexcept {
    switch (s) {
        case LifecycleState::Created:    return "Created";
        case LifecycleState::Configured: return "Configured";
        case LifecycleState::Ready:      return "Ready";
        case LifecycleState::Running:    return "Running";
        case LifecycleState::Suspended:  return "Suspended";
        case LifecycleState::Stopped:    return "Stopped";
        case LifecycleState::ShutDown:   return "ShutDown";
    }
    return "Unknown";
}

// Result of a pure transition attempt.
struct LifecycleTransition {
    LifecycleState next;    // resulting state (== input state when error != Ok)
    EngineError    error;   // Ok if the transition is legal
};

class EngineLifecycle {
public:
    LifecycleState state() const noexcept { return state_; }

    // Attempt a transition. On success applies it and returns Ok; on an illegal
    // transition leaves the state unchanged and returns the classifying error.
    EngineError apply(LifecycleEvent ev) noexcept {
        const LifecycleTransition t = step(state_, ev);
        state_ = t.next;
        return t.error;
    }

    // Back to Created (for handle reuse in tests). Not part of the runtime contract.
    void reset() noexcept { state_ = LifecycleState::Created; }

    // Pure transition function — no side effects, exposed for replay/testing.
    static LifecycleTransition step(LifecycleState s, LifecycleEvent ev) noexcept;

private:
    LifecycleState state_ = LifecycleState::Created;
};

} // namespace ai_arranger::session

#endif // AI_ARRANGER_SESSION_ENGINE_LIFECYCLE_H

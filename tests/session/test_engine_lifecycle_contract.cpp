// Gate 4 — engine lifecycle contract (headless, no hardware).
//
// Exhaustively exercises the pure lifecycle state machine: the legal happy path,
// every illegal edge and its classifying error, the panic safety valve, the
// terminal ShutDown state, deterministic replay, and repeated create/destroy.
// Links the portable core ONLY (no platform-apple / no Apple framework), which
// doubles as a platform-boundary assertion for the contract.

#include "session/engine_lifecycle.h"
#include <cstdio>

using namespace ai_arranger::session;
using S = LifecycleState;
using E = LifecycleEvent;

static int failures = 0, passes = 0;
#define TEST(name, expr) do { \
    if (!(expr)) { std::fprintf(stderr, "  FAIL: %s\n", name); failures++; } \
    else { std::printf("  PASS: %s\n", name); passes++; } \
} while(0)

// Apply and assert the resulting state + error code in one shot.
static bool applies(EngineLifecycle& lc, E ev, S expectState, EngineError expectErr) {
    const EngineError err = lc.apply(ev);
    return err == expectErr && lc.state() == expectState;
}

int main() {
    std::printf("Test: Gate 4 engine lifecycle contract\n");

    // ── Initial state ──
    {
        EngineLifecycle lc;
        TEST("starts in Created", lc.state() == S::Created);
    }

    // ── Full legal happy path ──
    {
        EngineLifecycle lc;
        TEST("Created --Configure--> Configured",
             applies(lc, E::Configure, S::Configured, EngineError::Ok));
        TEST("Configured --LoadStyle--> Ready",
             applies(lc, E::LoadStyle, S::Ready, EngineError::Ok));
        TEST("Ready --LoadStyle--> Ready (reload)",
             applies(lc, E::LoadStyle, S::Ready, EngineError::Ok));
        TEST("Ready --Start--> Running",
             applies(lc, E::Start, S::Running, EngineError::Ok));
        TEST("Running --Suspend--> Suspended",
             applies(lc, E::Suspend, S::Suspended, EngineError::Ok));
        TEST("Suspended --Resume--> Running",
             applies(lc, E::Resume, S::Running, EngineError::Ok));
        TEST("Running --Stop--> Stopped",
             applies(lc, E::Stop, S::Stopped, EngineError::Ok));
        TEST("Stopped --Start--> Running (restart)",
             applies(lc, E::Start, S::Running, EngineError::Ok));
        TEST("Running --Shutdown--> ShutDown",
             applies(lc, E::Shutdown, S::ShutDown, EngineError::Ok));
    }

    // ── Direct Start from Created (boot loads a default style) ──
    {
        EngineLifecycle lc;
        TEST("Created --Start--> Running (implicit default style)",
             applies(lc, E::Start, S::Running, EngineError::Ok));
    }

    // ── Suspended --Stop--> Stopped ──
    {
        EngineLifecycle lc;
        lc.apply(E::Start); lc.apply(E::Suspend);
        TEST("Suspended --Stop--> Stopped",
             applies(lc, E::Stop, S::Stopped, EngineError::Ok));
    }

    // ── Illegal edges classify correctly and DON'T move state ──
    {
        EngineLifecycle lc;   // Created
        TEST("Created --Suspend--> NotStarted (no move)",
             applies(lc, E::Suspend, S::Created, EngineError::NotStarted));
        TEST("Created --Resume--> NotStarted (no move)",
             applies(lc, E::Resume, S::Created, EngineError::NotStarted));
        TEST("Created --Stop--> NotStarted (no move)",
             applies(lc, E::Stop, S::Created, EngineError::NotStarted));
    }
    {
        EngineLifecycle lc; lc.apply(E::Start);   // Running
        TEST("Running --Start--> AlreadyStarted (no move)",
             applies(lc, E::Start, S::Running, EngineError::AlreadyStarted));
        TEST("Running --Resume--> InvalidState (no move)",
             applies(lc, E::Resume, S::Running, EngineError::InvalidState));
        TEST("Running --LoadStyle--> InvalidState (no move)",
             applies(lc, E::LoadStyle, S::Running, EngineError::InvalidState));
        TEST("Running --Configure--> InvalidState (no move)",
             applies(lc, E::Configure, S::Running, EngineError::InvalidState));
    }
    {
        EngineLifecycle lc; lc.apply(E::LoadStyle);   // Ready
        TEST("Ready --Resume--> NotStarted (no move)",
             applies(lc, E::Resume, S::Ready, EngineError::NotStarted));
        TEST("Ready --Suspend--> NotStarted (no move)",
             applies(lc, E::Suspend, S::Ready, EngineError::NotStarted));
    }
    {
        EngineLifecycle lc; lc.apply(E::Start); lc.apply(E::Stop);   // Stopped
        TEST("Stopped --Suspend--> NotStarted (no move)",
             applies(lc, E::Suspend, S::Stopped, EngineError::NotStarted));
        TEST("Stopped --LoadStyle--> Ready",
             applies(lc, E::LoadStyle, S::Ready, EngineError::Ok));
    }

    // ── Panic is a from-any-active safety valve: legal, no state change ──
    {
        const S actives[] = {S::Created, S::Configured, S::Ready,
                             S::Running, S::Suspended, S::Stopped};
        bool ok = true;
        for (S st : actives) {
            const LifecycleTransition t = EngineLifecycle::step(st, E::Panic);
            if (t.error != EngineError::Ok || t.next != st) ok = false;
        }
        TEST("Panic legal from every active state, no state change", ok);
    }

    // ── Terminal ShutDown: nothing is legal, state pinned ──
    {
        EngineLifecycle lc; lc.apply(E::Shutdown);
        TEST("in ShutDown after Shutdown", lc.state() == S::ShutDown);
        bool allRejected = true;
        const E every[] = {E::Configure, E::LoadStyle, E::Start, E::Suspend,
                          E::Resume, E::Stop, E::Panic, E::Shutdown};
        for (E ev : every) {
            if (lc.apply(ev) != EngineError::InvalidState || lc.state() != S::ShutDown)
                allRejected = false;
        }
        TEST("ShutDown rejects every event with InvalidState, stays terminal", allRejected);
    }

    // ── Determinism: same event sequence -> same state, always ──
    {
        const E seq[] = {E::Configure, E::LoadStyle, E::Start, E::Suspend,
                        E::Resume, E::Stop, E::Start, E::Panic, E::Stop};
        S a = S::Created, b = S::Created;
        for (E ev : seq) { a = EngineLifecycle::step(a, ev).next;
                           b = EngineLifecycle::step(b, ev).next; }
        TEST("pure step is deterministic (replays identically)", a == b && a == S::Stopped);
    }

    // ── Repeated create/destroy is stable (reset returns to Created) ──
    {
        EngineLifecycle lc;
        bool ok = true;
        for (int i = 0; i < 1000; ++i) {
            lc.reset();
            if (lc.state() != S::Created) { ok = false; break; }
            lc.apply(E::Start);
            lc.apply(E::Stop);
            lc.apply(E::Shutdown);
            if (lc.state() != S::ShutDown) { ok = false; break; }
        }
        TEST("1000x create/run/stop/shutdown cycles stable", ok);
    }

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

// Gate 12 Task A — realtime state machine tests (deterministic, no hardware).

#include "engine/performance/realtime_state_machine.h"
#include <cstdio>
#include <vector>

using namespace ai_arranger::performance;

static int failures = 0, passes = 0;
#define TEST(name, expr) do { \
    if (!(expr)) { std::fprintf(stderr, "  \xE2\x9D\x8C FAIL: %s\n", name); failures++; } \
    else { std::printf("  \xE2\x9C\x85 PASS: %s\n", name); passes++; } \
} while(0)

using S = PerformerState;
using I = PerformerInput;

int main() {
    std::printf("Test: Gate 12 Task A — realtime state machine\n");

    // ── Happy path ──
    {
        RealtimeStateMachine m;
        TEST("starts Idle", m.state() == S::Idle);
        TEST("Arm -> Armed (changed)", m.apply(I::Arm) && m.state() == S::Armed);
        TEST("ChordDetected -> Playing (sync start)",
             m.apply(I::ChordDetected) && m.state() == S::Playing);
        TEST("QueueTransition -> PendingTransition",
             m.apply(I::QueueTransition) && m.state() == S::PendingTransition);
        TEST("TransitionResolved -> Playing",
             m.apply(I::TransitionResolved) && m.state() == S::Playing);
        TEST("Stop -> Stopping", m.apply(I::Stop) && m.state() == S::Stopping);
        TEST("Stopped -> Idle", m.apply(I::Stopped) && m.state() == S::Idle);
    }

    // ── Invalid inputs are no-ops (return false, state unchanged) ──
    {
        RealtimeStateMachine m;
        TEST("Idle + Stop = no-op", !m.apply(I::Stop) && m.state() == S::Idle);
        TEST("Idle + TransitionResolved = no-op",
             !m.apply(I::TransitionResolved) && m.state() == S::Idle);
        m.apply(I::Arm);
        TEST("Armed + QueueTransition = no-op",
             !m.apply(I::QueueTransition) && m.state() == S::Armed);
    }

    // ── Re-queue while pending stays pending (no double) ──
    {
        RealtimeStateMachine m;
        m.apply(I::Start); m.apply(I::QueueTransition);
        TEST("Playing->Pending via Start path", m.state() == S::PendingTransition);
        bool changed = m.apply(I::QueueTransition);
        TEST("re-queue stays Pending (no-op change=false)",
             !changed && m.state() == S::PendingTransition);
    }

    // ── Panic / Reset force Idle from any state ──
    {
        RealtimeStateMachine m;
        m.apply(I::Start); m.apply(I::QueueTransition);
        TEST("Panic from Pending -> Idle", m.apply(I::Panic) && m.state() == S::Idle);
        TEST("Panic from Idle = no change", !m.apply(I::Panic));
        m.apply(I::Start);
        m.reset();
        TEST("reset() -> Idle", m.state() == S::Idle);
    }

    // ── Determinism: same input sequence replays to same states ──
    {
        std::vector<I> seq = {I::Arm, I::ChordDetected, I::QueueTransition,
                              I::TransitionResolved, I::Stop, I::Stopped};
        RealtimeStateMachine a, b;
        bool identical = true;
        for (I in : seq) {
            a.apply(in); b.apply(in);
            if (a.state() != b.state()) identical = false;
        }
        TEST("deterministic replay (two machines agree)", identical);
        TEST("replay ends Idle", a.state() == S::Idle);
    }

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

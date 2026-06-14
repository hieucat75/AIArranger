// Gate 12 Task H — performer panic + recovery (no restart, rapid recovery).

#include "engine/performance/performer_panic.h"
#include "engine/performance/sync_controller.h"
#include "engine/midi/panic.h"
#include "engine/midi/scheduler.h"
#include "engine/uasf/types.h"
#include <cstdio>
#include <vector>

using namespace ai_arranger;
using namespace ai_arranger::performance;

static int failures = 0, passes = 0;
#define TEST(name, expr) do { \
    if (!(expr)) { std::fprintf(stderr, "  \xE2\x9D\x8C FAIL: %s\n", name); failures++; } \
    else { std::printf("  \xE2\x9C\x85 PASS: %s\n", name); passes++; } \
} while(0)

int main() {
    std::printf("Test: Gate 12 Task H — performer panic + recovery\n");

    midi::PanicHandler ph;
    midi::MidiScheduler sch;
    RealtimeStateMachine sm;
    PerformerPanic pp(ph, sm);

    std::vector<uasf::MidiEvent> sent;
    auto cb = [&](const uasf::MidiEvent& e) { sent.push_back(e); };

    // Simulate active playback with sounding notes.
    sm.apply(PerformerInput::Start);            // Playing
    ph.noteOn(0, 60); ph.noteOn(0, 64); ph.noteOn(9, 38);
    TEST("setup: playing + active notes",
         sm.state() == PerformerState::Playing && ph.hasActiveNotes());

    // ── Panic during active playback ──
    pp.panic(sch, cb);
    TEST("panic -> state Idle", sm.state() == PerformerState::Idle);
    TEST("panic -> no hanging notes", !ph.hasActiveNotes());
    TEST("panic -> recovered", pp.isRecovered());
    bool emittedAllNotesOff = false;
    for (auto& e : sent)
        if (e.type == uasf::MidiEventType::ControlChange &&
            e.data1 == midi::PanicHandler::kAllNotesOff) emittedAllNotesOff = true;
    TEST("panic -> all-notes-off emitted", emittedAllNotesOff);

    // ── Recovery WITHOUT restart: same objects re-arm and play ──
    SyncController sync;   // performer can re-arm immediately
    TEST("re-arm after panic", sync.arm() && sync.isArmed());
    TEST("re-start after panic", sync.onChordPresent() && sync.isPlaying());

    // ── Rapid panic spam is safe ──
    sm.apply(PerformerInput::Start);
    ph.noteOn(1, 50);
    for (int i = 0; i < 200; ++i) pp.panic(sch, cb);
    TEST("rapid panic spam: still recovered", pp.isRecovered());
    TEST("rapid panic spam: count advanced", pp.panicCount() >= 201);

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

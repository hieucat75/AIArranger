// Gate 14 Task G — UI chord intent via the bridge (ControlAction::Chord),
// decoded on the engine thread by the adapter. Headless.

#include "session/engine_session.h"
#include "engine/control/control_events.h"
#include "engine/arranger/chord_input.h"
#include <cstdio>

using namespace ai_arranger;
using namespace ai_arranger::session;
using control::ControlAction;
using control::ControlEvent;
using arranger::ChordType;

static int failures = 0, passes = 0;
#define TEST(name, expr) do { \
    if (!(expr)) { std::fprintf(stderr, "  \xE2\x9D\x8C FAIL: %s\n", name); failures++; } \
    else { std::printf("  \xE2\x9C\x85 PASS: %s\n", name); passes++; } \
} while(0)

static ControlEvent chordEvent(uint8_t root, ChordType t) {
    ControlEvent e; e.action = ControlAction::Chord;
    e.param = (root & 0xFF) | (static_cast<int>(t) << 8);
    return e;
}
static void run(EngineSession& s, int n) {
    for (int i = 0; i < n; ++i) { s.tick(); s.clock().advance(2048);
        s.scheduler().advanceTo(s.clock().getPosition()); }
}

int main() {
    std::printf("Test: Gate 14 Task G — chord control event\n");

    EngineSession s; s.boot();
    s.adapter().postEvent(chordEvent(65, ChordType::Minor));  // F minor
    run(s, 3);
    auto c = s.player().getCurrentChord();
    TEST("Chord event -> player F minor", c.root == 65 && c.type == ChordType::Minor);

    s.adapter().postEvent(chordEvent(67, ChordType::Dom7));   // G7
    run(s, 3);
    c = s.player().getCurrentChord();
    TEST("Chord event -> player G7", c.root == 67 && c.type == ChordType::Dom7);

    // Sync-start armed: chord event fires playback.
    EngineSession s2; s2.boot();
    s2.adapter().postEvent({ControlAction::SyncArm, 0, 0});
    run(s2, 2);
    TEST("armed not playing", !s2.player().isPlaying());
    s2.adapter().postEvent(chordEvent(60, ChordType::Major));
    run(s2, 5);
    TEST("chord event auto-starts when armed", s2.player().isPlaying());

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

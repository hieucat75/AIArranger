// Gate 14 Task J — snapshot capture/serialize/restore (headless).

#include "session/snapshot_manager.h"
#include "session/engine_session.h"
#include <cstdio>
#include <string>

using namespace ai_arranger;
using namespace ai_arranger::session;

static int failures = 0, passes = 0;
#define TEST(name, expr) do { \
    if (!(expr)) { std::fprintf(stderr, "  \xE2\x9D\x8C FAIL: %s\n", name); failures++; } \
    else { std::printf("  \xE2\x9C\x85 PASS: %s\n", name); passes++; } \
} while(0)

int main() {
    std::printf("Test: Gate 14 Task J — snapshot manager\n");

    // ── serialize -> deserialize round-trip ──
    {
        SessionSnapshot s;
        s.variation = 2; s.tempo_bpm = 132; s.split_point = 54;
        s.manual_bass = true; s.sync_armed = true; s.chord_scan_mode = 1;
        std::string js = SnapshotManager::serialize(s);
        SessionSnapshot r;
        TEST("deserialize succeeds", SnapshotManager::deserialize(js, r));
        TEST("round-trip equal", r.variation == 2 && r.tempo_bpm == 132 &&
             r.split_point == 54 && r.manual_bass && r.sync_armed &&
             r.chord_scan_mode == 1);
    }

    // ── validation rejects bad input ──
    {
        SessionSnapshot r;
        TEST("reject garbage", !SnapshotManager::deserialize("not json", r));
        TEST("reject bad version",
             !SnapshotManager::deserialize(
                 "{\"version\":99,\"variation\":0,\"tempo_bpm\":120,"
                 "\"split_point\":60,\"manual_bass\":0,\"sync_armed\":0,"
                 "\"chord_scan_mode\":0}", r));
        TEST("reject out-of-range variation",
             !SnapshotManager::deserialize(
                 "{\"version\":1,\"variation\":7,\"tempo_bpm\":120,"
                 "\"split_point\":60,\"manual_bass\":0,\"sync_armed\":0,"
                 "\"chord_scan_mode\":0}", r));
        TEST("reject out-of-range tempo",
             !SnapshotManager::deserialize(
                 "{\"version\":1,\"variation\":0,\"tempo_bpm\":9999,"
                 "\"split_point\":60,\"manual_bass\":0,\"sync_armed\":0,"
                 "\"chord_scan_mode\":0}", r));
    }

    // ── capture from a live session ──
    {
        EngineSession s; s.boot();
        s.clock().setTempo(140);
        s.adapter().split().setSplitPoint(48);
        s.adapter().split().setManualBass(true);
        s.adapter().variation().setImmediate(performance::Variation::C);
        SessionSnapshot snap = SnapshotManager::capture(s, /*scanMode*/ 2);
        TEST("capture tempo", snap.tempo_bpm == 140);
        TEST("capture split", snap.split_point == 48);
        TEST("capture manual bass", snap.manual_bass);
        TEST("capture variation C", snap.variation == 2);
        TEST("capture scan mode index", snap.chord_scan_mode == 2);
    }

    // ── restore into a fresh session ──
    {
        SessionSnapshot snap;
        snap.variation = 3; snap.tempo_bpm = 96; snap.split_point = 55;
        snap.manual_bass = true; snap.sync_armed = true; snap.chord_scan_mode = 1;

        EngineSession s; s.boot();
        TEST("restore succeeds", SnapshotManager::restore(snap, s));
        TEST("restored tempo", s.clock().getTempo() == 96);
        TEST("restored split", s.adapter().split().splitPoint() == 55);
        TEST("restored manual bass", s.adapter().split().manualBass());
        TEST("restored variation D", s.adapter().variation().current() ==
             performance::Variation::D);
        TEST("restored sync armed", s.adapter().sync().isArmed());

        SessionSnapshot bad; bad.version = 42;
        TEST("restore rejects invalid", !SnapshotManager::restore(bad, s));
    }

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

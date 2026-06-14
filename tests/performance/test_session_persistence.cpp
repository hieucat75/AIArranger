// Gate 15 Task G — performer session persistence (disk JSON round-trip).

#include "session/session_persistence.h"
#include <cstdio>
#include <string>

using namespace ai_arranger::session;

static int failures = 0, passes = 0;
#define TEST(name, expr) do { \
    if (!(expr)) { std::fprintf(stderr, "  \xE2\x9D\x8C FAIL: %s\n", name); failures++; } \
    else { std::printf("  \xE2\x9C\x85 PASS: %s\n", name); passes++; } \
} while(0)

int main() {
    std::printf("Test: Gate 15 Task G — session persistence\n");

    PerformerSession s;
    s.style_name = "POP_ACOUSTIC_2";
    s.variation = 2; s.tempo_bpm = 132; s.split_point = 54; s.manual_bass = true;
    s.sync_armed = true; s.chord_scan_mode = 1; s.groove_profile = 3;
    s.midi_output_name = "IAC Bus 1"; s.ui_layout = 1; s.theme = 0;

    // ── serialize -> deserialize ──
    {
        PerformerSession r;
        TEST("deserialize ok", SessionPersistence::deserialize(SessionPersistence::serialize(s), r));
        TEST("round-trip core", r.variation == 2 && r.tempo_bpm == 132 &&
             r.split_point == 54 && r.manual_bass && r.sync_armed && r.chord_scan_mode == 1);
        TEST("round-trip gate15 fields", r.groove_profile == 3 && r.ui_layout == 1 &&
             r.theme == 0);
        TEST("round-trip strings", r.style_name == "POP_ACOUSTIC_2" &&
             r.midi_output_name == "IAC Bus 1");
    }

    // ── save -> load (disk) ──
    {
        const std::string path = "/tmp/g15_session_test.json";
        TEST("save to disk", SessionPersistence::save(s, path));
        PerformerSession r;
        TEST("load from disk", SessionPersistence::load(path, r));
        TEST("loaded matches", r.tempo_bpm == 132 && r.groove_profile == 3 &&
             r.midi_output_name == "IAC Bus 1");
    }

    // ── validation rejects garbage / out-of-range / missing file ──
    {
        PerformerSession r;
        TEST("reject garbage", !SessionPersistence::deserialize("not json", r));
        TEST("reject bad groove_profile",
             !SessionPersistence::deserialize(
                 "{\"version\":1,\"variation\":0,\"tempo_bpm\":120,\"split_point\":60,"
                 "\"manual_bass\":0,\"sync_armed\":0,\"chord_scan_mode\":0,"
                 "\"groove_profile\":9,\"ui_layout\":0,\"theme\":0}", r));
        TEST("reject bad version",
             !SessionPersistence::deserialize(
                 "{\"version\":99,\"variation\":0,\"tempo_bpm\":120,\"split_point\":60,"
                 "\"manual_bass\":0,\"sync_armed\":0,\"chord_scan_mode\":0,"
                 "\"groove_profile\":0,\"ui_layout\":0,\"theme\":0}", r));
        TEST("missing file -> false", !SessionPersistence::load("/tmp/nope_g15_missing.json", r));
    }

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

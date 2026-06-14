// Gate 11 Task G — fixture set + Korg placeholder validation (synthetic).

#include "korg_validation/midi_capture.h"
#include "korg_validation/timing_analyzer.h"
#include "korg_validation/stuck_note_detector.h"
#include <cstdio>
#include <filesystem>
#include <string>

using namespace ai_arranger::korg_validation;
namespace fs = std::filesystem;

static int failures = 0, passes = 0;
#define TEST(name, expr) do { \
    if (!(expr)) { std::fprintf(stderr, "  \xE2\x9D\x8C FAIL: %s\n", name); failures++; } \
    else { std::printf("  \xE2\x9C\x85 PASS: %s\n", name); passes++; } \
} while(0)

#ifndef KORG_FIXTURE_DIR
#define KORG_FIXTURE_DIR "."
#endif
#ifndef KORG_REPO_DIR
#define KORG_REPO_DIR "."
#endif

int main() {
    std::printf("Test: Gate 11 Task G — fixtures + placeholders\n");

    const std::string syn = std::string(KORG_FIXTURE_DIR) + "/synthetic/";

    // ── Synthetic engine/reference fixtures load and pair cleanly ──
    MidiCapture eng = loadCsv(syn + "two_bar_engine.csv");
    MidiCapture ref = loadCsv(syn + "two_bar_reference.csv");
    TEST("engine fixture non-empty", !eng.events.empty());
    TEST("reference fixture non-empty", !ref.events.empty());

    TimingSummary t = analyzeTiming(eng, ref, 120.0);
    TEST("engine vs reference: all matched", t.orphan_engine == 0 && t.orphan_reference == 0);
    TEST("engine vs reference: zero delta", t.max_abs_ms == 0.0);

    StuckResult s = detectStuck(eng);
    TEST("engine fixture is note-balanced", s.balanced && s.hanging == 0);

    // ── Korg hardware-capture placeholders exist (and stay empty) ──
    const std::string korg = std::string(KORG_REPO_DIR) + "/fixtures/korg/";
    TEST("fixtures/korg/README.md exists", fs::exists(korg + "README.md"));
    TEST("PA700 placeholder dir exists", fs::is_directory(korg + "PA700"));
    TEST("PA1000 placeholder dir exists", fs::is_directory(korg + "PA1000"));

    // No real captures yet — only .gitkeep should be present (honest status).
    int pa700Captures = 0;
    std::error_code ec;
    for (const auto& e : fs::directory_iterator(korg + "PA700", ec)) {
        const std::string fn = e.path().filename().string();
        if (fn != ".gitkeep") ++pa700Captures;
    }
    TEST("PA700 has no real captures yet (synthetic-only)", pa700Captures == 0);

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

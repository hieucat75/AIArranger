// Chord latch + hold-mode (chord-engine-spec v0.9 §6), headless + deterministic.
// Hold ON (default) keeps the last chord on release. Hold OFF arms an ~80 ms
// release debounce: the chord survives a brief release / re-fingering and only
// clears to NoChord once the keys stay up past the window.

#include "session/engine_session.h"
#include "engine/chord/fingered_detector.h"
#include "engine/arranger/chord_input.h"
#include <cstdio>

using namespace ai_arranger;
using session::EngineSession;
using arranger::ChordType;

static int failures = 0, passes = 0;
#define TEST(name, expr) do { \
    if (!(expr)) { std::fprintf(stderr, "  FAIL: %s\n", name); failures++; } \
    else { std::printf("  PASS: %s\n", name); passes++; } \
} while(0)

static void pressCEG(EngineSession& s) { s.adapter().noteOn(48); s.adapter().noteOn(52); s.adapter().noteOn(55); }
static void releaseCEG(EngineSession& s) { s.adapter().noteOff(48); s.adapter().noteOff(52); s.adapter().noteOff(55); }

int main() {
    std::printf("Test: chord latch + hold-mode\n");
    chord::FingeredDetector det;

    // ── Hold mode ON (default): release keeps the last chord ──
    {
        EngineSession s; s.boot();
        s.adapter().setChordScanMode(&det);
        TEST("default hold-mode ON", s.adapter().holdMode());
        pressCEG(s);
        TEST("hold: C major detected", s.player().getCurrentChord().type == ChordType::Major);
        releaseCEG(s);
        s.clock().advance(48000 /* 1s */); s.tick();
        TEST("hold: chord kept after full release", s.player().getCurrentChord().type == ChordType::Major);
    }

    // ── Hold mode OFF: latch debounces, then clears ──
    {
        EngineSession s; s.boot();               // clock default 48 kHz
        s.clock().start();                        // clock advances while running (as it does under playback)
        s.adapter().setChordScanMode(&det);
        s.adapter().setHoldMode(false);
        s.adapter().setChordLatchMs(80);          // 80 ms = 3840 samples @ 48k

        pressCEG(s);
        TEST("hold-off: C major detected", s.player().getCurrentChord().type == ChordType::Major);

        // Release, then tick within the window -> chord still held.
        releaseCEG(s);
        s.clock().advance(2000);                  // ~42 ms < 80 ms
        s.tick();
        TEST("latch: chord held within window", s.player().getCurrentChord().type == ChordType::Major);

        // Re-press within the window cancels the latch (no flicker).
        pressCEG(s);
        s.clock().advance(1000);
        s.tick();
        TEST("latch: re-press keeps chord", s.player().getCurrentChord().type == ChordType::Major);

        // Release and let the window elapse -> chord clears to NoChord.
        releaseCEG(s);
        s.clock().advance(5000);                  // > 80 ms past the arm point
        s.tick();
        TEST("latch: chord cleared after window", s.player().getCurrentChord().type == ChordType::NoChord);
    }

    // ── A new valid chord always applies immediately (latch never delays it) ──
    {
        EngineSession s; s.boot();
        s.adapter().setChordScanMode(&det);
        s.adapter().setHoldMode(false);
        pressCEG(s);
        s.tick();
        // Switch to F major below the split (F2 A2 C3 = 41 45 48).
        s.adapter().noteOff(48); s.adapter().noteOff(52); s.adapter().noteOff(55);
        s.adapter().noteOn(41); s.adapter().noteOn(45); s.adapter().noteOn(48);
        s.tick();
        auto c = s.player().getCurrentChord();
        TEST("new chord applies immediately (F major root 41)", c.type == ChordType::Major && c.root == 41);
    }

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

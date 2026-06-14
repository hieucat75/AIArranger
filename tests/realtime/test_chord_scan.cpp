// Gate 12 Task E — chord scan modes (deterministic, synthetic fingerings).

#include "engine/chord/fingered_detector.h"
#include "engine/chord/fingered_on_bass_detector.h"
#include "engine/chord/single_finger_detector.h"
#include "engine/chord/ai_fingered_placeholder.h"
#include <cstdio>
#include <string>
#include <vector>

using namespace ai_arranger::chord;
using ai_arranger::arranger::ChordType;

static int failures = 0, passes = 0;
#define TEST(name, expr) do { \
    if (!(expr)) { std::fprintf(stderr, "  \xE2\x9D\x8C FAIL: %s\n", name); failures++; } \
    else { std::printf("  \xE2\x9C\x85 PASS: %s\n", name); passes++; } \
} while(0)

static Chord det(const IChordScanMode& m, std::vector<uint8_t> n) {
    return m.detect(n.data(), n.size());
}

int main() {
    std::printf("Test: Gate 12 Task E — chord scan modes\n");

    FingeredDetector fing;
    FingeredOnBassDetector onbass;
    SingleFingerDetector single;
    AIFingeredPlaceholder ai;

    // ── Fingered triads / 7ths ──
    {
        Chord c = det(fing, {60, 64, 67});           // C E G
        TEST("Fingered C major", c.type == ChordType::Major && c.root == 60);
        c = det(fing, {60, 63, 67});                 // C Eb G
        TEST("Fingered C minor", c.type == ChordType::Minor && c.root == 60);
        c = det(fing, {62, 66, 69, 72});             // D F# A C
        TEST("Fingered D7", c.type == ChordType::Dom7 && c.root == 62);
        c = det(fing, {60, 64, 67, 71});             // C E G B
        TEST("Fingered Cmaj7", c.type == ChordType::Maj7);
        c = det(fing, {60});                         // single note
        TEST("Fingered single note -> NoChord", c.type == ChordType::NoChord);
    }

    // ── Fingered-on-bass (slash) ──
    {
        // E bass + C E G above -> C major / E
        Chord c = det(onbass, {52, 60, 64, 67});
        TEST("on-bass: C major over E", c.type == ChordType::Major &&
             c.root == 60 && c.bass == 52);
        // plain triad -> bass = lowest
        c = det(onbass, {60, 64, 67});
        TEST("on-bass: bass = lowest note", c.bass == 60);
    }

    // ── Single-finger convention ──
    {
        TEST("single: 1 note -> Major", det(single, {60}).type == ChordType::Major);
        TEST("single: root+min3 -> Minor", det(single, {60, 63}).type == ChordType::Minor);
        TEST("single: root+b7 -> Dom7", det(single, {60, 70}).type == ChordType::Dom7);
        TEST("single: root+5th -> Power", det(single, {60, 67}).type == ChordType::Power);
    }

    // ── AI placeholder == Fingered fallback (documented, no model) ──
    {
        Chord a = det(ai, {60, 64, 67});
        Chord f = det(fing, {60, 64, 67});
        TEST("AI placeholder falls back to Fingered",
             a.type == f.type && a.root == f.root);
        TEST("AI placeholder name flags placeholder",
             std::string(ai.name()) == "ai-fingered-placeholder");
    }

    // ── Determinism: same input twice -> identical ──
    {
        Chord a = det(fing, {65, 69, 72});
        Chord b = det(fing, {65, 69, 72});
        TEST("deterministic detect", a.type == b.type && a.root == b.root);
    }

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

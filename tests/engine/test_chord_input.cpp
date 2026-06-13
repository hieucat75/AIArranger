#include "engine/arranger/chord_input.h"
#include <cstdio>

using namespace ai_arranger::arranger;

static int failures = 0;
static int passes = 0;

#define TEST(name, expr) do { \
    if (!(expr)) { std::fprintf(stderr, "  ❌ FAIL: %s\n", name); failures++; } \
    else { std::printf("  ✅ PASS: %s\n", name); passes++; } \
} while(0)

int main() {
    std::printf("Test: ChordInput\n");

    ChordInput ci;

    // Default chord
    Chord c = ci.getChord();
    TEST("Default chord type = Major", c.type == ChordType::Major);
    TEST("Default chord root = 60 (C)", c.root == 60);

    // Set chord
    Chord dm = {ChordType::Minor, 62, 0, 0};
    ci.setChord(dm);
    c = ci.getChord();
    TEST("Set Dm chord type = Minor", c.type == ChordType::Minor);
    TEST("Set Dm chord root = 62 (D)", c.root == 62);

    // Chord pack/unpack roundtrip
    Chord input = {ChordType::Dom7, 65, 0, 0};
    ci.setChord(input);
    Chord output = ci.getChord();
    TEST("Dom7 F root = 65", output.root == 65);
    TEST("Dom7 type preserved", output.type == ChordType::Dom7);

    // Constants
    TEST("CMaj root = 60", chords::CMaj.root == 60);
    TEST("CMaj type = Major", chords::CMaj.type == ChordType::Major);
    TEST("AMin root = 69", chords::AMin.root == 69);
    TEST("AMin type = Minor", chords::AMin.type == ChordType::Minor);

    // Progression
    Chord progression[] = {chords::CMaj, chords::GMaj, chords::AMin, chords::FMaj};
    ci.setProgression(progression, 4);
    Chord first = ci.nextProgressionChord();
    TEST("Progression first = CMaj", first.root == 60 && first.type == ChordType::Major);
    Chord second = ci.nextProgressionChord();
    TEST("Progression second = GMaj", second.root == 67 && second.type == ChordType::Major);
    ci.nextProgressionChord(); // skip AMin
    ci.nextProgressionChord(); // skip FMaj
    Chord wrapped = ci.nextProgressionChord();
    TEST("Progression wraps to CMaj", wrapped.root == 60);

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

// Gate 10B Task E — Guitar NTT + MegaVoice fallback.
//
// 1. Guitar NTT (NttMode::Guitar) voices recorded notes onto target chord
//    tones, with NtrRule::Guitar handling the root (falls back to root
//    transposition for v1).
// 2. MegaVoice graceful degrade: KeyswitchRenderer(false) passes notes through
//    unchanged when the target can't honour keyswitch articulation.

#include "engine/music/ntt.h"
#include "engine/articulation/renderer.h"
#include "engine/arranger/chord_input.h"
#include <cstdio>
#include <vector>

using namespace ai_arranger;
using namespace ai_arranger::music;
using namespace ai_arranger::articulation;
using arranger::Chord;
using arranger::ChordType;

static int failures = 0;
static int passes = 0;

#define TEST(name, expr) do { \
    if (!(expr)) { std::fprintf(stderr, "  \xE2\x9D\x8C FAIL: %s\n", name); failures++; } \
    else { std::printf("  \xE2\x9C\x85 PASS: %s\n", name); passes++; } \
} while(0)

#define TEST_EQ(name, got, want) do { \
    int g = (int)(got), w = (int)(want); \
    if (g != w) { std::fprintf(stderr, "  \xE2\x9D\x8C FAIL: %s (got %d want %d)\n", name, g, w); failures++; } \
    else { std::printf("  \xE2\x9C\x85 PASS: %s\n", name); passes++; } \
} while(0)

static Chord maj(uint8_t root) { return {ChordType::Major, root, 0, 0}; }
static Chord min(uint8_t root) { return {ChordType::Minor, root, 0, 0}; }

struct CollectSink final : EventSink {
    std::vector<uasf::MidiEvent> events;
    void emit(const uasf::MidiEvent& ev) noexcept override { events.push_back(ev); }
};

// Is `note` a chord tone of `chord` (any octave)?
static bool isChordTone(int note, Chord c) {
    int pc = ((note - c.root) % 12 + 12) % 12;
    if (c.type == ChordType::Major) return pc == 0 || pc == 4 || pc == 7;
    if (c.type == ChordType::Minor) return pc == 0 || pc == 3 || pc == 7;
    return false;
}

int main() {
    std::printf("Test: Gate 10B Task E — Guitar NTT + MegaVoice fallback\n");

    // ── Guitar NTT voices every note onto a target chord tone ──
    // Recorded root C4 (60) over F major -> an F-major chord tone.
    {
        uint8_t out = transpose(60, maj(65), NtrRule::Guitar, NttMode::Guitar);
        TEST("Guitar: C over F renders an F-chord tone", isChordTone(out, maj(65)));
        TEST_EQ("Guitar: root C4 over F -> F (65)", out, 65);
    }
    // Recorded major 3rd E4 (64) over C minor must voice the minor 3rd (Eb).
    {
        uint8_t out = transpose(64, min(60), NtrRule::Guitar, NttMode::Guitar);
        TEST("Guitar: maj-3rd over Cmin -> chord tone", isChordTone(out, min(60)));
        TEST_EQ("Guitar: E4 over Cmin -> Eb4 (63)", out, 63);
    }
    // A non-chord tone (D4=62, the 2nd) snaps onto a chord tone.
    {
        uint8_t out = transpose(62, maj(60), NtrRule::Guitar, NttMode::Guitar);
        TEST("Guitar: non-chord tone snaps to a chord tone",
             isChordTone(out, maj(60)));
    }
    // NtrRule::Guitar root handling tracks the chord root across a progression.
    {
        TEST_EQ("Guitar root: C over G -> G chord tone present",
                isChordTone(transpose(60, maj(67), NtrRule::Guitar, NttMode::Guitar),
                            maj(67)) ? 1 : 0, 1);
    }

    // ── MegaVoice fallback: keyswitch unavailable -> pass-through ──
    uasf::ArticulationMetadata ks{};
    ks.has_keyswitch = true;
    ks.keyswitch_note = 24;
    uasf::MidiEvent note{uasf::MidiEventType::NoteOn, 0, 60, 100, 480};

    {
        KeyswitchRenderer available(true);
        CollectSink s;
        available.render(note, ks, s);
        TEST("Keyswitch available -> 3 events (ks on/off + main)",
             s.events.size() == 3);
        TEST("Available renderer name = keyswitch",
             std::string(available.name()) == "keyswitch");
    }
    {
        KeyswitchRenderer fallback(false);     // MegaVoice degrade
        CollectSink s;
        fallback.render(note, ks, s);
        TEST("MegaVoice fallback -> 1 event (note only, no keyswitch)",
             s.events.size() == 1 && s.events[0].data1 == 60);
        TEST("Fallback renderer name flags degradation",
             std::string(fallback.name()) == "keyswitch-fallback");
        TEST("Fallback exposes keyswitchAvailable()=false",
             fallback.keyswitchAvailable() == false);
    }

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

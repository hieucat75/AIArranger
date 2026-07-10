// End-to-end: raw MIDI input bytes -> router -> PerformerAdapter (lock-free) ->
// chord detection -> StylePlayer. Headless (no CoreMIDI). Proves the missing MIDI
// INPUT half of MIDI I/O drives the existing chord engine, and that CC64 sustain
// defers note releases. All state changes drain inside tick().

#include "session/engine_session.h"
#include "midi/midi_input_router.h"
#include "engine/chord/fingered_detector.h"
#include "engine/arranger/chord_input.h"
#include <cstdio>

using namespace ai_arranger;
using ai_arranger::session::EngineSession;
using ai_arranger::arranger::ChordType;

static int failures = 0, passes = 0;
#define TEST(name, expr) do { \
    if (!(expr)) { std::fprintf(stderr, "  FAIL: %s\n", name); failures++; } \
    else { std::printf("  PASS: %s\n", name); passes++; } \
} while(0)

// Feed a raw MIDI buffer through the router into the adapter, then drain.
static void feed(EngineSession& s, const uint8_t* b, size_t len) {
    midi::routeMidiBytes(b, len, s.adapter());
    s.tick();
}

int main() {
    std::printf("Test: MIDI input bytes -> chord (end to end)\n");

    EngineSession s; s.boot();
    chord::FingeredDetector detector;
    s.adapter().setChordScanMode(&detector);   // split point defaults to C4 (60)

    // ── C major triad below the split: C3=48, E3=52, G3=55 ──
    {
        const uint8_t on[] = {0x90, 48, 100,  0x90, 52, 100,  0x90, 55, 100};
        feed(s, on, sizeof(on));
        auto c  = s.adapter().lastChord();
        auto pc = s.player().getCurrentChord();
        TEST("three notes held", s.adapter().heldCount() == 3);
        TEST("detected C major (root 48)", c.type == ChordType::Major && c.root == 48);
        TEST("chord reached the player", pc.type == ChordType::Major && pc.root == 48);
    }

    // ── Sustain pedal down (CC64=127): releasing the keys keeps the notes ──
    {
        const uint8_t pedalDown[] = {0xB0, 64, 127};
        feed(s, pedalDown, sizeof(pedalDown));
        const uint8_t off[] = {0x80, 48, 0,  0x80, 52, 0,  0x80, 55, 0};
        feed(s, off, sizeof(off));
        TEST("sustain flag on", s.adapter().sustain());
        TEST("sustain holds the 3 notes", s.adapter().heldCount() == 3);
    }

    // ── Pedal up (CC64=0): deferred releases apply at once ──
    {
        const uint8_t pedalUp[] = {0xB0, 64, 0};
        feed(s, pedalUp, sizeof(pedalUp));
        TEST("sustain flag off", !s.adapter().sustain());
        TEST("pedal up releases all", s.adapter().heldCount() == 0);
    }

    // ── Without the pedal, a normal release empties the held set ──
    {
        const uint8_t on[]  = {0x90, 48, 100,  0x90, 52, 100,  0x90, 55, 100};
        feed(s, on, sizeof(on));
        TEST("re-press holds 3", s.adapter().heldCount() == 3);
        const uint8_t off[] = {0x80, 48, 0,  0x80, 52, 0,  0x80, 55, 0};
        feed(s, off, sizeof(off));
        TEST("normal release clears held", s.adapter().heldCount() == 0);
    }

    // ── Upper-zone note (C5=72, above the split) is ignored for chord ──
    {
        const uint8_t hi[] = {0x90, 72, 100};
        feed(s, hi, sizeof(hi));
        TEST("upper-zone note ignored", s.adapter().heldCount() == 0);
    }

    // ── NoteOn velocity 0 (running-status keyboards) counts as release ──
    {
        const uint8_t on[]  = {0x90, 48, 100,  0x90, 52, 100,  0x90, 55, 100};
        feed(s, on, sizeof(on));
        TEST("held before vel0 release", s.adapter().heldCount() == 3);
        const uint8_t vel0[] = {0x90, 48, 0,  0x90, 52, 0,  0x90, 55, 0};
        feed(s, vel0, sizeof(vel0));
        TEST("vel0 releases held", s.adapter().heldCount() == 0);
    }

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

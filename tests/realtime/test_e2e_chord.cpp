// Gate 13 Task E — ChordScanMode <-> StylePlayer end-to-end.

#include "engine/integration/performer_adapter.h"
#include "engine/arranger/style_player.h"
#include "engine/realtime/clock.h"
#include "engine/midi/scheduler.h"
#include "engine/midi/panic.h"
#include "engine/chord/fingered_detector.h"
#include "engine/chord/fingered_on_bass_detector.h"
#include "engine/chord/single_finger_detector.h"
#include "engine/chord/ai_fingered_placeholder.h"
#include "engine/arranger/chord_input.h"
#include <cstdio>

using namespace ai_arranger;
using namespace ai_arranger::integration;
using arranger::Chord;
using arranger::ChordType;

static int failures = 0, passes = 0;
#define TEST(name, expr) do { \
    if (!(expr)) { std::fprintf(stderr, "  \xE2\x9D\x8C FAIL: %s\n", name); failures++; } \
    else { std::printf("  \xE2\x9C\x85 PASS: %s\n", name); passes++; } \
} while(0)

struct Rig {
    realtime::RealtimeClock clock; midi::MidiScheduler scheduler; midi::PanicHandler panic;
    arranger::StylePlayer player{clock, scheduler, panic};
    PerformerAdapter adapter{clock, player};
    Rig() {
        player.loadStyle(arranger::buildDemoStyle());
        clock.setSampleRate(48000); clock.setResolution(480); clock.setTempo(120);
        adapter.setSplitPoint(127);  // whole keyboard = lower zone
    }
    void allOff() { for (int n = 0; n < 128; ++n) adapter.noteOff((uint8_t)n); }
};

int main() {
    std::printf("Test: Gate 13 Task E — chord scan E2E\n");

    chord::FingeredDetector fingered;
    chord::FingeredOnBassDetector onbass;
    chord::SingleFingerDetector single;
    chord::AIFingeredPlaceholder ai;

    // ── Fingered -> StylePlayer chord ──
    {
        Rig r; r.adapter.setChordScanMode(&fingered);
        r.adapter.noteOn(60); r.adapter.noteOn(64); r.adapter.noteOn(67);
        Chord c = r.player.getCurrentChord();
        TEST("Fingered C major -> player chord", c.type == ChordType::Major && c.root == 60);
        r.allOff();
        r.adapter.noteOn(65); r.adapter.noteOn(69); r.adapter.noteOn(72);
        c = r.player.getCurrentChord();
        TEST("Fingered F major -> player chord", c.type == ChordType::Major && c.root == 65);
    }

    // ── Fingered-on-bass ──
    {
        Rig r; r.adapter.setChordScanMode(&onbass);
        r.adapter.noteOn(52); r.adapter.noteOn(60); r.adapter.noteOn(64); r.adapter.noteOn(67);
        Chord c = r.player.getCurrentChord();
        TEST("on-bass C/E -> Major root 60", c.type == ChordType::Major && c.root == 60);
    }

    // ── Single-finger ──
    {
        Rig r; r.adapter.setChordScanMode(&single);
        r.adapter.noteOn(60);
        TEST("single 1 note -> Major", r.player.getCurrentChord().type == ChordType::Major);
        r.allOff();
        r.adapter.noteOn(60); r.adapter.noteOn(63);
        TEST("single root+min3 -> Minor", r.player.getCurrentChord().type == ChordType::Minor);
    }

    // ── AI placeholder == Fingered ──
    {
        Rig r; r.adapter.setChordScanMode(&ai);
        r.adapter.noteOn(60); r.adapter.noteOn(64); r.adapter.noteOn(67);
        Chord c = r.player.getCurrentChord();
        TEST("AI placeholder -> Major root 60", c.type == ChordType::Major && c.root == 60);
    }

    // ── Chord change drives transpose (bass follows root) ──
    {
        Rig r; r.adapter.setChordScanMode(&fingered);
        int dispatchedAfterF = 0; bool sawF = false;
        r.scheduler.setOutputCallback([&](const uasf::MidiEvent& e){
            if (sawF && (e.type==uasf::MidiEventType::NoteOn) && e.data2>0) {
                ++dispatchedAfterF;
            }
        });
        r.adapter.postEvent({control::ControlAction::Start, 0, 0});
        // C major
        r.adapter.noteOn(60); r.adapter.noteOn(64); r.adapter.noteOn(67);
        for (int i=0;i<400;++i){ r.adapter.tick(); r.clock.advance(256); r.scheduler.advanceTo(r.clock.getPosition()); }
        // switch to F major
        r.allOff(); r.adapter.noteOn(65); r.adapter.noteOn(69); r.adapter.noteOn(72);
        sawF = true;
        for (int i=0;i<400;++i){ r.adapter.tick(); r.clock.advance(256); r.scheduler.advanceTo(r.clock.getPosition()); }
        TEST("transpose active after chord change (notes dispatched)", dispatchedAfterF > 0);
        TEST("player chord is F after change", r.player.getCurrentChord().root == 65);
    }

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

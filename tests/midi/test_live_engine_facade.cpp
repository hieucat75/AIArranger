// LiveEngineFacade end-to-end (headless, no CoreMIDI): a fake MIDI input source
// feeds chords in, a fake MIDI output provider captures what goes out, and the UI
// reads an atomic snapshot. Proves the full portable live loop + the no-stuck-
// notes invariant on stop.

#include "session/live_engine_facade.h"
#include "engine/chord/fingered_detector.h"
#include "engine/arranger/chord_input.h"
#include "fake_midi_input_source.h"
#include "fake_midi_output_provider.h"
#include <cstdio>

using namespace ai_arranger;
using session::LiveEngineFacade;
using arranger::ChordType;

static int failures = 0, passes = 0;
#define TEST(name, expr) do { \
    if (!(expr)) { std::fprintf(stderr, "  FAIL: %s\n", name); failures++; } \
    else { std::printf("  PASS: %s\n", name); passes++; } \
} while(0)

int main() {
    std::printf("Test: LiveEngineFacade end-to-end\n");

    midi::FakeMidiInputSource  fin;  fin.devices = {{0, "keyboard"}};
    midi::FakeMidiOutputProvider fout; fout.setDevices({{0, "synth"}});

    LiveEngineFacade fac(&fin, &fout);
    fac.start();
    TEST("select input",  fac.selectMidiInput(0));
    TEST("select output", fac.selectMidiOutput(0));
    chord::FingeredDetector det;
    fac.setChordScanMode(&det);

    // ── Input bytes -> chord -> snapshot ──
    const uint8_t on[] = {0x90, 48, 100,  0x90, 52, 100,  0x90, 55, 100}; // C3-E3-G3
    fin.inject(on, sizeof(on));
    fac.tick(48);
    auto s = fac.snapshot();
    TEST("snapshot: C major root 48", s.chordType == static_cast<uint8_t>(ChordType::Major) && s.chordRoot == 48);
    TEST("snapshot: midi in live", s.midiInLive);
    TEST("snapshot: midi out live", s.midiOutLive);
    TEST("snapshot: received messages", s.receivedMessages > 0);

    // ── Tempo command reflected ──
    fac.setTempo(100);
    fac.tick(48);
    TEST("snapshot: tempo applied", fac.snapshot().tempoBpm == 100);

    // ── Transport start -> playing -> output flows ──
    fac.transportStart();
    for (int i = 0; i < 400; ++i) fac.tick(48);   // ~400ms of engine time
    s = fac.snapshot();
    TEST("snapshot: playing", s.playing);
    TEST("output dispatched some events", s.dispatchedNotes > 0);
    TEST("provider captured events", !fout.sent.empty());

    // ── Stop -> flush -> no stuck notes ──
    fac.transportStop();
    for (int i = 0; i < 32; ++i) fac.tick(48);     // let note-offs flush out
    s = fac.snapshot();
    TEST("snapshot: stopped", !s.playing);

    int balance = 0;   // +1 per NoteOn(vel>0), -1 per NoteOff / NoteOn(vel0)
    for (const auto& e : fout.sent) {
        if (e.type == uasf::MidiEventType::NoteOn && e.data2 > 0) ++balance;
        else if (e.type == uasf::MidiEventType::NoteOff ||
                 (e.type == uasf::MidiEventType::NoteOn && e.data2 == 0)) --balance;
    }
    TEST("no stuck notes after stop (NoteOn == NoteOff)", balance == 0);

    // ── Panic path: dispatches all-notes-off so the device kills every voice ──
    // Panic uses CC120 (AllSoundOff) + CC123 (AllNotesOff) on all channels and
    // clears the scheduler (standard, efficient panic) rather than emitting one
    // NoteOff per sounding note, so we assert the all-notes-off control reached
    // the output, not a NoteOn/NoteOff balance.
    fac.transportStart();
    fin.inject(on, sizeof(on));
    for (int i = 0; i < 100; ++i) fac.tick(48);
    const size_t beforePanic = fout.sent.size();
    fac.panic();
    for (int i = 0; i < 16; ++i) fac.tick(48);
    bool sawAllNotesOff = false;
    for (size_t i = beforePanic; i < fout.sent.size(); ++i) {
        const auto& e = fout.sent[i];
        if (e.type == uasf::MidiEventType::ControlChange && (e.data1 == 123 || e.data1 == 120)) {
            sawAllNotesOff = true; break;
        }
    }
    TEST("panic dispatched all-notes-off (CC120/123)", sawAllNotesOff);

    fac.stop();

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

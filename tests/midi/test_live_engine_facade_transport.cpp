// Gate 2 — LiveEngineFacade section/transport behaviour, headless (no CoreMIDI).
//
// Drives the facade with a synthetic multi-section style and asserts the live
// performer transitions the reference host needs:
//   - Start begins the intro immediately (bar 1, no silent bar)
//   - Intro → Variation, and every section change lands ONLY on a bar boundary
//     (a mid-bar command is held until the boundary — musical quantise)
//   - Variation → Fill → a different Variation
//   - Break, Ending
//   - Stop leaves no stuck notes
//   - Sync Start arms; the first detected chord fires playback
//   - Panic dispatches all-notes-off and stops
//   - A tempo change mid-playback does not break scheduling (output keeps
//     flowing, position keeps advancing, notes stay balanced)
//
// Section identity is read from snapshot().section (the sequencer's current
// index); timing from snapshot().positionTicks. Deterministic, no sleeps.

#include "session/live_engine_facade.h"
#include "engine/chord/fingered_detector.h"
#include "engine/arranger/chord_input.h"
#include "engine/uasf/types.h"
#include "fake_midi_input_source.h"
#include "fake_midi_output_provider.h"
#include <cstdio>
#include <cstdint>

using namespace ai_arranger;
using session::LiveEngineFacade;

static int failures = 0, passes = 0;
#define TEST(name, expr) do { \
    if (!(expr)) { std::fprintf(stderr, "  FAIL: %s\n", name); failures++; } \
    else { std::printf("  PASS: %s\n", name); passes++; } \
} while(0)

// ── Synthetic style ─────────────────────────────────────────────────────────
// 6 sections at fixed indices so we can assert exactly which one is active:
//   0 Intro1 | 1 Main1 | 2 Main2 | 3 Fill1 | 4 Break | 5 Ending1
// Each 2 bars, resolution 480, 4/4 -> 1920 ticks/bar. Every section carries a
// drum track (ch 9) plus a pitched chord track (ch 2) so playback always has
// output and sounding notes to flush/panic.
static constexpr int kTicksPerBar = 1920;   // 480 * 4 (matches EngineSession boot)

static uasf::MidiEvent ev(uasf::MidiEventType t, uint8_t ch, uint8_t d1,
                          uint8_t d2, uint64_t tick) {
    return uasf::MidiEvent{t, ch, d1, d2, tick};
}

static uasf::TrackDefinition drumTrack() {
    uasf::TrackDefinition t;
    t.name = "Drums"; t.midi_channel = 9; t.role = uasf::TrackRole::Drum; t.is_drum = true;
    for (int bar = 0; bar < 2; ++bar) {
        const int64_t b = bar * kTicksPerBar;
        for (int beat = 0; beat < 4; ++beat) {
            const int64_t on = b + beat * 480;
            t.events.push_back(ev(uasf::MidiEventType::NoteOn, 9, 36, 100, on));
            t.events.push_back(ev(uasf::MidiEventType::NoteOff, 9, 36, 0, on + 200));
        }
    }
    return t;
}

static uasf::TrackDefinition chordTrack() {
    uasf::TrackDefinition t;
    t.name = "Chord"; t.midi_channel = 2; t.role = uasf::TrackRole::Chord; t.is_drum = false;
    // Held triad, re-voiced each bar (gives sounding notes across the whole bar).
    for (int bar = 0; bar < 2; ++bar) {
        const int64_t b = bar * kTicksPerBar;
        for (uint8_t n : {60, 64, 67}) {
            t.events.push_back(ev(uasf::MidiEventType::NoteOn, 2, n, 80, b + 10));
            t.events.push_back(ev(uasf::MidiEventType::NoteOff, 2, n, 0, b + kTicksPerBar - 10));
        }
    }
    return t;
}

static uasf::SectionDefinition section(uasf::SectionType type, const char* name) {
    uasf::SectionDefinition s;
    s.type = type; s.name = name; s.bars = 2; s.resolution = 480;
    s.beats_per_bar = 4; s.beat_note = 4;
    s.tracks.push_back(drumTrack());
    s.tracks.push_back(chordTrack());
    return s;
}

static uasf::StyleDefinition buildStyle() {
    uasf::StyleDefinition st;
    st.name = "Transport Test Style"; st.format_version = "1.0";
    st.tempo_bpm = 120; st.resolution = 480;
    st.sections.push_back(section(uasf::SectionType::Intro1,  "Intro"));   // 0
    st.sections.push_back(section(uasf::SectionType::Main1,   "Main A"));  // 1
    st.sections.push_back(section(uasf::SectionType::Main2,   "Main B"));  // 2
    st.sections.push_back(section(uasf::SectionType::Fill1,   "Fill"));    // 3
    st.sections.push_back(section(uasf::SectionType::Break,   "Break"));   // 4
    st.sections.push_back(section(uasf::SectionType::Ending1, "Ending"));  // 5
    return st;
}

// ── Tick helpers ────────────────────────────────────────────────────────────
// Small steps so a command posted mid-bar is observed BEFORE the boundary.
static void stepSmall(LiveEngineFacade& f, int calls) {
    for (int i = 0; i < calls; ++i) f.tick(256);   // ~5.12 ticks/call at 120bpm/480
}

// Advance until we cross into the next bar (triggering any queued switch).
static void crossBar(LiveEngineFacade& f) {
    const int64_t startBar = f.snapshot().positionTicks / kTicksPerBar;
    for (int i = 0; i < 40000; ++i) {
        f.tick(256);
        if (f.snapshot().positionTicks / kTicksPerBar > startBar) return;
    }
}

static int noteBalance(const midi::FakeMidiOutputProvider& out) {
    int balance = 0;
    for (const auto& e : out.sent) {
        if (e.type == uasf::MidiEventType::NoteOn && e.data2 > 0) ++balance;
        else if (e.type == uasf::MidiEventType::NoteOff ||
                 (e.type == uasf::MidiEventType::NoteOn && e.data2 == 0)) --balance;
    }
    return balance;
}

int main() {
    std::printf("Test: LiveEngineFacade section/transport\n");
    const uasf::StyleDefinition style = buildStyle();

    // ─────────────────────────────────────────────────────────────────────────
    // Main walk-through: start -> intro -> variation -> fill -> variation ->
    // break -> ending, asserting bar-boundary quantisation at each step.
    // ─────────────────────────────────────────────────────────────────────────
    {
        midi::FakeMidiInputSource   fin;  fin.devices = {{0, "kbd"}};
        midi::FakeMidiOutputProvider fout; fout.setDevices({{0, "synth"}});
        LiveEngineFacade fac(&fin, &fout);
        fac.loadStyle(style);
        fac.start();
        fac.selectMidiInput(0);
        fac.selectMidiOutput(0);
        chord::FingeredDetector det;
        fac.setChordScanMode(&det);
        // Variation A->Main1(1), B->Main2(2), C->Main1(1), D->Main2(2).
        fac.session().adapter().setVariationSections(1, 2, 1, 2);

        // Start: intro should activate immediately (index 0), no silent bar.
        fac.transportStart();
        stepSmall(fac, 4);
        TEST("start -> playing", fac.snapshot().playing);
        TEST("start begins on intro (section 0)", fac.snapshot().section == 0);

        // Intro -> Variation A (Main1): command mid-bar is held until boundary.
        crossBar(fac);                       // land at a fresh bar start
        fac.setVariation(0);                 // queue Main1
        stepSmall(fac, 3);                   // still early in the bar
        TEST("variation held mid-bar (still intro)", fac.snapshot().section == 0);
        crossBar(fac);                       // cross boundary -> apply
        TEST("intro -> variation A applies on bar boundary (section 1)",
             fac.snapshot().section == 1);

        // Variation -> Fill (index 3) on boundary.
        fac.fill();
        crossBar(fac);
        TEST("variation -> fill on boundary (section 3)", fac.snapshot().section == 3);

        // Fill -> a DIFFERENT variation (B -> Main2, index 2).
        fac.setVariation(1);
        crossBar(fac);
        TEST("fill -> different variation B on boundary (section 2)",
             fac.snapshot().section == 2);

        // Break (index 4).
        fac.breakSection();
        crossBar(fac);
        TEST("break applies on boundary (section 4)", fac.snapshot().section == 4);

        // Ending (index 5).
        fac.ending();
        crossBar(fac);
        TEST("ending applies on boundary (section 5)", fac.snapshot().section == 5);

        // Stop -> flush -> no stuck notes.
        fac.transportStop();
        for (int i = 0; i < 32; ++i) fac.tick(256);
        TEST("stop -> not playing", !fac.snapshot().playing);
        TEST("stop leaves no stuck notes", noteBalance(fout) == 0);
        fac.stop();
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Sync Start: arm, then the first detected chord fires playback.
    // ─────────────────────────────────────────────────────────────────────────
    {
        midi::FakeMidiInputSource   fin;  fin.devices = {{0, "kbd"}};
        midi::FakeMidiOutputProvider fout; fout.setDevices({{0, "synth"}});
        LiveEngineFacade fac(&fin, &fout);
        fac.loadStyle(style);
        fac.start();
        fac.selectMidiInput(0);
        fac.selectMidiOutput(0);
        chord::FingeredDetector det;
        fac.setChordScanMode(&det);

        fac.syncStart();
        stepSmall(fac, 4);
        TEST("sync start: not playing until a chord arrives", !fac.snapshot().playing);

        const uint8_t on[] = {0x90, 48, 100, 0x90, 52, 100, 0x90, 55, 100}; // C-E-G
        fin.inject(on, sizeof(on));
        stepSmall(fac, 4);
        TEST("sync start: first chord fires playback", fac.snapshot().playing);
        TEST("sync start: chord detected (C major root 48)",
             fac.snapshot().chordRoot == 48);
        fac.transportStop();
        for (int i = 0; i < 32; ++i) fac.tick(256);
        TEST("sync start: no stuck notes after stop", noteBalance(fout) == 0);
        fac.stop();
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Panic mid-playback: dispatches all-notes-off and stops.
    // ─────────────────────────────────────────────────────────────────────────
    {
        midi::FakeMidiInputSource   fin;  fin.devices = {{0, "kbd"}};
        midi::FakeMidiOutputProvider fout; fout.setDevices({{0, "synth"}});
        LiveEngineFacade fac(&fin, &fout);
        fac.loadStyle(style);
        fac.start();
        fac.selectMidiInput(0);
        fac.selectMidiOutput(0);
        chord::FingeredDetector det;
        fac.setChordScanMode(&det);

        fac.transportStart();
        for (int i = 0; i < 200; ++i) fac.tick(256);   // build up sounding notes
        const size_t before = fout.sent.size();
        fac.panic();
        for (int i = 0; i < 16; ++i) fac.tick(256);
        bool sawAllNotesOff = false;
        for (size_t i = before; i < fout.sent.size(); ++i) {
            const auto& e = fout.sent[i];
            if (e.type == uasf::MidiEventType::ControlChange &&
                (e.data1 == 123 || e.data1 == 120)) { sawAllNotesOff = true; break; }
        }
        TEST("panic dispatched all-notes-off (CC120/123)", sawAllNotesOff);
        TEST("panic stops playback", !fac.snapshot().playing);
        fac.stop();
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Tempo change mid-playback does not break scheduling.
    // ─────────────────────────────────────────────────────────────────────────
    {
        midi::FakeMidiInputSource   fin;  fin.devices = {{0, "kbd"}};
        midi::FakeMidiOutputProvider fout; fout.setDevices({{0, "synth"}});
        LiveEngineFacade fac(&fin, &fout);
        fac.loadStyle(style);
        fac.start();
        fac.selectMidiInput(0);
        fac.selectMidiOutput(0);
        chord::FingeredDetector det;
        fac.setChordScanMode(&det);

        fac.transportStart();
        for (int i = 0; i < 200; ++i) fac.tick(256);
        const uint64_t notesBefore = fac.snapshot().dispatchedNotes;
        const int64_t  posBefore   = fac.snapshot().positionTicks;

        fac.setTempo(160);
        for (int i = 0; i < 400; ++i) fac.tick(256);
        auto s = fac.snapshot();
        TEST("tempo change applied", s.tempoBpm == 160);
        TEST("tempo change: output keeps flowing", s.dispatchedNotes > notesBefore);
        TEST("tempo change: clock keeps advancing", s.positionTicks > posBefore);

        fac.transportStop();
        for (int i = 0; i < 32; ++i) fac.tick(256);
        TEST("tempo change: no stuck notes after stop", noteBalance(fout) == 0);
        fac.stop();
    }

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

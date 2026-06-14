// Gate 11 Task B — MIDI capture pipeline tests (synthetic only).

#include "korg_validation/midi_capture.h"
#include <cstdio>
#include <string>
#include <vector>

using namespace ai_arranger::korg_validation;

static int failures = 0, passes = 0;
#define TEST(name, expr) do { \
    if (!(expr)) { std::fprintf(stderr, "  \xE2\x9D\x8C FAIL: %s\n", name); failures++; } \
    else { std::printf("  \xE2\x9C\x85 PASS: %s\n", name); passes++; } \
} while(0)

#ifndef KORG_FIXTURE_DIR
#define KORG_FIXTURE_DIR "."
#endif

// Build a tiny one-track SMF equivalent to: tick0 NoteOn(36), tick480 NoteOff.
static std::vector<uint8_t> tinySmf() {
    std::vector<uint8_t> b = {
        'M','T','h','d', 0,0,0,6, 0,0, 0,1, 0x01,0xE0, // division 0x01E0 = 480
        'M','T','r','k', 0,0,0,0                         // len patched below
    };
    std::vector<uint8_t> trk = {
        0x00, 0x90, 36, 100,   // delta0 NoteOn ch0 note36 vel100
        0x83,0x60, 0x80, 36, 0 // delta 480 (0x8360) NoteOff
    };
    size_t lenPos = b.size() - 4;
    b[lenPos+0] = (trk.size() >> 24) & 0xFF;
    b[lenPos+1] = (trk.size() >> 16) & 0xFF;
    b[lenPos+2] = (trk.size() >> 8) & 0xFF;
    b[lenPos+3] = (trk.size()) & 0xFF;
    b.insert(b.end(), trk.begin(), trk.end());
    return b;
}

int main() {
    std::printf("Test: Gate 11 Task B — MIDI capture\n");

    // ── CSV string loader ──
    {
        const char* csv =
            "ppqn=480\n# comment\n0,0x90,36,100\n240,0x80,36,0\n480,144,43,90\n";
        MidiCapture c = loadCsvString(csv);
        TEST("CSV ppqn directive parsed", c.ppqn == 480);
        TEST("CSV event count (3)", c.events.size() == 3);
        TEST("CSV hex status parsed", c.events[0].status == 0x90);
        TEST("CSV decimal status parsed (144==0x90)", c.events[2].status == 0x90);
        TEST("CSV first is NoteOn", c.events[0].isNoteOn());
        TEST("CSV second is NoteOff", c.events[1].isNoteOff());
        TEST("CSV ascending tick order", c.events[2].tick == 480);
    }

    // ── CSV file fixture ──
    {
        MidiCapture c = loadCsv(std::string(KORG_FIXTURE_DIR) +
                                "/synthetic/two_bar_engine.csv");
        TEST("Fixture loads non-empty", !c.events.empty());
        TEST("Fixture ppqn 480", c.ppqn == 480);
        bool sorted = true;
        for (size_t i = 1; i < c.events.size(); ++i)
            if (c.events[i].tick < c.events[i-1].tick) sorted = false;
        TEST("Fixture sorted by tick", sorted);
    }

    // ── SMF (in-memory) loader matches the equivalent CSV ──
    {
        MidiCapture smf = loadSmfBytes(tinySmf());
        TEST("SMF division -> ppqn 480", smf.ppqn == 480);
        TEST("SMF event count (2)", smf.events.size() == 2);
        TEST("SMF NoteOn at tick 0", smf.events[0].tick == 0 && smf.events[0].isNoteOn());
        TEST("SMF NoteOff at tick 480", smf.events[1].tick == 480 && smf.events[1].isNoteOff());

        MidiCapture csv = loadCsvString("ppqn=480\n0,0x90,36,100\n480,0x80,36,0\n");
        bool same = smf.events.size() == csv.events.size();
        for (size_t i = 0; same && i < smf.events.size(); ++i)
            same = smf.events[i].tick == csv.events[i].tick &&
                   smf.events[i].status == csv.events[i].status &&
                   smf.events[i].data1 == csv.events[i].data1;
        TEST("SMF and CSV normalize identically", same);
    }

    // ── tickToMs ──
    TEST("tickToMs: 480 tick @120bpm,480ppqn = 500ms",
         tickToMs(480, 480, 120.0) == 500.0);
    TEST("tickToMs: zero ppqn safe", tickToMs(480, 0, 120.0) == 0.0);

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

// Gate 10 Task C — articulation render strategy.
//
// Tests the IArticulationRenderer strategies: the naive pass-through baseline
// and the keyswitch renderer (which injects an articulation-select note ahead
// of the main note). A small in-memory sink collects emitted events so we can
// assert order and content.

#include "engine/articulation/renderer.h"
#include <cstdio>
#include <vector>

using namespace ai_arranger;
using namespace ai_arranger::articulation;

static int failures = 0;
static int passes = 0;

#define TEST(name, expr) do { \
    if (!(expr)) { std::fprintf(stderr, "  \xE2\x9D\x8C FAIL: %s\n", name); failures++; } \
    else { std::printf("  \xE2\x9C\x85 PASS: %s\n", name); passes++; } \
} while(0)

// Collecting sink for tests (NOT realtime — fine in a test harness).
struct CollectSink final : EventSink {
    std::vector<uasf::MidiEvent> events;
    void emit(const uasf::MidiEvent& ev) noexcept override { events.push_back(ev); }
};

static uasf::MidiEvent noteOn(uint8_t note, uint8_t vel, uint64_t tick) {
    return {uasf::MidiEventType::NoteOn, 0, note, vel, tick};
}
static uasf::MidiEvent noteOff(uint8_t note, uint64_t tick) {
    return {uasf::MidiEventType::NoteOff, 0, note, 0, tick};
}

int main() {
    std::printf("Test: Gate 10 Task C — articulation render strategy\n");

    NaiveRenderer naive;
    KeyswitchRenderer keyswitch;
    uasf::ArticulationMetadata plain{};      // no keyswitch
    uasf::ArticulationMetadata ks{};
    ks.has_keyswitch = true;
    ks.keyswitch_note = 24;                   // low C, selects articulation

    TEST("Strategies expose stable names",
         std::string(naive.name()) == "naive" &&
         std::string(keyswitch.name()) == "keyswitch");

    // ── Naive: every event passes through unchanged ──
    {
        CollectSink s;
        naive.render(noteOn(60, 100, 480), plain, s);
        TEST("Naive NoteOn -> exactly 1 event", s.events.size() == 1);
        TEST("Naive NoteOn unchanged",
             s.events[0].data1 == 60 && s.events[0].data2 == 100 &&
             s.events[0].tick == 480);
    }
    {
        CollectSink s;
        naive.render(noteOff(60, 600), plain, s);
        TEST("Naive NoteOff passes through", s.events.size() == 1 &&
             s.events[0].type == uasf::MidiEventType::NoteOff);
    }

    // ── Keyswitch: trigger note injected strictly before the main note ──
    {
        CollectSink s;
        keyswitch.render(noteOn(60, 100, 480), ks, s);
        TEST("Keyswitch NoteOn -> 3 events (ks on/off + main)",
             s.events.size() == 3);

        // Locate keyswitch note (24) and main note (60) in emission order.
        int ksIdx = -1, mainIdx = -1;
        for (int i = 0; i < (int)s.events.size(); ++i) {
            if (s.events[i].type == uasf::MidiEventType::NoteOn &&
                s.events[i].data1 == 24) ksIdx = i;
            if (s.events[i].type == uasf::MidiEventType::NoteOn &&
                s.events[i].data1 == 60) mainIdx = i;
        }
        TEST("Keyswitch note present", ksIdx >= 0);
        TEST("Main note present", mainIdx >= 0);
        TEST("Keyswitch note emitted before main note", ksIdx >= 0 && mainIdx > ksIdx);
        if (ksIdx >= 0 && mainIdx >= 0) {
            TEST("Keyswitch tick <= main tick",
                 s.events[ksIdx].tick <= s.events[mainIdx].tick);
            TEST("Keyswitch tick strictly precedes main (when room)",
                 s.events[ksIdx].tick == 479);
            TEST("Main note preserved (60, vel 100, tick 480)",
                 s.events[mainIdx].data1 == 60 && s.events[mainIdx].data2 == 100 &&
                 s.events[mainIdx].tick == 480);
        }
    }

    // ── Keyswitch: no-keyswitch hint -> pass-through ──
    {
        CollectSink s;
        keyswitch.render(noteOn(60, 100, 480), plain, s);
        TEST("Keyswitch w/o hint passes through (1 event)", s.events.size() == 1);
    }

    // ── Keyswitch: NoteOff is never decorated ──
    {
        CollectSink s;
        keyswitch.render(noteOff(60, 600), ks, s);
        TEST("Keyswitch leaves NoteOff alone", s.events.size() == 1 &&
             s.events[0].type == uasf::MidiEventType::NoteOff);
    }

    // ── Keyswitch: main note already on the keyswitch key -> no recursion ──
    {
        CollectSink s;
        keyswitch.render(noteOn(24, 100, 480), ks, s);
        TEST("Keyswitch does not re-trigger its own key", s.events.size() == 1);
    }

    // ── Keyswitch at tick 0 clamps without underflow ──
    {
        CollectSink s;
        keyswitch.render(noteOn(60, 100, 0), ks, s);
        bool ok = s.events.size() == 3;
        for (const auto& e : s.events) if (e.tick != 0 && e.data1 == 24) ok = false;
        TEST("Keyswitch at tick 0 stays at tick 0 (no underflow)", ok);
    }

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

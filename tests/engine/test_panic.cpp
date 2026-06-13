#include "engine/midi/panic.h"
#include "engine/midi/note_balance.h"
#include "engine/midi/scheduler.h"
#include <cstdio>

using namespace ai_arranger::midi;
using namespace ai_arranger::uasf;

static int failures = 0;
static int passes = 0;

#define TEST(name, expr) do { \
    if (!(expr)) { \
        std::fprintf(stderr, "  ❌ FAIL: %s\n", name); \
        failures++; \
    } else { \
        std::printf("  ✅ PASS: %s\n", name); \
        passes++; \
    } \
} while(0)

int main() {
    std::printf("Test: PanicHandler\n");

    PanicHandler panic;

    // Initial state
    TEST("No active notes initially", !panic.hasActiveNotes());

    // Register note on
    panic.noteOn(0, 60); // Middle C on channel 0
    TEST("Active note after NoteOn", panic.hasActiveNotes());

    panic.noteOn(0, 64); // E on channel 0
    panic.noteOn(1, 36); // Kick on channel 1
    TEST("Multiple active notes", panic.hasActiveNotes());

    // Note off clears tracking
    panic.noteOff(0, 60);
    TEST("Still active after partial NoteOff", panic.hasActiveNotes());

    panic.noteOff(0, 64);
    panic.noteOff(1, 36);
    TEST("No active notes after all NoteOff", !panic.hasActiveNotes());

    // Panic test
    MidiScheduler scheduler;
    int callbackCount = 0;
    uint8_t lastController = 0;

    // Schedule some pending events
    MidiEvent ev;
    ev.type = MidiEventType::NoteOn;
    ev.channel = 0;
    ev.data1 = 60;
    ev.data2 = 100;
    ev.tick = 99999; // Far in the future
    scheduler.scheduleEvent(ev);
    scheduler.scheduleEvent(ev);
    scheduler.scheduleEvent(ev);

    // Register some active notes
    panic.noteOn(0, 60);
    panic.noteOn(0, 64);
    panic.noteOn(5, 72);

    MidiEventCallback sendCallback = [&](const MidiEvent& e) {
        callbackCount++;
        if (e.type == MidiEventType::ControlChange) {
            lastController = e.data1;
        }
    };

    // Execute panic
    panic.panic(scheduler, sendCallback);

    // Check panic effects
    // 16 channels × 2 controllers each (AllSoundOff + AllNotesOff) = 32 callbacks
    TEST("Panic sent 32+ events", callbackCount >= 32);
    TEST("AllSoundOff sent", lastController == 123); // AllNotesOff is last
    TEST("Queue cleared after panic", scheduler.pendingCount() == 0);
    TEST("Active notes cleared", !panic.hasActiveNotes());

    // Boundary: out-of-range note
    PanicHandler panic2;
    panic2.noteOn(0, 128); // Should be ignored
    TEST("Note 128 ignored", !panic2.hasActiveNotes());

    panic2.noteOn(15, 127); // Max valid note
    TEST("Max note registered", panic2.hasActiveNotes());
    panic2.noteOff(15, 127);
    TEST("Max note cleared", !panic2.hasActiveNotes());

    // ── PanicHandler: isNoteActive + flushActiveNotes (Gate 9) ───────
    PanicHandler ph;
    ph.noteOn(0, 60);
    ph.noteOn(0, 64);
    ph.noteOn(9, 36);
    TEST("isNoteActive true for on note", ph.isNoteActive(0, 60));
    TEST("isNoteActive false for off note", !ph.isNoteActive(0, 61));
    TEST("isNoteActive across channels", ph.isNoteActive(9, 36) && !ph.isNoteActive(1, 36));

    int flushed = 0;
    bool seen60 = false, seen64 = false, seen36 = false;
    bool allNoteOff = true, allVel0 = true;
    ph.flushActiveNotes([&](const MidiEvent& e) {
        flushed++;
        if (e.type != MidiEventType::NoteOff) allNoteOff = false;
        if (e.data2 != 0) allVel0 = false;
        if (e.channel == 0 && e.data1 == 60) seen60 = true;
        if (e.channel == 0 && e.data1 == 64) seen64 = true;
        if (e.channel == 9 && e.data1 == 36) seen36 = true;
    });
    TEST("flushActiveNotes emits one NoteOff per active note", flushed == 3);
    TEST("flushActiveNotes emits NoteOff for each tracked note", seen60 && seen64 && seen36);
    TEST("flushActiveNotes events are NoteOff vel0", allNoteOff && allVel0);
    TEST("flushActiveNotes clears tracking", !ph.hasActiveNotes());
    int flushedAgain = 0;
    ph.flushActiveNotes([&](const MidiEvent&) { flushedAgain++; });
    TEST("flushActiveNotes is idempotent when empty", flushedAgain == 0);

    // ── NoteBalance detector (Gate 9) ────────────────────────────────
    auto mk = [](MidiEventType t, uint8_t ch, uint8_t d1, uint8_t d2) {
        MidiEvent e; e.type = t; e.channel = ch; e.data1 = d1; e.data2 = d2; e.tick = 0; return e;
    };

    NoteBalance nb;
    TEST("NoteBalance starts balanced", nb.isBalanced() && nb.stuckNoteCount() == 0);

    // Balanced stream
    nb.observe(mk(MidiEventType::NoteOn, 0, 60, 100));
    nb.observe(mk(MidiEventType::NoteOn, 0, 64, 100));
    TEST("NoteBalance tracks active", nb.activeCount() == 2 && !nb.isBalanced());
    TEST("NoteBalance peak polyphony", nb.maxConcurrent() == 2);
    nb.observe(mk(MidiEventType::NoteOff, 0, 60, 0));
    nb.observe(mk(MidiEventType::NoteOff, 0, 64, 0));
    TEST("NoteBalance balanced after off", nb.isBalanced() && nb.stuckNoteCount() == 0);
    TEST("NoteBalance no orphans on clean stream", nb.orphanOffCount() == 0);
    TEST("NoteBalance counts on/off", nb.totalNoteOn() == 2 && nb.totalNoteOff() == 2);

    // NoteOn velocity 0 == NoteOff
    NoteBalance nb2;
    nb2.observe(mk(MidiEventType::NoteOn, 1, 50, 90));
    nb2.observe(mk(MidiEventType::NoteOn, 1, 50, 0)); // vel0 = note off
    TEST("NoteOn vel0 treated as NoteOff", nb2.isBalanced() && nb2.orphanOffCount() == 0);

    // Double NoteOff -> orphan
    NoteBalance nb3;
    nb3.observe(mk(MidiEventType::NoteOn, 2, 70, 100));
    nb3.observe(mk(MidiEventType::NoteOff, 2, 70, 0));
    nb3.observe(mk(MidiEventType::NoteOff, 2, 70, 0)); // orphan
    TEST("Double NoteOff produces an orphan", nb3.orphanOffCount() == 1);
    TEST("Double NoteOff stays balanced", nb3.isBalanced());

    // Stuck note detected
    NoteBalance nb4;
    nb4.observe(mk(MidiEventType::NoteOn, 3, 80, 100));
    TEST("Stuck note detected", nb4.stuckNoteCount() == 1 && !nb4.isBalanced());
    TEST("isNoteActive reports stuck note", nb4.isNoteActive(3, 80));

    // All Notes Off (CC123) clears stuck notes
    nb4.observe(mk(MidiEventType::ControlChange, 3, 123, 0));
    TEST("CC123 clears active notes", nb4.isBalanced() && nb4.stuckNoteCount() == 0);

    // All Sound Off (CC120) clears too
    NoteBalance nb5;
    nb5.observe(mk(MidiEventType::NoteOn, 4, 60, 100));
    nb5.observe(mk(MidiEventType::NoteOn, 4, 67, 100));
    nb5.observe(mk(MidiEventType::ControlChange, 4, 120, 0));
    TEST("CC120 clears active notes", nb5.isBalanced());

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

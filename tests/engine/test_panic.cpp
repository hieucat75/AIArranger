#include "engine/midi/panic.h"
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

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

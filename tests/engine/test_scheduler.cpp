#include "engine/midi/scheduler.h"
#include <cassert>
#include <cstdio>
#include <vector>

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
    std::printf("Test: MidiScheduler\n");

    MidiScheduler scheduler;

    // Initial state
    TEST("Queue empty initially", scheduler.pendingCount() == 0);

    // Schedule events
    MidiEvent ev;
    ev.type = MidiEventType::NoteOn;
    ev.channel = 0;
    ev.data1 = 60;
    ev.data2 = 100;
    ev.tick = 480; // Beat 1

    TEST("Schedule event succeeds", scheduler.scheduleEvent(ev));
    TEST("Queue has 1 pending", scheduler.pendingCount() == 1);

    // Schedule more events
    ev.tick = 960; // Beat 2
    TEST("Schedule second event", scheduler.scheduleEvent(ev));
    TEST("Queue has 2 pending", scheduler.pendingCount() == 2);

    // Dispatch events by advancing
    int dispatched_count = 0;
    MidiEvent last_dispatched{};

    scheduler.setOutputCallback([&](const MidiEvent& e) {
        dispatched_count++;
        last_dispatched = e;
    });

    // Advance to tick 480 -> first event should dispatch
    scheduler.advanceTo(480);
    TEST("First event dispatched (tick=480)", dispatched_count == 1);
    TEST("First event tick = 480", last_dispatched.tick == 480);
    TEST("Queue has 1 remaining", scheduler.pendingCount() == 1);

    // Advance past remaining
    scheduler.advanceTo(960);
    TEST("Second event dispatched", dispatched_count == 2);
    TEST("Queue empty after dispatch", scheduler.pendingCount() == 0);

    // Advance again: no new events
    scheduler.advanceTo(99999);
    TEST("No extra dispatches", dispatched_count == 2);

    // Schedule in future
    ev.tick = 100000;
    scheduler.scheduleEvent(ev);
    scheduler.advanceTo(50000);
    TEST("Future event not dispatched early", dispatched_count == 2);

    // Clear
    scheduler.clear();
    TEST("Queue empty after clear", scheduler.pendingCount() == 0);

    // Queue overflow test
    size_t maxPush = 0;
    for (size_t i = 0; i < 10000; i++) {
        ev.tick = i * 480;
        if (scheduler.scheduleEvent(ev)) maxPush++;
    }
    TEST("Can push up to capacity", maxPush > 8000);

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

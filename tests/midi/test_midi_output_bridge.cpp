// Gate 14C Task C — engine->MIDI output bridge (lock-free SPSC, synthetic).

#include "control/midi_output_bridge.h"
#include "fake_midi_output_provider.h"
#include "engine/uasf/types.h"
#include <atomic>
#include <cstdio>
#include <thread>

using namespace ai_arranger;
using namespace ai_arranger::control;
using namespace ai_arranger::midi;

static int failures = 0, passes = 0;
#define TEST(name, expr) do { \
    if (!(expr)) { std::fprintf(stderr, "  \xE2\x9D\x8C FAIL: %s\n", name); failures++; } \
    else { std::printf("  \xE2\x9C\x85 PASS: %s\n", name); passes++; } \
} while(0)

static uasf::MidiEvent ev(uint8_t d1) {
    return {uasf::MidiEventType::NoteOn, 0, d1, 100, 0};
}

int main() {
    std::printf("Test: Gate 14C Task C — MIDI output bridge\n");

    // ── Basic send -> pump -> provider ──
    {
        MidiOutputBridge b;
        FakeMidiOutputProvider p; p.setDevices({{0,"X"}}); p.select(0);
        b.send(ev(60)); b.send(ev(64));
        size_t n = b.pump(p);
        TEST("pump forwards 2", n == 2 && p.sent.size() == 2);
        TEST("order preserved", p.sent[0].data1 == 60 && p.sent[1].data1 == 64);
        TEST("forwarded count", b.forwarded() == 2);
    }

    // ── Flood, single thread ──
    {
        MidiOutputBridge b;
        FakeMidiOutputProvider p; p.setDevices({{0,"X"}}); p.select(0);
        const int N = 5000;
        for (int i = 0; i < N; ++i) b.send(ev((uint8_t)(i & 0x7F)));
        b.pump(p);
        TEST("flood 5000 forwarded", p.sent.size() == (size_t)N);
    }

    // ── Bounded overflow ──
    {
        MidiOutputBridge b;
        int ok = 0;
        for (int i = 0; i < 20000; ++i) if (b.send(ev(60))) ++ok;
        TEST("queue bounded (overflow dropped)", ok == (int)MidiOutputBridge::capacity());
        TEST("dropped counted", b.dropped() > 0);
    }

    // ── Concurrent producer/consumer, in order ──
    {
        MidiOutputBridge b;
        FakeMidiOutputProvider p; p.setDevices({{0,"X"}}); p.select(0);
        const int N = 50000;
        std::atomic<bool> done{false};
        std::thread producer([&]{
            for (int i = 0; i < N; ++i)
                while (!b.send(ev((uint8_t)(i & 0x7F)))) { /* spin if full */ }
            done.store(true);
        });
        // consumer drains until producer done AND queue empty.
        while (!done.load() || b.forwarded() < (uint64_t)N) b.pump(p);
        producer.join();
        b.pump(p);
        bool ordered = true;
        for (int i = 0; i < N && i < (int)p.sent.size(); ++i)
            if (p.sent[i].data1 != (uint8_t)(i & 0x7F)) { ordered = false; break; }
        TEST("concurrent: all 50k forwarded", p.sent.size() == (size_t)N);
        TEST("concurrent: in order", ordered);
    }

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

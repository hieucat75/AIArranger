// Gate 15 Task C — groove humanization determinism (byte-for-byte replay).

#include "performance/groove/groove_engine.h"
#include "performance/groove/groove_profile.h"
#include "performance/groove/deterministic_rng.h"
#include "engine/uasf/types.h"
#include <cstdio>
#include <string>
#include <vector>

using namespace ai_arranger;
using namespace ai_arranger::performance;

static int failures = 0, passes = 0;
#define TEST(name, expr) do { \
    if (!(expr)) { std::fprintf(stderr, "  \xE2\x9D\x8C FAIL: %s\n", name); failures++; } \
    else { std::printf("  \xE2\x9C\x85 PASS: %s\n", name); passes++; } \
} while(0)

// A fixed note sequence.
static std::vector<uasf::MidiEvent> sequence() {
    std::vector<uasf::MidiEvent> v;
    for (int bar = 0; bar < 4; ++bar)
        for (int e = 0; e < 8; ++e) {
            uint64_t tick = bar * 1920 + e * 240;
            v.push_back({uasf::MidiEventType::NoteOn, (uint8_t)(e % 4), (uint8_t)(48 + e), 90, tick});
            v.push_back({uasf::MidiEventType::NoteOff,(uint8_t)(e % 4), (uint8_t)(48 + e), 0, tick + 120});
        }
    return v;
}

// Serialize an output stream to a comparable blob (tick|data1|data2 per event).
static std::string run(uint64_t seed, int profileIdx) {
    GrooveEngine g(seed, 480);
    g.setProfile(&grooveByIndex(profileIdx));
    g.reset();
    std::string blob;
    char b[48];
    for (const auto& ev : sequence()) {
        auto o = g.apply(ev);
        std::snprintf(b, sizeof(b), "%llu,%d,%d;",
                      (unsigned long long)o.tick, o.data1, o.data2);
        blob += b;
    }
    return blob;
}

int main() {
    std::printf("Test: Gate 15 Task C — groove determinism\n");

    TEST("5 profiles available", kGrooveProfileCount == 5);

    // ── Byte-for-byte replay: same seed + profile -> identical stream ──
    for (int p = 0; p < kGrooveProfileCount; ++p) {
        std::string a = run(12345, p);
        std::string b = run(12345, p);
        char nm[64]; std::snprintf(nm, sizeof(nm),
            "profile %d (%s) deterministic replay", p, grooveByIndex(p).name());
        TEST(nm, a == b);
    }

    // ── Different seed -> different output (humanization actually varies) ──
    TEST("seed changes output (live-pop)", run(1, 3) != run(2, 3));

    // ── Tight profile barely moves; values stay valid MIDI ──
    {
        GrooveEngine g(7, 480); g.setProfile(&grooveTight()); g.reset();
        bool valid = true;
        for (const auto& ev : sequence()) {
            auto o = g.apply(ev);
            if (o.type == uasf::MidiEventType::NoteOn && (o.data2 < 1 || o.data2 > 127)) valid = false;
        }
        TEST("velocities stay in 1..127", valid);
    }

    // ── Non-note events pass through unchanged ──
    {
        GrooveEngine g(7, 480); g.setProfile(&grooveLivePop());
        uasf::MidiEvent cc{uasf::MidiEventType::ControlChange, 0, 7, 100, 500};
        auto o = g.apply(cc);
        TEST("CC unchanged", o.tick == 500 && o.data1 == 7 && o.data2 == 100);
    }

    // ── No profile -> pass-through ──
    {
        GrooveEngine g(7, 480);  // no profile set
        uasf::MidiEvent n{uasf::MidiEventType::NoteOn, 0, 60, 90, 240};
        auto o = g.apply(n);
        TEST("no profile -> unchanged", o.tick == 240 && o.data2 == 90);
    }

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

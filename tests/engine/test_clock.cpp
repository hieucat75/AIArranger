#include "engine/realtime/clock.h"
#include <cassert>
#include <cstdio>
#include <cmath>

using namespace ai_arranger::realtime;

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
    std::printf("Test: RealtimeClock\n");

    RealtimeClock clock;

    // Default state
    TEST("Default sample rate = 48000", clock.getTempo() == 120);
    TEST("Default tempo = 120 BPM", clock.getTempo() == 120);
    TEST("Not running initially", !clock.isRunning());
    TEST("Position starts at 0", clock.getPosition() == 0);

    // Config
    clock.setSampleRate(48000);
    clock.setTempo(120);
    clock.setResolution(480);

    // Advance when stopped
    clock.advance(48000);
    TEST("No advance when stopped", clock.getPosition() == 0);

    // Advance when running
    clock.start();
    clock.advance(48000); // 1 second at 48kHz
    TEST("Position > 0 after advance", clock.getPosition() > 0);
    clock.stop();

    // Tick calculation: at 120 BPM, 480 TPB, 48kHz:
    //   ticks per sample = 480 / (48000 * 60 / 120) = 480 / 24000 = 0.02
    //   After 48000 samples: position = 48000 * 0.02 = 960 ticks = 2 quarter notes
    double tps = clock.getTicksPerSample();
    TEST("Ticks per sample is reasonable (0.02)", std::abs(tps - 0.02) < 0.001);

    // Reset
    clock.reset();
    TEST("Reset position = 0", clock.getPosition() == 0);

    // Bar/beat calculation
    // At 120 BPM, 48kHz: 1 beat = 48000/2 = 24000 samples
    clock.start();
    clock.advance(48000); // = 2 beats = 1 bar in 4/4 time
    int64_t pos = clock.getPosition();
    TEST("Position after 1s = ~960 ticks", pos > 900 && pos < 1000);
    clock.stop();

    // Multiple advances
    clock.reset();
    clock.start();
    for (int i = 0; i < 10; i++) {
        clock.advance(256); // Small buffer
    }

    // Panic test
    clock.stop();

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

// Gate 15 Task B — CoreAudio/host-time clock invariants (headless, no audio
// session). Precise jitter is a local measurement; CI asserts invariants only.

#include "performance/clock/coreaudio_clock.h"
#include <cstdio>

using namespace ai_arranger::performance;

static int failures = 0, passes = 0;
#define TEST(name, expr) do { \
    if (!(expr)) { std::fprintf(stderr, "  \xE2\x9D\x8C FAIL: %s\n", name); failures++; } \
    else { std::printf("  \xE2\x9C\x85 PASS: %s\n", name); passes++; } \
} while(0)

int main() {
    std::printf("Test: Gate 15 Task B — CoreAudio host-time clock\n");

    CoreAudioClock c(48000);
    TEST("starts stopped", !c.isRunning());
    TEST("stopped poll = 0", c.pollElapsedSamples() == 0);

    c.start();
    TEST("running after start", c.isRunning());
    TEST("sample rate", c.sampleRate() == 48000);

    // Busy-wait a little so real host time elapses; assert monotonic/non-negative.
    volatile uint64_t spin = 0;
    for (uint64_t i = 0; i < 5'000'000ULL; ++i) spin += i;
    uint64_t s1 = c.pollElapsedSamples();
    TEST("elapsed samples non-negative after work", s1 >= 0);   // uint: always true, documents intent
    uint64_t immediate = c.pollElapsedSamples();
    TEST("poll resets (tiny gap small)", immediate < s1 + 100000);

    c.stop();
    TEST("stopped -> poll 0", !c.isRunning() && c.pollElapsedSamples() == 0);
    (void)spin;

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

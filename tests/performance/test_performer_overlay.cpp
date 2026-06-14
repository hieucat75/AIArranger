// Gate 15 Task J — performer overlay indicators (headless).

#include "ui/performance/performer_overlay.h"
#include <cstdio>

using namespace ai_arranger::ui;

static int failures = 0, passes = 0;
#define TEST(name, expr) do { \
    if (!(expr)) { std::fprintf(stderr, "  \xE2\x9D\x8C FAIL: %s\n", name); failures++; } \
    else { std::printf("  \xE2\x9C\x85 PASS: %s\n", name); passes++; } \
} while(0)

int main() {
    std::printf("Test: Gate 15 Task J — performer overlay\n");

    PerformerOverlay o;
    int changes = 0;
    o.setOnChanged([&]{ ++changes; });

    TEST("7 indicators", PerformerOverlay::kIndicatorCount == 7);

    // ── Each indicator updates ──
    o.setFillQueued(true);       TEST("fill queued", o.fillQueued());
    o.setVariationPending(true); TEST("variation pending", o.variationPending());
    o.setSyncArmed(true);        TEST("sync armed", o.syncArmed());
    o.setMidiActive(true);       TEST("midi active", o.midiActive());
    o.setReconnecting(true);     TEST("reconnecting", o.reconnecting());
    o.setGrooveProfile(3);       TEST("groove profile 3", o.grooveProfile() == 3);
    TEST("6 changes so far", changes == 6);

    // ── onChanged only on actual change ──
    int before = changes;
    o.setFillQueued(true);       // same
    TEST("no spurious change", changes == before);

    // ── Latency thresholds ──
    o.setLatencyMs(2.0);   TEST("low latency -> Good", o.latency() == LatencyLevel::Good);
    o.setLatencyMs(15.0);  TEST("mid latency -> Warn", o.latency() == LatencyLevel::Warn);
    o.setLatencyMs(50.0);  TEST("high latency -> Bad", o.latency() == LatencyLevel::Bad);
    int c2 = changes;
    o.setLatencyMs(55.0);  // still Bad
    TEST("same level -> no change", changes == c2);

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

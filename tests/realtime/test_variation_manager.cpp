// Gate 12 Task D — variation manager tests (deterministic, no duplicate).

#include "engine/performance/variation_manager.h"
#include <cstdio>

using namespace ai_arranger::performance;
using V = Variation;

static int failures = 0, passes = 0;
#define TEST(name, expr) do { \
    if (!(expr)) { std::fprintf(stderr, "  \xE2\x9D\x8C FAIL: %s\n", name); failures++; } \
    else { std::printf("  \xE2\x9C\x85 PASS: %s\n", name); passes++; } \
} while(0)

int main() {
    std::printf("Test: Gate 12 Task D — variation manager\n");

    // ── Default + basic queue/resolve ──
    {
        VariationManager m;
        TEST("default A", m.current() == V::A);
        TEST("no pending initially", !m.hasPending());
        m.queue(V::B);
        TEST("pending B", m.hasPending() && m.pending() == V::B);
        auto r = m.onBarBoundary();
        TEST("resolve to B (changed)", r.changed && r.variation == V::B);
        TEST("current now B, no pending", m.current() == V::B && !m.hasPending());
    }

    // ── Last-write-wins, single resolution ──
    {
        VariationManager m;
        m.queue(V::B); m.queue(V::D); m.queue(V::C);
        auto r = m.onBarBoundary();
        TEST("last-queued wins (C)", r.changed && r.variation == V::C);
        auto r2 = m.onBarBoundary();
        TEST("no second resolution", !r2.changed);
    }

    // ── No duplicate: queue == current -> no change ──
    {
        VariationManager m;            // current A
        m.queue(V::A);
        auto r = m.onBarBoundary();
        TEST("queue current -> no change", !r.changed && m.current() == V::A);
    }

    // ── Immediate switch ──
    {
        VariationManager m;
        auto r = m.setImmediate(V::D);
        TEST("immediate -> D changed", r.changed && m.current() == V::D);
        m.queue(V::A);
        auto r2 = m.setImmediate(V::B);
        TEST("immediate clears pending", r2.changed && m.current() == V::B &&
             !m.hasPending());
    }

    // ── reset ──
    {
        VariationManager m; m.setImmediate(V::C); m.queue(V::D);
        m.reset();
        TEST("reset -> A, no pending", m.current() == V::A && !m.hasPending());
    }

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

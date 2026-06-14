// Gate 14 Task D — diagnostics ViewModel (UI-agnostic, headless).

#include "ui/diagnostics_view_model.h"
#include <cstdio>

using namespace ai_arranger::ui;
using ai_arranger::control::EngineTelemetry;

static int failures = 0, passes = 0;
#define TEST(name, expr) do { \
    if (!(expr)) { std::fprintf(stderr, "  \xE2\x9D\x8C FAIL: %s\n", name); failures++; } \
    else { std::printf("  \xE2\x9C\x85 PASS: %s\n", name); passes++; } \
} while(0)

int main() {
    std::printf("Test: Gate 14 Task D — diagnostics ViewModel\n");

    DiagnosticsViewModel vm;
    int changes = 0;
    vm.setOnChanged([&]{ ++changes; });

    EngineTelemetry t;
    t.playing = true; t.section = 3; t.state = 2; t.position_ticks = 9600;
    t.chord_root = 67; t.dispatched_notes = 42;
    vm.applyTelemetry(t);

    TEST("playing mirrored", vm.playing());
    TEST("section mirrored", vm.section() == 3);
    TEST("state mirrored", vm.state() == 2);
    TEST("position mirrored", vm.position() == 9600);
    TEST("chord root mirrored", vm.chordRoot() == 67);
    TEST("dispatched notes mirrored", vm.dispatchedNotes() == 42);
    TEST("update count", vm.updateCount() == 1);
    TEST("onChanged fired", changes == 1);

    vm.applyTelemetry(t);
    TEST("update count advances", vm.updateCount() == 2 && changes == 2);

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

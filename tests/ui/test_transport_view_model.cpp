// Gate 14 Task D — transport ViewModel (UI-agnostic, headless).

#include "ui/transport_view_model.h"
#include <cstdio>
#include <vector>

using namespace ai_arranger::ui;
using ai_arranger::control::ControlAction;
using ai_arranger::control::ControlEvent;
using ai_arranger::control::EngineTelemetry;

static int failures = 0, passes = 0;
#define TEST(name, expr) do { \
    if (!(expr)) { std::fprintf(stderr, "  \xE2\x9D\x8C FAIL: %s\n", name); failures++; } \
    else { std::printf("  \xE2\x9C\x85 PASS: %s\n", name); passes++; } \
} while(0)

int main() {
    std::printf("Test: Gate 14 Task D — transport ViewModel\n");

    std::vector<ControlEvent> emitted;
    int changes = 0;
    TransportViewModel vm;
    vm.setControlSink([&](const ControlEvent& e){ emitted.push_back(e); });
    vm.setOnChanged([&]{ ++changes; });

    // ── Actions emit the right ControlEvents ──
    vm.start();   vm.fill();   vm.variation(2);   vm.panic();   vm.stop();
    TEST("5 events emitted", emitted.size() == 5);
    TEST("start -> Start", emitted[0].action == ControlAction::Start);
    TEST("fill -> Fill", emitted[1].action == ControlAction::Fill);
    TEST("variation(2) -> VariationC", emitted[2].action == ControlAction::VariationC);
    TEST("panic -> Panic", emitted[3].action == ControlAction::Panic);
    TEST("stop -> Stop", emitted[4].action == ControlAction::Stop);

    // ── Invalid variation index ignored ──
    emitted.clear();
    vm.variation(9);
    TEST("invalid variation ignored", emitted.empty());

    // ── Telemetry updates observable state + notifies on change ──
    EngineTelemetry t; t.playing = true; t.section = 1;
    vm.applyTelemetry(t);
    TEST("playing reflected", vm.isPlaying() && vm.section() == 1);
    TEST("onChanged fired", changes == 1);
    vm.applyTelemetry(t);  // same -> no change
    TEST("no spurious change notify", changes == 1);

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

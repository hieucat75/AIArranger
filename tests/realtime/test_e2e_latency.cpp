// Gate 13 Task H — end-to-end control->dispatch latency benchmark.
// Reuses the Gate 12 performer latency reporter (deterministic=true,
// hardware_validated=false). Measures ControlEvent -> effect (section switch /
// dispatch) latency on synthetic playback.

#include "engine/integration/performer_adapter.h"
#include "engine/arranger/style_player.h"
#include "engine/realtime/clock.h"
#include "engine/midi/scheduler.h"
#include "engine/midi/panic.h"
#include "engine/performance/latency_report.h"
#include "engine/control/control_events.h"
#include <cstdio>
#include <string>
#include <vector>

using namespace ai_arranger;
using namespace ai_arranger::integration;
using control::ControlAction;

static int failures = 0, passes = 0;
#define TEST(name, expr) do { \
    if (!(expr)) { std::fprintf(stderr, "  \xE2\x9D\x8C FAIL: %s\n", name); failures++; } \
    else { std::printf("  \xE2\x9C\x85 PASS: %s\n", name); passes++; } \
} while(0)
static bool has(const std::string& h, const std::string& n){ return h.find(n)!=std::string::npos; }
static double tickMs(double ticks){ return ticks / 480.0 * (60000.0 / 120.0); }

struct Rig {
    realtime::RealtimeClock clock; midi::MidiScheduler scheduler; midi::PanicHandler panic;
    arranger::StylePlayer player{clock, scheduler, panic};
    PerformerAdapter adapter{clock, player};
    Rig(){ player.loadStyle(arranger::buildDemoStyle()); clock.setSampleRate(48000);
           clock.setResolution(480); clock.setTempo(120); adapter.setVariationSections(0,1,2,3); }
    void step(int n){ for(int i=0;i<n;++i){adapter.tick();clock.advance(256);scheduler.advanceTo(clock.getPosition());} }
};

// Measure ticks from posting a variation to the section actually switching.
static double measureSwitchLatency(int targetSection, control::ControlAction act) {
    Rig r;
    r.adapter.postEvent({ControlAction::Start,0,0});
    r.step(20);
    const int64_t controlTick = r.clock.getPosition();
    r.adapter.postEvent({act,0,0});
    for (int i=0;i<3000;++i){
        r.adapter.tick(); r.clock.advance(256); r.scheduler.advanceTo(r.clock.getPosition());
        if (r.player.getCurrentSection()==targetSection)
            return static_cast<double>(r.clock.getPosition()-controlTick);
    }
    return -1.0;
}

int main() {
    std::printf("Test: Gate 13 Task H — E2E latency benchmark\n");

    double tB = measureSwitchLatency(1, ControlAction::VariationB);
    double tC = measureSwitchLatency(2, ControlAction::VariationC);
    TEST("variation B switch measured", tB >= 0.0);
    TEST("variation C switch measured", tC >= 0.0);
    // Quantize latency must be < 1 bar (1920 ticks) since it lands on the next boundary.
    TEST("variation latency within one bar", tB < 1920.0 && tC < 1920.0);

    std::vector<double> samples = { tickMs(tB), tickMs(tC) };
    // Budget = one bar (2000 ms @120bpm) + slack: a bar-quantized switch lands on
    // the next boundary, so worst case approaches one bar of latency.
    performance::LatencyMetric m =
        performance::measureLatency("control_to_section_switch", samples, /*budget*/ 2050.0);
    TEST("E2E latency within one-bar budget", m.within_budget);

    performance::PerformerLatencyReport rep;
    rep.scenario = "e2e_control_to_dispatch";
    rep.metrics = { m };

    const std::string js = performance::toJson(rep);
    const std::string md = performance::toMarkdown(rep);
    TEST("report deterministic:true", has(js, "\"deterministic\": true"));
    TEST("report hardware_validated:false", has(js, "\"hardware_validated\": false"));
    TEST("report never hardware true", !has(js, "\"hardware_validated\": true"));
    TEST("report has E2E metric", has(js, "control_to_section_switch"));
    TEST("MD flags present", has(md, "deterministic: true") &&
         has(md, "hardware_validated: false"));

    std::printf("\n  [e2e] varB=%.0ftick varC=%.0ftick\n", tB, tC);
    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

// Gate 11 Task F — reporter (JSON + Markdown) tests.
// Asserts the report shape and the hard-coded hardware_validated=false.

#include "korg_validation/reporter.h"
#include "korg_validation/midi_capture.h"
#include "korg_validation/timing_analyzer.h"
#include "korg_validation/stuck_note_detector.h"
#include "korg_validation/jitter_benchmark.h"
#include "korg_validation/latency_meter.h"
#include "korg_validation/transition_validator.h"
#include <cstdio>
#include <string>

using namespace ai_arranger::korg_validation;

static int failures = 0, passes = 0;
#define TEST(name, expr) do { \
    if (!(expr)) { std::fprintf(stderr, "  \xE2\x9D\x8C FAIL: %s\n", name); failures++; } \
    else { std::printf("  \xE2\x9C\x85 PASS: %s\n", name); passes++; } \
} while(0)

static bool has(const std::string& hay, const std::string& needle) {
    return hay.find(needle) != std::string::npos;
}

int main() {
    std::printf("Test: Gate 11 Task F — reporter\n");

    MidiCapture eng = loadCsvString(
        "ppqn=480\n0,0x90,36,100\n240,0x80,36,0\n480,0x90,43,90\n720,0x80,43,0\n");

    ValidationReport r;
    r.fixture = "synthetic_two_bar";
    r.timing = analyzeTiming(eng, eng, 120.0);
    r.stuck = detectStuck(eng);
    r.jitter = benchmarkJitter(eng, 240, 120.0);
    r.latency = measureLatency(eng, {0}, 120.0);
    r.transitions = validateTransitions({{"Main A->B", 1920, 1920}}, 1920, 0);

    const std::string js = toJson(r);
    const std::string md = toMarkdown(r);

    // ── JSON shape + claims policy ──
    TEST("JSON hard-codes hardware_validated:false",
         has(js, "\"hardware_validated\": false"));
    TEST("JSON has harness_version", has(js, "\"harness_version\""));
    TEST("JSON has fixture name", has(js, "synthetic_two_bar"));
    TEST("JSON has timing block", has(js, "\"timing\""));
    TEST("JSON has transitions block", has(js, "\"transitions\""));
    TEST("JSON has latency block", has(js, "\"latency\""));
    TEST("JSON has stuck block", has(js, "\"stuck\""));
    TEST("JSON has jitter block", has(js, "\"jitter\""));
    TEST("JSON never claims true", !has(js, "\"hardware_validated\": true"));

    // ── Markdown shape ──
    TEST("MD title", has(md, "# KORG Validation Report"));
    TEST("MD states hardware_validated false", has(md, "hardware_validated: false"));
    TEST("MD Timing section", has(md, "## Timing"));
    TEST("MD Transitions section", has(md, "## Transitions"));
    TEST("MD Chord latency section", has(md, "## Chord latency"));
    TEST("MD Stuck-note section", has(md, "## Stuck-note"));
    TEST("MD Jitter section", has(md, "## Jitter"));

    // ── writeReport round-trip to a temp path ──
    {
        const std::string jp = "/tmp/korg_report_test.json";
        const std::string mp = "/tmp/korg_report_test.md";
        bool ok = writeReport(r, jp, mp);
        TEST("writeReport succeeds", ok);
    }

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

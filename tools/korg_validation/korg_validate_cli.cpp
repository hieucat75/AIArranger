// korg-validate — CLI front-end for the KORG validation harness (Gate 11).
//
// Synthetic-only, CI-safe. Builds a ValidationReport from MIDI captures and
// emits JSON + Markdown. NEVER claims hardware parity (reports carry
// hardware_validated=false).
#include "korg_validation/harness.h"
#include "korg_validation/midi_capture.h"
#include "korg_validation/timing_analyzer.h"
#include "korg_validation/stuck_note_detector.h"
#include "korg_validation/jitter_benchmark.h"
#include "korg_validation/latency_meter.h"
#include "korg_validation/reporter.h"
#include <cstdio>
#include <cstring>
#include <string>

using namespace ai_arranger::korg_validation;

static std::string argOf(int argc, char** argv, const char* key) {
    for (int i = 1; i + 1 < argc; ++i)
        if (std::strcmp(argv[i], key) == 0) return argv[i + 1];
    return {};
}

int main(int argc, char** argv) {
    if (argc >= 2 && std::strcmp(argv[1], "--version") == 0) {
        std::printf("korg-validate v%d (hardware_validated=%s)\n",
                    kHarnessVersion, kHardwareValidated ? "true" : "false");
        return 0;
    }

    const std::string engine = argOf(argc, argv, "--engine");
    const std::string reference = argOf(argc, argv, "--reference");
    const std::string outJson = argOf(argc, argv, "--out-json");
    const std::string outMd = argOf(argc, argv, "--out-md");
    const double bpm = 120.0;
    const uint64_t bar = 1920, grid = 240;

    if (engine.empty()) {
        std::printf("korg-validate — KORG validation harness (Gate 11)\n");
        std::printf("  Synthetic-only. hardware_validated=%s.\n",
                    kHardwareValidated ? "true" : "false");
        std::printf("  Usage: korg-validate --engine <csv|mid> [--reference <csv|mid>]\n");
        std::printf("                       [--out-json <path>] [--out-md <path>]\n");
        return 0;
    }

    auto load = [](const std::string& p) {
        return (p.size() > 4 && p.substr(p.size() - 4) == ".csv") ? loadCsv(p)
                                                                  : loadSmf(p);
    };

    MidiCapture eng = load(engine);
    MidiCapture ref = reference.empty() ? eng : load(reference);

    ValidationReport rep;
    rep.fixture = engine;
    rep.timing = analyzeTiming(eng, ref, bpm);
    rep.stuck = detectStuck(eng);
    rep.jitter = benchmarkJitter(eng, grid, bpm);
    rep.latency = measureLatency(eng, {}, bpm);
    rep.transitions = {};  // supplied by the orchestrating harness, not the CLI
    (void)bar;

    if (!outJson.empty() || !outMd.empty()) {
        writeReport(rep, outJson, outMd);
        std::printf("wrote report (hardware_validated=false)\n");
    } else {
        std::fputs(toMarkdown(rep).c_str(), stdout);
    }
    return 0;
}

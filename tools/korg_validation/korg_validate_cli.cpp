// korg-validate — CLI front-end for the KORG validation harness (Gate 11).
//
// Synthetic-only, CI-safe. Wired with full report generation in Task F.
#include "korg_validation/harness.h"
#include <cstdio>
#include <cstring>

using namespace ai_arranger::korg_validation;

int main(int argc, char** argv) {
    if (argc >= 2 && std::strcmp(argv[1], "--version") == 0) {
        std::printf("korg-validate v%d (hardware_validated=%s)\n",
                    kHarnessVersion, kHardwareValidated ? "true" : "false");
        return 0;
    }
    std::printf("korg-validate — KORG validation harness (Gate 11)\n");
    std::printf("  Synthetic-only. hardware_validated=%s.\n",
                kHardwareValidated ? "true" : "false");
    std::printf("  Usage: korg-validate --engine <csv> --reference <csv> "
                "--out-json <path> --out-md <path>\n");
    std::printf("  (report wiring lands in G11-TASK-F)\n");
    return 0;
}

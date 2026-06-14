// Gate 14 Task D — chord ViewModel (UI-agnostic, headless).

#include "ui/chord_view_model.h"
#include <cstdio>
#include <string>

using namespace ai_arranger::ui;
using ai_arranger::arranger::Chord;
using ai_arranger::arranger::ChordType;
using ai_arranger::control::EngineTelemetry;

static int failures = 0, passes = 0;
#define TEST(name, expr) do { \
    if (!(expr)) { std::fprintf(stderr, "  \xE2\x9D\x8C FAIL: %s\n", name); failures++; } \
    else { std::printf("  \xE2\x9C\x85 PASS: %s\n", name); passes++; } \
} while(0)

int main() {
    std::printf("Test: Gate 14 Task D — chord ViewModel\n");

    ChordViewModel vm;
    Chord captured{ChordType::NoChord, 0, 0, 0};
    int changes = 0;
    vm.setChordSink([&](const Chord& c){ captured = c; });
    vm.setOnChanged([&]{ ++changes; });

    // ── Input forwards a chord through the sink ──
    vm.inputChord(65, ChordType::Minor);   // F minor
    TEST("input forwards chord (Fm)", captured.root == 65 && captured.type == ChordType::Minor);

    // ── Telemetry drives the display name ──
    auto tele = [](uint8_t root, ChordType t){
        EngineTelemetry e; e.chord_root = root; e.chord_type = (uint8_t)t; return e; };
    vm.applyTelemetry(tele(60, ChordType::Major));
    TEST("display C", vm.displayName() == "C");
    vm.applyTelemetry(tele(65, ChordType::Minor));
    TEST("display Fm", vm.displayName() == "Fm");
    vm.applyTelemetry(tele(67, ChordType::Dom7));
    TEST("display G7", vm.displayName() == "G7");
    vm.applyTelemetry(tele(62, ChordType::Min7));
    TEST("display Dm7", vm.displayName() == "Dm7");
    vm.applyTelemetry(tele(0, ChordType::NoChord));
    TEST("display -- for NoChord", vm.displayName() == "--");

    TEST("onChanged fired per distinct chord", changes == 5);
    int before = changes;
    vm.applyTelemetry(tele(0, ChordType::NoChord)); // same
    TEST("no spurious change", changes == before);

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

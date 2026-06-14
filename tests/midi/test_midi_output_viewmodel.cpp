// Gate 14C Task E — MIDI output ViewModel (UI-agnostic, headless).

#include "ui/midi/midi_output_viewmodel.h"
#include "engine/uasf/types.h"
#include <cstdio>
#include <string>

using namespace ai_arranger;
using namespace ai_arranger::ui;

static int failures = 0, passes = 0;
#define TEST(name, expr) do { \
    if (!(expr)) { std::fprintf(stderr, "  \xE2\x9D\x8C FAIL: %s\n", name); failures++; } \
    else { std::printf("  \xE2\x9C\x85 PASS: %s\n", name); passes++; } \
} while(0)

static uasf::MidiEvent noteOn(uint8_t n, uint8_t v) {
    return {uasf::MidiEventType::NoteOn, 0, n, v, 0};
}

int main() {
    std::printf("Test: Gate 14C Task E — MIDI output ViewModel\n");

    MidiOutputViewModel vm;
    int selected = -99, changes = 0;
    vm.setSelectSink([&](int i){ selected = i; });
    vm.setOnChanged([&]{ ++changes; });

    // ── device list + selection forwarding ──
    vm.setDevices({{0,"IAC Bus 1"},{1,"Korg PA700"}});
    TEST("devices set", vm.devices().size() == 2);
    vm.selectDevice(1);
    TEST("selectDevice forwards to sink", selected == 1);

    // ── connection state ──
    vm.setConnection(midi::OutputState::Connected, "Korg PA700", 1);
    TEST("connection state text", std::string(vm.connectionText()) == "connected");
    TEST("selected name/index", vm.selectedName() == "Korg PA700" && vm.selectedIndex() == 1);
    vm.setConnection(midi::OutputState::Disconnected, "Korg PA700", -1);
    TEST("disconnected text", std::string(vm.connectionText()) == "disconnected");

    // ── sent events: count + last + format ──
    vm.recordSent(noteOn(60, 100));
    TEST("sent count 1", vm.sentCount() == 1);
    TEST("last message format", vm.lastMessage() == "NoteOn ch1 60 100");
    TEST("formatEvent CC", MidiOutputViewModel::formatEvent(
         {uasf::MidiEventType::ControlChange,2,7,127,0}) == "CC ch3 7 127");

    // ── ring buffer wrap (kRecent) ──
    for (int i = 0; i < MidiOutputViewModel::kRecent + 5; ++i)
        vm.recordSent(noteOn((uint8_t)(64 + (i % 4)), 90));
    TEST("ring capped at kRecent", (int)vm.recentMessages().size() == MidiOutputViewModel::kRecent);
    TEST("sent count accumulates", vm.sentCount() == (uint64_t)(1 + MidiOutputViewModel::kRecent + 5));

    // ── onChanged fired on updates ──
    TEST("onChanged fired", changes > 0);

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

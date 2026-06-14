// Gate 14 Task I — MIDI device manager (synthetic provider, headless).

#include "midi/midi_device_manager.h"
#include <cstdio>

using namespace ai_arranger::midi;

static int failures = 0, passes = 0;
#define TEST(name, expr) do { \
    if (!(expr)) { std::fprintf(stderr, "  \xE2\x9D\x8C FAIL: %s\n", name); failures++; } \
    else { std::printf("  \xE2\x9C\x85 PASS: %s\n", name); passes++; } \
} while(0)

// Synthetic provider whose device list we mutate to simulate hot-plug.
struct FakeProvider : IMidiDeviceProvider {
    std::vector<MidiDeviceInfo> list;
    std::vector<MidiDeviceInfo> enumerateDestinations() override { return list; }
};

int main() {
    std::printf("Test: Gate 14 Task I — MIDI device manager\n");

    FakeProvider p;
    MidiDeviceManager mgr(p);
    int changes = 0;
    mgr.setOnChanged([&]{ ++changes; });

    // ── Enumerate ──
    p.list = { {"Korg PA700", 10}, {"IAC Bus", 20} };
    mgr.refresh();
    TEST("enumerates 2 devices", mgr.devices().size() == 2);
    TEST("onChanged fired", changes == 1);
    TEST("no selection initially", mgr.selectedIndex() < 0);

    // ── Select ──
    TEST("select by id", mgr.selectDevice(20) && mgr.selected() &&
         mgr.selected()->name == "IAC Bus");
    TEST("select unknown id fails", !mgr.selectDevice(999));

    // ── Hot-plug add: selection preserved by id ──
    p.list = { {"Korg PA700", 10}, {"IAC Bus", 20}, {"USB MIDI", 30} };
    mgr.notifyHotPlug();
    TEST("hot-plug add -> 3 devices", mgr.devices().size() == 3);
    TEST("selection preserved by id (IAC Bus)",
         mgr.selected() && mgr.selected()->id == 20);

    // ── Hot-plug remove the selected device -> graceful clear ──
    p.list = { {"Korg PA700", 10}, {"USB MIDI", 30} };
    mgr.notifyHotPlug();
    TEST("selected device removed -> no crash, cleared", mgr.selectedIndex() < 0 &&
         mgr.selected() == nullptr);

    // ── Empty enumeration is safe ──
    p.list.clear();
    mgr.refresh();
    TEST("empty enumeration safe", mgr.devices().empty() && mgr.selectedIndex() < 0);

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

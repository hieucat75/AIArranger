#ifndef AI_ARRANGER_MIDI_MIDI_DEVICE_MANAGER_H
#define AI_ARRANGER_MIDI_MIDI_DEVICE_MANAGER_H

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

// ── MIDI device layer (Gate 14 Task I) ───────────────────────────────
//
// Enumerates MIDI destinations, tracks a selection, and survives hot-plug
// (add/remove). The device source is abstracted behind IMidiDeviceProvider so
// the manager is fully testable with a synthetic provider — no CoreMIDI calls in
// CI. The real app wires a CoreMIDI-backed provider.

namespace ai_arranger::midi {

struct MidiDeviceInfo {
    std::string name;
    uint32_t    id = 0;
};

class IMidiDeviceProvider {
public:
    virtual ~IMidiDeviceProvider() = default;
    virtual std::vector<MidiDeviceInfo> enumerateDestinations() = 0;
};

class MidiDeviceManager {
public:
    explicit MidiDeviceManager(IMidiDeviceProvider& provider) noexcept
        : provider_(provider) {}

    void setOnChanged(std::function<void()> cb) { on_changed_ = std::move(cb); }

    // Re-enumerate. If the currently selected device disappeared, selection is
    // cleared gracefully. Fires onChanged.
    void refresh();

    // Hot-plug hook — the real provider's notify callback calls this.
    void notifyHotPlug() { refresh(); }

    const std::vector<MidiDeviceInfo>& devices() const noexcept { return devices_; }

    // Select by id; returns true if found.
    bool selectDevice(uint32_t id) noexcept;

    int selectedIndex() const noexcept { return selected_; }
    const MidiDeviceInfo* selected() const noexcept {
        return (selected_ >= 0 && selected_ < (int)devices_.size())
                   ? &devices_[selected_] : nullptr;
    }

private:
    IMidiDeviceProvider&        provider_;
    std::vector<MidiDeviceInfo> devices_;
    int                         selected_ = -1;
    uint32_t                    selected_id_ = 0;
    bool                        has_selection_ = false;
    std::function<void()>       on_changed_;
};

} // namespace ai_arranger::midi

#endif // AI_ARRANGER_MIDI_MIDI_DEVICE_MANAGER_H

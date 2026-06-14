#include "midi/midi_device_manager.h"

namespace ai_arranger::midi {

void MidiDeviceManager::refresh() {
    devices_ = provider_.enumerateDestinations();

    // Re-resolve the selection by id so add/remove keeps pointing at the same
    // device (or clears gracefully if it was unplugged).
    selected_ = -1;
    if (has_selection_) {
        for (int i = 0; i < (int)devices_.size(); ++i) {
            if (devices_[i].id == selected_id_) { selected_ = i; break; }
        }
        if (selected_ < 0) has_selection_ = false;  // device gone
    }
    if (on_changed_) on_changed_();
}

bool MidiDeviceManager::selectDevice(uint32_t id) noexcept {
    for (int i = 0; i < (int)devices_.size(); ++i) {
        if (devices_[i].id == id) {
            selected_ = i;
            selected_id_ = id;
            has_selection_ = true;
            return true;
        }
    }
    return false;
}

} // namespace ai_arranger::midi

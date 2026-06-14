#ifndef AI_ARRANGER_UI_MIDI_OUTPUT_VIEWMODEL_H
#define AI_ARRANGER_UI_MIDI_OUTPUT_VIEWMODEL_H

#include "midi/midi_output_manager.h"
#include "engine/uasf/types.h"
#include <functional>
#include <string>
#include <vector>

// ── MIDI output ViewModel (Gate 14C Task E) ──────────────────────────
//
// UI-agnostic state for the MIDI Output panel: device list, selected index,
// connection state, sent-event count, and a small ring of recent messages.
// The UI calls selectDevice() (forwarded via a sink to the manager); telemetry
// (state/sent) is pushed in. No JUCE, no engine logic.

namespace ai_arranger::ui {

class MidiOutputViewModel {
public:
    using SelectSink = std::function<void(int index)>;

    static constexpr int kRecent = 16;

    void setSelectSink(SelectSink s) { sink_ = std::move(s); }
    void setOnChanged(std::function<void()> cb) { on_changed_ = std::move(cb); }

    // ── from manager ──
    void setDevices(std::vector<midi::MidiOutDeviceInfo> d) {
        devices_ = std::move(d); notify();
    }
    void setConnection(midi::OutputState state, const std::string& name, int index) {
        state_ = state; selected_name_ = name; selected_index_ = index; notify();
    }

    // ── from MIDI dispatch telemetry ──
    void recordSent(const uasf::MidiEvent& ev);

    // ── UI action ──
    void selectDevice(int index) { if (sink_) sink_(index); }

    // ── getters ──
    const std::vector<midi::MidiOutDeviceInfo>& devices() const { return devices_; }
    midi::OutputState state() const { return state_; }
    const char* connectionText() const;
    int selectedIndex() const { return selected_index_; }
    const std::string& selectedName() const { return selected_name_; }
    uint64_t sentCount() const { return sent_count_; }
    std::string lastMessage() const;                 // most recent
    std::vector<std::string> recentMessages() const; // up to kRecent, newest last

    static std::string formatEvent(const uasf::MidiEvent& ev);

private:
    void notify() { if (on_changed_) on_changed_(); }

    SelectSink sink_;
    std::function<void()> on_changed_;
    std::vector<midi::MidiOutDeviceInfo> devices_;
    midi::OutputState state_ = midi::OutputState::NoDevice;
    std::string selected_name_;
    int selected_index_ = -1;
    uint64_t sent_count_ = 0;
    std::string ring_[kRecent];
    int ring_head_ = 0;
    int ring_count_ = 0;
};

} // namespace ai_arranger::ui

#endif // AI_ARRANGER_UI_MIDI_OUTPUT_VIEWMODEL_H

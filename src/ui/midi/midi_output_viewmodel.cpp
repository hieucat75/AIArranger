#include "ui/midi/midi_output_viewmodel.h"
#include <cstdio>

namespace ai_arranger::ui {

const char* MidiOutputViewModel::connectionText() const {
    switch (state_) {
        case midi::OutputState::NoDevice:    return "no device";
        case midi::OutputState::Selected:    return "selected";
        case midi::OutputState::Connected:   return "connected";
        case midi::OutputState::Disconnected:return "disconnected";
        case midi::OutputState::Reconnecting:return "reconnecting";
    }
    return "?";
}

std::string MidiOutputViewModel::formatEvent(const uasf::MidiEvent& ev) {
    const char* type = "?";
    switch (ev.type) {
        case uasf::MidiEventType::NoteOff:         type = "NoteOff"; break;
        case uasf::MidiEventType::NoteOn:          type = "NoteOn";  break;
        case uasf::MidiEventType::ControlChange:   type = "CC";      break;
        case uasf::MidiEventType::ProgramChange:   type = "PC";      break;
        case uasf::MidiEventType::PitchBend:       type = "Bend";    break;
        default:                                    type = "MIDI";    break;
    }
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%s ch%d %d %d",
                  type, ev.channel + 1, ev.data1, ev.data2);
    return buf;
}

void MidiOutputViewModel::recordSent(const uasf::MidiEvent& ev) {
    ++sent_count_;
    ring_[ring_head_] = formatEvent(ev);
    ring_head_ = (ring_head_ + 1) % kRecent;
    if (ring_count_ < kRecent) ++ring_count_;
    notify();
}

std::string MidiOutputViewModel::lastMessage() const {
    if (ring_count_ == 0) return "--";
    const int idx = (ring_head_ - 1 + kRecent) % kRecent;
    return ring_[idx];
}

std::vector<std::string> MidiOutputViewModel::recentMessages() const {
    std::vector<std::string> out;
    const int start = (ring_head_ - ring_count_ + kRecent) % kRecent;
    for (int i = 0; i < ring_count_; ++i)
        out.push_back(ring_[(start + i) % kRecent]);
    return out;  // oldest first, newest last
}

} // namespace ai_arranger::ui

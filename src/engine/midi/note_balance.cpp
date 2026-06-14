#include "engine/midi/note_balance.h"
#include <cstring>

namespace ai_arranger::midi {

void NoteBalance::reset() noexcept {
    std::memset(active_, 0, sizeof(active_));
    active_total_ = 0;
    max_concurrent_ = 0;
    total_on_ = 0;
    total_off_ = 0;
    orphan_off_ = 0;
}

void NoteBalance::observe(const uasf::MidiEvent& ev) noexcept {
    const uint8_t ch = ev.channel & 0x0F;

    // All Sound Off / All Notes Off clear every active note on the channel.
    if (ev.type == uasf::MidiEventType::ControlChange &&
        (ev.data1 == kAllSoundOff || ev.data1 == kAllNotesOff)) {
        for (int n = 0; n < 128; ++n) {
            active_total_ -= active_[ch][n];
            active_[ch][n] = 0;
        }
        return;
    }

    if (ev.data1 > 127) return;
    const uint8_t note = ev.data1;

    const bool isNoteOn  = ev.type == uasf::MidiEventType::NoteOn && ev.data2 > 0;
    const bool isNoteOff = ev.type == uasf::MidiEventType::NoteOff ||
                           (ev.type == uasf::MidiEventType::NoteOn && ev.data2 == 0);

    if (isNoteOn) {
        active_[ch][note]++;
        active_total_++;
        total_on_++;
        if (active_total_ > max_concurrent_) max_concurrent_ = active_total_;
    } else if (isNoteOff) {
        total_off_++;
        if (active_[ch][note] > 0) {
            active_[ch][note]--;
            active_total_--;
        } else {
            orphan_off_++;
        }
    }
}

int NoteBalance::stuckNoteCount() const noexcept {
    int n = 0;
    for (int ch = 0; ch < 16; ++ch) {
        for (int note = 0; note < 128; ++note) {
            if (active_[ch][note] > 0) ++n;
        }
    }
    return n;
}

bool NoteBalance::isNoteActive(uint8_t channel, uint8_t note) const noexcept {
    if (channel > 15 || note > 127) return false;
    return active_[channel][note] > 0;
}

} // namespace ai_arranger::midi

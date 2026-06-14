#include "engine/midi/panic.h"
#include <cstring>

namespace ai_arranger::midi {

PanicHandler::PanicHandler() {
    for (auto& low : channel_notes_low_) low.store(0, std::memory_order_relaxed);
    for (auto& high : channel_notes_high_) high.store(0, std::memory_order_relaxed);
}

void PanicHandler::noteOn(uint8_t channel, uint8_t note) noexcept {
    if (channel > kMaxChannel || note > 127) return;
    if (note < 64) {
        channel_notes_low_[channel].fetch_or(1ULL << note, std::memory_order_acq_rel);
    } else {
        channel_notes_high_[channel].fetch_or(1ULL << (note - 64), std::memory_order_acq_rel);
    }
    active_note_count_.fetch_add(1, std::memory_order_acq_rel);
}

void PanicHandler::noteOff(uint8_t channel, uint8_t note) noexcept {
    if (channel > kMaxChannel || note > 127) return;
    uint64_t old_low, new_low;
    if (note < 64) {
        do {
            old_low = channel_notes_low_[channel].load(std::memory_order_acquire);
            new_low = old_low & ~(1ULL << note);
        } while (!channel_notes_low_[channel].compare_exchange_weak(old_low, new_low,
            std::memory_order_acq_rel, std::memory_order_acquire));
        if (old_low != new_low) {
            active_note_count_.fetch_sub(1, std::memory_order_acq_rel);
        }
    } else {
        do {
            old_low = channel_notes_high_[channel].load(std::memory_order_acquire);
            new_low = old_low & ~(1ULL << (note - 64));
        } while (!channel_notes_high_[channel].compare_exchange_weak(old_low, new_low,
            std::memory_order_acq_rel, std::memory_order_acquire));
        if (old_low != new_low) {
            active_note_count_.fetch_sub(1, std::memory_order_acq_rel);
        }
    }
}

void PanicHandler::panic(MidiScheduler& scheduler, MidiEventCallback sendCallback) noexcept {
    if (!sendCallback) return;

    // Phase 1: Clear pending scheduler queue
    scheduler.clear();

    // Phase 2: Send AllSoundOff (CC 120) on all channels
    uasf::MidiEvent allSoundOff{};
    allSoundOff.type = uasf::MidiEventType::ControlChange;
    allSoundOff.data1 = kAllSoundOff;
    allSoundOff.data2 = 0;
    allSoundOff.tick = 0;

    uasf::MidiEvent allNotesOff{};
    allNotesOff.type = uasf::MidiEventType::ControlChange;
    allNotesOff.data1 = kAllNotesOff;
    allNotesOff.data2 = 0;
    allNotesOff.tick = 0;

    for (uint8_t ch = 0; ch <= kMaxChannel; ++ch) {
        allSoundOff.channel = ch;
        allNotesOff.channel = ch;
        sendCallback(allSoundOff);
        sendCallback(allNotesOff);
    }

    // Phase 3: Clear active note tracking
    for (auto& low : channel_notes_low_) low.store(0, std::memory_order_release);
    for (auto& high : channel_notes_high_) high.store(0, std::memory_order_release);
    active_note_count_.store(0, std::memory_order_release);
}

void PanicHandler::flushActiveNotes(MidiEventCallback sendCallback) noexcept {
    if (!sendCallback) return;

    uasf::MidiEvent noteOff{};
    noteOff.type = uasf::MidiEventType::NoteOff;
    noteOff.data2 = 0;
    noteOff.tick = 0;

    for (uint8_t ch = 0; ch <= kMaxChannel; ++ch) {
        uint64_t low = channel_notes_low_[ch].exchange(0, std::memory_order_acq_rel);
        while (low) {
            uint8_t bit = static_cast<uint8_t>(__builtin_ctzll(low));
            low &= (low - 1);
            noteOff.channel = ch;
            noteOff.data1 = bit;
            sendCallback(noteOff);
        }
        uint64_t high = channel_notes_high_[ch].exchange(0, std::memory_order_acq_rel);
        while (high) {
            uint8_t bit = static_cast<uint8_t>(__builtin_ctzll(high));
            high &= (high - 1);
            noteOff.channel = ch;
            noteOff.data1 = static_cast<uint8_t>(bit + 64);
            sendCallback(noteOff);
        }
    }
    active_note_count_.store(0, std::memory_order_release);
}

bool PanicHandler::hasActiveNotes() const noexcept {
    return active_note_count_.load(std::memory_order_acquire) > 0;
}

bool PanicHandler::isNoteActive(uint8_t channel, uint8_t note) const noexcept {
    if (channel > kMaxChannel || note > 127) return false;
    if (note < 64) {
        return (channel_notes_low_[channel].load(std::memory_order_acquire) >> note) & 1ULL;
    }
    return (channel_notes_high_[channel].load(std::memory_order_acquire) >> (note - 64)) & 1ULL;
}

} // namespace ai_arranger::midi

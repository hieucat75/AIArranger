#include "engine/arranger/section_sequencer.h"

namespace ai_arranger::arranger {

SectionSequencer::SectionSequencer() = default;

void SectionSequencer::setSections(const uasf::SectionDefinition* sections, size_t count) noexcept {
    sections_ = sections;
    section_count_ = count;
}

void SectionSequencer::queueSection(int index) noexcept {
    queued_section_.store(index, std::memory_order_release);
}

void SectionSequencer::queueFill() noexcept {
    // Find the first Fill section
    for (size_t i = 0; i < section_count_; ++i) {
        if (sections_ && sections_[i].type >= uasf::SectionType::Fill1 &&
            sections_[i].type <= uasf::SectionType::Fill4) {
            queued_section_.store(static_cast<int>(i), std::memory_order_release);
            return;
        }
    }
}

void SectionSequencer::queueEnding() noexcept {
    // Find the first Ending section
    for (size_t i = 0; i < section_count_; ++i) {
        if (sections_ && sections_[i].type >= uasf::SectionType::Ending1 &&
            sections_[i].type <= uasf::SectionType::Ending3) {
            queued_section_.store(static_cast<int>(i), std::memory_order_release);
            return;
        }
    }
}

SequencerState SectionSequencer::getState() const noexcept {
    return state_.load(std::memory_order_acquire);
}

int SectionSequencer::getCurrentSectionIndex() const noexcept {
    return current_section_.load(std::memory_order_acquire);
}

int SectionSequencer::getQueuedSectionIndex() const noexcept {
    return queued_section_.load(std::memory_order_acquire);
}

bool SectionSequencer::isQueued() const noexcept {
    return queued_section_.load(std::memory_order_acquire) >= 0;
}

bool SectionSequencer::advance(int64_t currentTick, int64_t barSize) noexcept {
    if (!sections_ || section_count_ == 0) return false;

    // Check if we've crossed a bar boundary
    int64_t currentBar = currentTick / barSize;
    int64_t lastBar = last_bar_start_.load(std::memory_order_acquire) / barSize;

    if (currentBar > lastBar) {
        last_bar_start_.store(currentBar * barSize, std::memory_order_release);

        // Execute queued section switch on bar boundary
        int queued = queued_section_.load(std::memory_order_acquire);
        if (queued >= 0 && queued < static_cast<int>(section_count_)) {
            current_section_.store(queued, std::memory_order_release);
            queued_section_.store(-1, std::memory_order_release);

            // Update state based on section type
            auto st = sections_[queued].type;
            if (st >= uasf::SectionType::Ending1 && st <= uasf::SectionType::Ending3) {
                state_.store(SequencerState::PlayingEnding, std::memory_order_release);
            } else if (st >= uasf::SectionType::Fill1 && st <= uasf::SectionType::Fill4) {
                state_.store(SequencerState::PlayingFill, std::memory_order_release);
            } else if (st >= uasf::SectionType::Main1 && st <= uasf::SectionType::Main4) {
                state_.store(SequencerState::PlayingMain, std::memory_order_release);
            } else if (st >= uasf::SectionType::Intro1 && st <= uasf::SectionType::Intro3) {
                state_.store(SequencerState::PlayingIntro, std::memory_order_release);
            }

            return true; // Section switched
        }
    }

    return false;
}

void SectionSequencer::setCurrentChord(const Chord& chord) noexcept {
    current_chord_.store(
        (static_cast<uint32_t>(chord.type) & 0xFF) |
        ((static_cast<uint32_t>(chord.root) & 0xFF) << 8),
        std::memory_order_release
    );
}

Chord SectionSequencer::getCurrentChord() const noexcept {
    uint32_t data = current_chord_.load(std::memory_order_acquire);
    Chord c;
    c.type = static_cast<ChordType>(data & 0xFF);
    c.root = static_cast<uint8_t>((data >> 8) & 0xFF);
    c.bass = 0;
    c.octave = 0;
    return c;
}

void SectionSequencer::panic() noexcept {
    state_.store(SequencerState::Panic, std::memory_order_release);
    current_section_.store(-1, std::memory_order_release);
    queued_section_.store(-1, std::memory_order_release);
}

} // namespace ai_arranger::arranger

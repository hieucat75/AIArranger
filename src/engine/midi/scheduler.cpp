#include "engine/midi/scheduler.h"

namespace ai_arranger::midi {

MidiScheduler::MidiScheduler() = default;

void MidiScheduler::setOutputCallback(MidiEventCallback callback) noexcept {
    callback_ = std::move(callback);
}

bool MidiScheduler::scheduleEvent(const uasf::MidiEvent& event) noexcept {
    return queue_.push(event);
}

void MidiScheduler::scheduleEvents(const uasf::MidiEvent* events, size_t count) noexcept {
    for (size_t i = 0; i < count; ++i) {
        if (!queue_.push(events[i])) {
            break; // Queue full, drop remaining
        }
    }
}

void MidiScheduler::advanceTo(int64_t currentTick) noexcept {
    if (!callback_) return;

    uasf::MidiEvent event;
    while (queue_.pop(event)) {
        if (event.tick <= currentTick) {
            callback_(event);
        } else {
            // Event is in the future — push it back (rare, only on tick mismatch)
            queue_.push(event);
            break;
        }
    }
    last_dispatched_tick_.store(currentTick, std::memory_order_release);
}

void MidiScheduler::clear() noexcept {
    queue_.clear();
}

size_t MidiScheduler::pendingCount() const noexcept {
    return queue_.size();
}

bool MidiScheduler::isFull() noexcept {
    uasf::MidiEvent dummy{};
    return !queue_.push(dummy); // Try push, pop if succeeded
}

} // namespace ai_arranger::midi

#ifndef AI_ARRANGER_MIDI_SCHEDULER_H
#define AI_ARRANGER_MIDI_SCHEDULER_H

#include "engine/uasf/types.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <atomic>
#include <functional>

namespace ai_arranger::midi {

/**
 * Lock-free bounded SPSC (Single Producer, Single Consumer) queue
 * for MIDI events. Designed for realtime safety:
 *
 * - No malloc
 * - No mutex
 * - No wait
 * - Fixed capacity at compile time
 *
 * Producer = arranger engine (non-realtime or realtime-safe path)
 * Consumer = MIDI output callback (realtime path)
 */
template <typename T, size_t Capacity = 8192>
class LockFreeQueue {
public:
    LockFreeQueue() = default;

    bool push(const T& item) noexcept {
        const auto currentTail = tail_.load(std::memory_order_relaxed);
        const auto nextTail = nextIndex(currentTail);
        if (nextTail == head_.load(std::memory_order_acquire)) {
            return false; // Full
        }
        buffer_[currentTail] = item;
        tail_.store(nextTail, std::memory_order_release);
        return true;
    }

    bool pop(T& item) noexcept {
        const auto currentHead = head_.load(std::memory_order_relaxed);
        if (currentHead == tail_.load(std::memory_order_acquire)) {
            return false; // Empty
        }
        item = buffer_[currentHead];
        head_.store(nextIndex(currentHead), std::memory_order_release);
        return true;
    }

    size_t size() const noexcept {
        const auto h = head_.load(std::memory_order_acquire);
        const auto t = tail_.load(std::memory_order_acquire);
        if (t >= h) return t - h;
        return Capacity - h + t;
    }

    bool empty() const noexcept {
        return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire);
    }

    void clear() noexcept {
        head_.store(tail_.load(std::memory_order_acquire), std::memory_order_release);
    }

    size_t capacity() const noexcept { return Capacity - 1; }

private:
    size_t nextIndex(size_t i) const noexcept {
        return (i + 1) % Capacity;
    }

    std::array<T, Capacity> buffer_{};
    std::atomic<size_t> head_{0};
    std::atomic<size_t> tail_{0};
};

// ── MIDI Scheduler ──────────────────────────────────────────────────

using MidiEventCallback = std::function<void(const uasf::MidiEvent&)>;

class MidiScheduler {
public:
    static constexpr size_t kDefaultQueueSize = 8192;

    MidiScheduler();

    // ── Configuration ───────────────────────────────────────────────
    void setOutputCallback(MidiEventCallback callback) noexcept;

    // ── Producer (scheduler thread / arranger) ──────────────────────
    bool scheduleEvent(const uasf::MidiEvent& event) noexcept;
    void scheduleEvents(const uasf::MidiEvent* events, size_t count) noexcept;

    // ── Consumer (audio/MIDI callback) ────────────────────────────
    // Advance the scheduler to the given tick position.
    // All events <= currentTick will be dispatched via the callback.
    void advanceTo(int64_t currentTick) noexcept;

    // ── Control ─────────────────────────────────────────────────────
    void clear() noexcept;
    [[nodiscard]] size_t pendingCount() const noexcept;
    [[nodiscard]] bool isFull() noexcept;

private:
    LockFreeQueue<uasf::MidiEvent, kDefaultQueueSize> queue_;
    MidiEventCallback callback_{nullptr};
    std::atomic<int64_t> last_dispatched_tick_{0};
};

} // namespace ai_arranger::midi

#endif // AI_ARRANGER_MIDI_SCHEDULER_H

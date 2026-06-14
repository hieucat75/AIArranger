#ifndef AI_ARRANGER_CONTROL_UI_EVENT_QUEUE_H
#define AI_ARRANGER_CONTROL_UI_EVENT_QUEUE_H

#include <atomic>
#include <cstddef>

// ── Generic lock-free SPSC ring (Gate 14 Task C) ─────────────────────
//
// Single-producer / single-consumer queue for hand-off between the UI thread
// and the engine thread. Pre-allocated, power-of-two capacity, one reserved slot
// to disambiguate full/empty. push/pop are wait-free, non-blocking, no alloc.

namespace ai_arranger::control {

template <typename T, size_t Capacity>
class UiEventQueue {
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be power of two");
public:
    // Producer. Returns false if full (caller decides to drop or retry).
    bool push(const T& item) noexcept {
        const size_t head = head_.load(std::memory_order_relaxed);
        const size_t next = (head + 1) & (Capacity - 1);
        if (next == tail_.load(std::memory_order_acquire)) return false;
        buf_[head] = item;
        head_.store(next, std::memory_order_release);
        return true;
    }

    // Consumer. Returns false if empty.
    bool pop(T& out) noexcept {
        const size_t tail = tail_.load(std::memory_order_relaxed);
        if (tail == head_.load(std::memory_order_acquire)) return false;
        out = buf_[tail];
        tail_.store((tail + 1) & (Capacity - 1), std::memory_order_release);
        return true;
    }

    bool empty() const noexcept {
        return tail_.load(std::memory_order_acquire) ==
               head_.load(std::memory_order_acquire);
    }

    static constexpr size_t capacity() noexcept { return Capacity - 1; }

private:
    T buf_[Capacity]{};
    std::atomic<size_t> head_{0};
    std::atomic<size_t> tail_{0};
};

} // namespace ai_arranger::control

#endif // AI_ARRANGER_CONTROL_UI_EVENT_QUEUE_H

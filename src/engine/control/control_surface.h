#ifndef AI_ARRANGER_CONTROL_CONTROL_SURFACE_H
#define AI_ARRANGER_CONTROL_CONTROL_SURFACE_H

#include "engine/control/control_events.h"
#include <atomic>
#include <cstddef>

// ── Control surface abstraction (Gate 12 Task G) ─────────────────────
//
// A vendor-neutral, UI-free seam for performer input. ControlEventQueue is a
// lock-free SPSC ring used to hand control events from a producer (control
// thread / MIDI input / UI) to the consumer (dispatch thread). IControlSurface
// is the interface any front-end implements. No allocation after construction,
// no locks, no syscalls — safe on the realtime path.

namespace ai_arranger::control {

// Single-producer / single-consumer lock-free ring. Capacity must be a power of
// two; one slot is reserved to disambiguate full/empty.
template <size_t Capacity>
class ControlEventQueue {
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be power of two");
public:
    // Producer side. Returns false if full (event dropped — caller may log).
    bool push(const ControlEvent& ev) noexcept {
        const size_t head = head_.load(std::memory_order_relaxed);
        const size_t next = (head + 1) & (Capacity - 1);
        if (next == tail_.load(std::memory_order_acquire)) return false; // full
        buf_[head] = ev;
        head_.store(next, std::memory_order_release);
        return true;
    }

    // Consumer side. Returns false if empty.
    bool pop(ControlEvent& out) noexcept {
        const size_t tail = tail_.load(std::memory_order_relaxed);
        if (tail == head_.load(std::memory_order_acquire)) return false; // empty
        out = buf_[tail];
        tail_.store((tail + 1) & (Capacity - 1), std::memory_order_release);
        return true;
    }

    bool empty() const noexcept {
        return tail_.load(std::memory_order_acquire) ==
               head_.load(std::memory_order_acquire);
    }

private:
    ControlEvent buf_[Capacity]{};
    std::atomic<size_t> head_{0};  // producer writes
    std::atomic<size_t> tail_{0};  // consumer writes
};

// Interface any front-end implements (MIDI adapter, UI, test harness).
class IControlSurface {
public:
    virtual ~IControlSurface() = default;
    // Dequeue the next pending event; returns false if none. Realtime-safe.
    virtual bool poll(ControlEvent& out) noexcept = 0;
};

} // namespace ai_arranger::control

#endif // AI_ARRANGER_CONTROL_CONTROL_SURFACE_H

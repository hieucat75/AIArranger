#ifndef AI_ARRANGER_TRACE_MPSC_RING_H
#define AI_ARRANGER_TRACE_MPSC_RING_H

#include <atomic>
#include <cstddef>
#include <cstdint>

// ── Bounded lock-free MPSC ring (trace buffer) ───────────────────────
//
// Generalises the repo's single-producer LockFreeQueue (engine/midi/scheduler.h)
// to MULTIPLE realtime producer threads with ONE non-realtime consumer (the
// collector) — required because the five latency waypoints fire from different
// threads (CoreMIDI read thread, audio tick thread, MIDI dispatch thread).
//
// Algorithm: Dmitry Vyukov's bounded MPMC queue, single-consumer here. Each cell
// carries a sequence stamp; producers claim a slot with one fetch_add + CAS loop,
// and a full ring is detected without blocking — try_push returns false so the
// caller drops the record and bumps a counter (see LatencyTracer). Properties on
// the producer (RT) path:
//   - no malloc, no mutex, no syscall, no unbounded wait
//   - full ring → immediate false (never blocks the audio/read/dispatch thread)
//   - T must be trivially copyable POD (enforced by the tracer's static_assert)
//
// Capacity must be a power of two.

namespace ai_arranger::trace {

template <typename T, size_t Capacity>
class MpscRing {
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be power of two");
    static_assert(Capacity >= 2, "Capacity must be >= 2");

public:
    MpscRing() noexcept {
        for (size_t i = 0; i < Capacity; ++i) {
            cells_[i].seq.store(i, std::memory_order_relaxed);
        }
        enqueue_pos_.store(0, std::memory_order_relaxed);
        dequeue_pos_.store(0, std::memory_order_relaxed);
    }

    MpscRing(const MpscRing&) = delete;
    MpscRing& operator=(const MpscRing&) = delete;

    // Producer (any RT thread). Returns false if the ring is full (record
    // dropped). Wait-free amortised; the CAS loop only spins under genuine
    // cross-thread contention on the same slot and never blocks.
    bool try_push(const T& item) noexcept {
        Cell* cell;
        size_t pos = enqueue_pos_.load(std::memory_order_relaxed);
        for (;;) {
            cell = &cells_[pos & (Capacity - 1)];
            const size_t seq = cell->seq.load(std::memory_order_acquire);
            const intptr_t diff =
                static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos);
            if (diff == 0) {
                if (enqueue_pos_.compare_exchange_weak(
                        pos, pos + 1, std::memory_order_relaxed)) {
                    break; // claimed this slot
                }
                // lost the race; pos was reloaded by compare_exchange_weak
            } else if (diff < 0) {
                return false; // ring full — consumer has not caught up
            } else {
                pos = enqueue_pos_.load(std::memory_order_relaxed);
            }
        }
        cell->value = item;
        cell->seq.store(pos + 1, std::memory_order_release);
        return true;
    }

    // Consumer (single, non-RT collector thread). Returns false if empty.
    bool try_pop(T& out) noexcept {
        Cell* cell;
        size_t pos = dequeue_pos_.load(std::memory_order_relaxed);
        for (;;) {
            cell = &cells_[pos & (Capacity - 1)];
            const size_t seq = cell->seq.load(std::memory_order_acquire);
            const intptr_t diff =
                static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos + 1);
            if (diff == 0) {
                if (dequeue_pos_.compare_exchange_weak(
                        pos, pos + 1, std::memory_order_relaxed)) {
                    break;
                }
            } else if (diff < 0) {
                return false; // empty
            } else {
                pos = dequeue_pos_.load(std::memory_order_relaxed);
            }
        }
        out = cell->value;
        cell->seq.store(pos + Capacity, std::memory_order_release);
        return true;
    }

    static constexpr size_t capacity() noexcept { return Capacity; }

private:
    struct Cell {
        std::atomic<size_t> seq;
        T                   value;
    };

    // Pad the two hot cursors apart so producer/consumer don't false-share.
    alignas(64) std::atomic<size_t> enqueue_pos_{0};
    alignas(64) std::atomic<size_t> dequeue_pos_{0};
    alignas(64) Cell cells_[Capacity];
};

} // namespace ai_arranger::trace

#endif // AI_ARRANGER_TRACE_MPSC_RING_H

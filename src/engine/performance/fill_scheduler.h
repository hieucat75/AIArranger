#ifndef AI_ARRANGER_PERFORMANCE_FILL_SCHEDULER_H
#define AI_ARRANGER_PERFORMANCE_FILL_SCHEDULER_H

#include <atomic>
#include <cstdint>

// ── Fill queueing system (Gate 12 Task C) ────────────────────────────
//
// Queue a fill (optionally targeting the next variation); it fires once on the
// next bar boundary. Rapid/repeated fill presses COLLAPSE to a single pending
// fill (no stacking, no double-fire); the last-queued target wins so a queued
// variation is never skipped. Lock-free (atomic), no audio.

namespace ai_arranger::performance {

// Target after the fill: kReturn = back to the current variation, or a 0-based
// variation index to advance to.
inline constexpr int kFillReturn = -1;

struct FillFire {
    bool fired = false;
    int  target_variation = kFillReturn;
};

class FillScheduler {
public:
    // Queue (or re-queue) a fill. Collapses: many calls -> one pending fill.
    void queueFill(int targetVariation = kFillReturn) noexcept;

    bool hasPending() const noexcept;
    int  pendingTarget() const noexcept;   // valid only if hasPending()

    // Call on a bar boundary: fires the pending fill once (clears it) and reports
    // the target. Returns {false,...} if nothing was pending.
    FillFire onBarBoundary() noexcept;

    void reset() noexcept;

private:
    static constexpr int64_t kNone = INT64_MIN;
    // Encodes "no pending" (kNone) or a target (>= kFillReturn).
    std::atomic<int64_t> pending_{kNone};
};

} // namespace ai_arranger::performance

#endif // AI_ARRANGER_PERFORMANCE_FILL_SCHEDULER_H

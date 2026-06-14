#include "engine/performance/fill_scheduler.h"

namespace ai_arranger::performance {

void FillScheduler::queueFill(int targetVariation) noexcept {
    // Last-write-wins overwrite -> repeated presses collapse to one pending fill
    // while honouring the most recent target (no skipped variation).
    pending_.store(static_cast<int64_t>(targetVariation), std::memory_order_release);
}

bool FillScheduler::hasPending() const noexcept {
    return pending_.load(std::memory_order_acquire) != kNone;
}

int FillScheduler::pendingTarget() const noexcept {
    const int64_t v = pending_.load(std::memory_order_acquire);
    return v == kNone ? kFillReturn : static_cast<int>(v);
}

FillFire FillScheduler::onBarBoundary() noexcept {
    const int64_t v = pending_.exchange(kNone, std::memory_order_acq_rel);
    if (v == kNone) return {false, kFillReturn};
    return {true, static_cast<int>(v)};
}

void FillScheduler::reset() noexcept {
    pending_.store(kNone, std::memory_order_release);
}

} // namespace ai_arranger::performance

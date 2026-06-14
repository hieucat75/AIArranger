#include "performance/telemetry/transition_state.h"

namespace ai_arranger::performance {

TransitionViz TransitionFeel::update(int64_t currentTick, int64_t barSize) noexcept {
    TransitionViz v;
    if (barSize <= 0) return v;

    const int64_t bar = currentTick / barSize;
    const int64_t inBar = currentTick % barSize;
    v.bar_progress = static_cast<double>(inBar) / static_cast<double>(barSize);
    v.ticks_to_fire = barSize - inBar;
    v.kind = pending_;
    v.pending = isPending();
    v.arming = v.pending && v.ticks_to_fire <= pre_roll_;

    if (last_bar_ < 0) last_bar_ = bar;

    // Crossed a bar boundary with something pending -> it fires now.
    if (bar > last_bar_) {
        last_bar_ = bar;
        if (isPending()) {
            v.kind = pending_;
            v.pending = true;
            v.ticks_to_fire = 0;
            v.arming = false;
            pending_ = TransitionKind::None;   // consumed
        }
    }
    return v;
}

} // namespace ai_arranger::performance

#ifndef AI_ARRANGER_PERFORMANCE_TELEMETRY_TRANSITION_STATE_H
#define AI_ARRANGER_PERFORMANCE_TELEMETRY_TRANSITION_STATE_H

#include <cstdint>

// ── Transition feel / visualization (Gate 15 Task D) ─────────────────
//
// Tracks a queued transition (fill/variation/intro/ending) relative to the bar
// grid and exposes visualization state: how far into the bar, how many ticks to
// the boundary, and whether it is "arming" (inside a pre-roll window so the UI
// can flash before it fires). The sequencer remains the actual bar-boundary
// authority — this is telemetry + a pre-roll signal, no scheduling. Pure +
// deterministic.

namespace ai_arranger::performance {

enum class TransitionKind : uint8_t { None, Fill, Variation, Intro, Ending };

struct TransitionViz {
    TransitionKind kind = TransitionKind::None;
    bool    pending = false;
    int64_t ticks_to_fire = 0;   // ticks until the next bar boundary
    double  bar_progress = 0.0;  // 0..1 within the current bar
    bool    arming = false;      // pending AND within the pre-roll window
};

class TransitionFeel {
public:
    void setPreRollTicks(int64_t ticks) noexcept { pre_roll_ = ticks < 0 ? 0 : ticks; }
    int64_t preRollTicks() const noexcept { return pre_roll_; }

    void queue(TransitionKind kind) noexcept { pending_ = kind; }
    void clear() noexcept { pending_ = TransitionKind::None; }
    bool isPending() const noexcept { return pending_ != TransitionKind::None; }

    // Compute viz at currentTick. If a bar boundary is crossed and something is
    // pending, the pending transition "fires" (viz.pending stays true for this
    // call with ticks_to_fire==0, then internal pending clears). Pure w.r.t.
    // inputs given the internal last-bar marker.
    TransitionViz update(int64_t currentTick, int64_t barSize) noexcept;

private:
    TransitionKind pending_ = TransitionKind::None;
    int64_t pre_roll_ = 240;       // default ~half a beat @480ppqn
    int64_t last_bar_ = -1;
};

} // namespace ai_arranger::performance

#endif // AI_ARRANGER_PERFORMANCE_TELEMETRY_TRANSITION_STATE_H

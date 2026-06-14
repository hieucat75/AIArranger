#ifndef KORG_VALIDATION_TRANSITION_VALIDATOR_H
#define KORG_VALIDATION_TRANSITION_VALIDATOR_H

#include <cstdint>
#include <string>
#include <vector>

// ── Section / fill transition validator (Gate 11 Task D) ─────────────
//
// Checks that section/fill transitions land on the expected bar boundary.
// Generic by label, so fill entry/exit are just two transitions.

namespace ai_arranger::korg_validation {

struct TransitionInput {
    std::string label;       // e.g. "Main A->B", "Fill in", "Fill out"
    uint64_t    actual_tick; // first event tick of the new section/fill
    uint64_t    expected_tick;
};

struct TransitionCheck {
    std::string label;
    uint64_t    expected_tick;
    uint64_t    actual_tick;
    int64_t     delta;        // actual - expected (ticks)
    uint64_t    grid_offset;  // distance to nearest bar boundary (ticks)
    bool        on_boundary;  // grid_offset <= tolerance
    bool        on_time;      // |delta| <= tolerance
};

std::vector<TransitionCheck> validateTransitions(
    const std::vector<TransitionInput>& transitions,
    uint64_t barTicks,
    uint64_t toleranceTicks = 0);

} // namespace ai_arranger::korg_validation

#endif // KORG_VALIDATION_TRANSITION_VALIDATOR_H

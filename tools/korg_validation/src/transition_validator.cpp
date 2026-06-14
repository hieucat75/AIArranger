#include "korg_validation/transition_validator.h"

#include <cstdlib>

namespace ai_arranger::korg_validation {

std::vector<TransitionCheck> validateTransitions(
    const std::vector<TransitionInput>& transitions,
    uint64_t barTicks,
    uint64_t toleranceTicks) {

    std::vector<TransitionCheck> out;
    out.reserve(transitions.size());

    for (const auto& t : transitions) {
        TransitionCheck c;
        c.label = t.label;
        c.expected_tick = t.expected_tick;
        c.actual_tick = t.actual_tick;
        c.delta = static_cast<int64_t>(t.actual_tick) -
                  static_cast<int64_t>(t.expected_tick);

        if (barTicks > 0) {
            const uint64_t rem = t.actual_tick % barTicks;
            c.grid_offset = rem < (barTicks - rem) ? rem : (barTicks - rem);
        } else {
            c.grid_offset = 0;
        }

        c.on_boundary = c.grid_offset <= toleranceTicks;
        const uint64_t adelta = c.delta < 0 ? static_cast<uint64_t>(-c.delta)
                                            : static_cast<uint64_t>(c.delta);
        c.on_time = adelta <= toleranceTicks;
        out.push_back(std::move(c));
    }
    return out;
}

} // namespace ai_arranger::korg_validation

#include "engine/music/groove.h"

namespace ai_arranger::music {

uint64_t applySwing(uint64_t tick, uint32_t ticksPerBeat,
                    uint8_t swingPercent) noexcept {
    if (swingPercent > kMaxSwing) swingPercent = kMaxSwing;
    if (swingPercent <= kStraightSwing || ticksPerBeat == 0) return tick;

    const uint32_t eighth = ticksPerBeat / 2;
    if (eighth == 0) return tick;

    const uint64_t beatBase = (tick / ticksPerBeat) * ticksPerBeat;
    const uint64_t offset   = tick - beatBase;   // 0 .. ticksPerBeat-1

    // On-beat half stays put; the off-beat half slides later by a fixed delay
    // so the whole second half of the beat shifts toward the triplet position.
    if (offset >= eighth) {
        const uint64_t delay =
            (static_cast<uint64_t>(swingPercent - kStraightSwing) * ticksPerBeat) / 100;
        return tick + delay;
    }
    return tick;
}

} // namespace ai_arranger::music

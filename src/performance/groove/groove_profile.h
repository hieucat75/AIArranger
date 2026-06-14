#ifndef AI_ARRANGER_PERFORMANCE_GROOVE_GROOVE_PROFILE_H
#define AI_ARRANGER_PERFORMANCE_GROOVE_GROOVE_PROFILE_H

#include "performance/groove/deterministic_rng.h"
#include <cstdint>

// ── Groove humanization profiles (Gate 15 Task C) ────────────────────
//
// A profile maps a note (its in-beat position + velocity) to a deterministic
// micro-timing offset (ticks) + velocity adjustment, using a seeded RNG. Same
// seed + same call sequence => identical adjustments (replay-safe). Pure,
// realtime-safe (integer math, no alloc/lock).

namespace ai_arranger::performance {

struct GrooveAdjust {
    int tick_offset = 0;  // +late / -early (ticks)
    int vel_adjust = 0;   // velocity delta
};

class IGrooveProfile {
public:
    virtual ~IGrooveProfile() = default;
    // `phase` is the note's position within the beat in ticks [0, ticksPerBeat).
    virtual GrooveAdjust adjust(uint32_t phase, uint32_t ticksPerBeat,
                                uint8_t velocity, DeterministicRng& rng) const noexcept = 0;
    virtual const char* name() const noexcept = 0;
};

// 5 built-in profiles (stateless singletons).
const IGrooveProfile& grooveTight() noexcept;
const IGrooveProfile& grooveLaidBack() noexcept;
const IGrooveProfile& grooveSwingLight() noexcept;
const IGrooveProfile& grooveLivePop() noexcept;
const IGrooveProfile& grooveAcousticSoft() noexcept;

// Lookup by index 0..4 (Tight/LaidBack/SwingLight/LivePop/AcousticSoft).
const IGrooveProfile& grooveByIndex(int index) noexcept;
constexpr int kGrooveProfileCount = 5;

} // namespace ai_arranger::performance

#endif // AI_ARRANGER_PERFORMANCE_GROOVE_GROOVE_PROFILE_H

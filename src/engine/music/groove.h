#ifndef AI_ARRANGER_MUSIC_GROOVE_H
#define AI_ARRANGER_MUSIC_GROOVE_H

#include <cstdint>

// ── Swing / shuffle groove model (Gate 10B Task B) ───────────────────
//
// Yamaha-style swing: the straight off-beat 8th (at 0.5 of the beat) slides
// later toward a triplet feel. The swing percentage is the fraction of the
// beat at which the off-beat 8th lands:
//   50%  = straight (no shift)
//   ~67% = triplet / shuffle feel (off-beat at 2/3 of the beat)
//   75%  = hard/dotted shuffle
//
// `applySwing` shifts the off-beat half of each beat later by
// (swing% - 50%) * ticksPerBeat, preserving the on-beat positions and the
// internal spacing of the off-beat half. Pure + realtime-safe: integer math
// only, no allocation, no locks — safe on the dispatch / audio path.

namespace ai_arranger::music {

constexpr uint8_t kStraightSwing = 50;   // no swing
constexpr uint8_t kMaxSwing      = 75;   // clamp ceiling (musical limit)

// Returns the swung tick for a beat-relative `tick`. `ticksPerBeat` is the
// resolution (ticks per quarter note). `swingPercent` is clamped to
// [50, 75]; <= 50 returns the tick unchanged.
uint64_t applySwing(uint64_t tick, uint32_t ticksPerBeat,
                    uint8_t swingPercent) noexcept;

} // namespace ai_arranger::music

#endif // AI_ARRANGER_MUSIC_GROOVE_H

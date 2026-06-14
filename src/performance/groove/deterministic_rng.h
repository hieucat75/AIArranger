#ifndef AI_ARRANGER_PERFORMANCE_GROOVE_DETERMINISTIC_RNG_H
#define AI_ARRANGER_PERFORMANCE_GROOVE_DETERMINISTIC_RNG_H

#include <cstdint>

// ── Deterministic seeded PRNG (Gate 15 Task C) ───────────────────────
//
// splitmix64 — fully reproducible: same seed + same call sequence => identical
// values. NO std::random_device / system entropy. Realtime-safe (integer only).

namespace ai_arranger::performance {

class DeterministicRng {
public:
    explicit DeterministicRng(uint64_t seed = 0x9E3779B97F4A7C15ULL) noexcept
        : state_(seed) {}

    void reseed(uint64_t seed) noexcept { state_ = seed; }

    uint64_t next() noexcept {
        uint64_t z = (state_ += 0x9E3779B97F4A7C15ULL);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }

    // Uniform int in [lo, hi] inclusive (deterministic).
    int range(int lo, int hi) noexcept {
        if (hi <= lo) return lo;
        const uint64_t span = static_cast<uint64_t>(hi - lo) + 1;
        return lo + static_cast<int>(next() % span);
    }

private:
    uint64_t state_;
};

} // namespace ai_arranger::performance

#endif // AI_ARRANGER_PERFORMANCE_GROOVE_DETERMINISTIC_RNG_H

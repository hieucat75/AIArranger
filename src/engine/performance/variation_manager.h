#ifndef AI_ARRANGER_PERFORMANCE_VARIATION_MANAGER_H
#define AI_ARRANGER_PERFORMANCE_VARIATION_MANAGER_H

#include <atomic>
#include <cstdint>

// ── Variation transition engine (Gate 12 Task D) ─────────────────────
//
// Manages Main A/B/C/D selection with a pending queue resolved on the bar
// boundary. Deterministic: a re-queue overwrites the pending target (last wins),
// and resolving to the current variation is a no-op (no duplicate apply).
// An immediate path is available for policy=immediate. Lock-free, no audio.

namespace ai_arranger::performance {

enum class Variation : uint8_t { A = 0, B = 1, C = 2, D = 3 };

struct VariationResolve {
    bool changed = false;
    Variation variation = Variation::A;
};

class VariationManager {
public:
    Variation current() const noexcept {
        return static_cast<Variation>(current_.load(std::memory_order_acquire));
    }

    bool hasPending() const noexcept {
        return pending_.load(std::memory_order_acquire) >= 0;
    }
    Variation pending() const noexcept {
        const int p = pending_.load(std::memory_order_acquire);
        return p >= 0 ? static_cast<Variation>(p) : current();
    }

    // Queue a variation for the next boundary (last-write-wins).
    void queue(Variation v) noexcept {
        pending_.store(static_cast<int>(v), std::memory_order_release);
    }

    // Resolve at a bar boundary: apply the pending variation once. changed=false
    // if nothing pending OR the pending target equals the current variation.
    VariationResolve onBarBoundary() noexcept;

    // Immediate switch (policy=immediate): apply now, clear any pending.
    VariationResolve setImmediate(Variation v) noexcept;

    void reset() noexcept {
        current_.store(0, std::memory_order_release);
        pending_.store(-1, std::memory_order_release);
    }

private:
    std::atomic<int> current_{0};   // Variation::A
    std::atomic<int> pending_{-1};  // -1 = none
};

} // namespace ai_arranger::performance

#endif // AI_ARRANGER_PERFORMANCE_VARIATION_MANAGER_H

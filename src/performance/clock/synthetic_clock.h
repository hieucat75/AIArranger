#ifndef AI_ARRANGER_PERFORMANCE_CLOCK_SYNTHETIC_CLOCK_H
#define AI_ARRANGER_PERFORMANCE_CLOCK_SYNTHETIC_CLOCK_H

#include "performance/clock/realtime_clock.h"
#include <atomic>

// ── Synthetic deterministic clock (Gate 15 Task A) ───────────────────
//
// Drives ticks from explicit advance() calls — no system clock, fully
// deterministic and replayable. Used for all CI tests (the CoreAudio clock is
// local-only). Zero jitter by construction.

namespace ai_arranger::performance {

class SyntheticClock final : public IRealtimeClock {
public:
    explicit SyntheticClock(uint32_t sampleRate = 48000) noexcept
        : sample_rate_(sampleRate) {}

    void start() noexcept override { running_.store(true, std::memory_order_release); }
    void stop() noexcept override  { running_.store(false, std::memory_order_release); }
    bool isRunning() const noexcept override { return running_.load(std::memory_order_acquire); }

    // Test/host pushes time forward (e.g. one audio block).
    void advance(uint64_t samples) noexcept {
        if (running_.load(std::memory_order_acquire))
            pending_.fetch_add(samples, std::memory_order_acq_rel);
    }

    uint64_t pollElapsedSamples() noexcept override {
        return pending_.exchange(0, std::memory_order_acq_rel);
    }

    uint32_t sampleRate() const noexcept override { return sample_rate_; }

private:
    uint32_t sample_rate_;
    std::atomic<bool>     running_{false};
    std::atomic<uint64_t> pending_{0};
};

} // namespace ai_arranger::performance

#endif // AI_ARRANGER_PERFORMANCE_CLOCK_SYNTHETIC_CLOCK_H

#ifndef AI_ARRANGER_PERFORMANCE_CLOCK_COREAUDIO_CLOCK_H
#define AI_ARRANGER_PERFORMANCE_CLOCK_COREAUDIO_CLOCK_H

#include "performance/clock/realtime_clock.h"
#include <atomic>

// ── CoreAudio / host-time clock (Gate 15 Task B, macOS) ──────────────
//
// Drives ticks from the mach host clock (the same time base CoreAudio render
// callbacks use). Each poll reads the TRUE elapsed host time and converts to
// samples — so there is no accumulated drift from a fixed sleep interval, giving
// much lower jitter than the 1ms prototype timer. Header-safe everywhere; the
// mach calls live in the .cpp (macOS). No audio session is opened, so it is
// safe to construct/poll headlessly; precise jitter is a local measurement.

namespace ai_arranger::performance {

class CoreAudioClock final : public IRealtimeClock {
public:
    explicit CoreAudioClock(uint32_t sampleRate = 48000) noexcept
        : sample_rate_(sampleRate) {}

    void start() noexcept override;
    void stop() noexcept override { running_.store(false, std::memory_order_release); }
    bool isRunning() const noexcept override { return running_.load(std::memory_order_acquire); }

    // True elapsed host time since the last poll, in samples. 0 while stopped.
    uint64_t pollElapsedSamples() noexcept override;

    uint32_t sampleRate() const noexcept override { return sample_rate_; }

private:
    uint32_t              sample_rate_;
    std::atomic<bool>     running_{false};
    std::atomic<uint64_t> last_host_{0};   // mach_absolute_time at last poll/start
};

} // namespace ai_arranger::performance

#endif // AI_ARRANGER_PERFORMANCE_CLOCK_COREAUDIO_CLOCK_H

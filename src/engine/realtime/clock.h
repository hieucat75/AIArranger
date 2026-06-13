#ifndef AI_ARRANGER_REALTIME_CLOCK_H
#define AI_ARRANGER_REALTIME_CLOCK_H

#include <cstdint>
#include <atomic>
#include <chrono>

namespace ai_arranger::realtime {

/**
 * Sample-position-based realtime clock.
 *
 * The clock derives position from an accumulated sample count,
 * NOT from OS timers. This matches CoreAudio render callback
 * semantics and ensures timing accuracy is bounded by the
 * audio buffer size, not the OS scheduler.
 *
 * Architecture rule (ADR-013):
 * - No malloc in realtime path
 * - No mutex in realtime path
 * - No file IO in realtime path
 */

class RealtimeClock {
public:
    static constexpr uint32_t kDefaultSampleRate = 48000;
    static constexpr uint32_t kDefaultResolution = 480; // ticks per quarter note

    RealtimeClock();

    // ── Configuration (called outside realtime thread) ──────────────
    void setSampleRate(uint32_t sampleRate) noexcept;
    void setTempo(uint32_t bpm) noexcept;
    void setResolution(uint32_t ticksPerQuarter) noexcept;
    void setTimeSignature(uint32_t beatsPerBar, uint32_t beatNote) noexcept;

    // ── Real-time safe (call from audio callback) ───────────────────
    void reset() noexcept;
    void advance(uint32_t numSamples) noexcept;
    void stop() noexcept;
    void start() noexcept;

    // ── Read-only queries (real-time safe) ───────────────────────────
    [[nodiscard]] int64_t getPosition() const noexcept;       // Current tick
    [[nodiscard]] int64_t getSamplePosition() const noexcept; // Current sample
    [[nodiscard]] uint32_t getTempo() const noexcept;
    [[nodiscard]] double getTicksPerSample() const noexcept;
    [[nodiscard]] bool isRunning() const noexcept;

    // ── Bar/beat calculation (not real-time safe if accuracy needed) ─
    [[nodiscard]] int32_t getCurrentBar(int64_t position) const noexcept;
    [[nodiscard]] int32_t getCurrentBeat(int64_t position) const noexcept;

private:
    // All state is lock-free: atomic for cross-thread visibility
    std::atomic<uint32_t> sample_rate_{kDefaultSampleRate};
    std::atomic<uint32_t> tempo_bpm_{120};
    std::atomic<uint32_t> resolution_{kDefaultResolution};
    std::atomic<uint32_t> beats_per_bar_{4};
    std::atomic<uint32_t> beat_note_{4};
    std::atomic<int64_t> sample_position_{0};
    std::atomic<bool> running_{false};

    // Derived (recalc on tempo/resolution/sampleRate change)
    std::atomic<double> ticks_per_sample_{0.0};
    std::atomic<int64_t> samples_per_beat_{0};
    std::atomic<int64_t> samples_per_bar_{0};

    void recalcDerived() noexcept;
};

} // namespace ai_arranger::realtime

#endif // AI_ARRANGER_REALTIME_CLOCK_H

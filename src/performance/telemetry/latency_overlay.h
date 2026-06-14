#ifndef AI_ARRANGER_PERFORMANCE_TELEMETRY_LATENCY_OVERLAY_H
#define AI_ARRANGER_PERFORMANCE_TELEMETRY_LATENCY_OVERLAY_H

#include <cstddef>
#include <cstdint>

// ── Live latency overlay (Gate 15 Task E) ────────────────────────────
//
// Rolling control→dispatch latency stats for the on-screen overlay: average,
// max spike, jitter (max−min over the window), and a fixed-bucket histogram.
// Pure/deterministic, fixed-size (no allocation).

namespace ai_arranger::performance {

class LatencyOverlay {
public:
    static constexpr int kWindow = 256;
    static constexpr int kBuckets = 16;

    void addSample(double ms) noexcept;
    void reset() noexcept { head_ = 0; count_ = 0; }

    int    samples() const noexcept { return count_; }
    double average() const noexcept;
    double maximum() const noexcept;
    double minimum() const noexcept;
    double jitter() const noexcept;     // max − min over the window

    // Histogram over [0, kBuckets*bucketMs); last bucket catches the overflow.
    void histogram(double bucketMs, int out[kBuckets]) const noexcept;

private:
    double ring_[kWindow]{};
    int head_ = 0;
    int count_ = 0;
};

} // namespace ai_arranger::performance

#endif // AI_ARRANGER_PERFORMANCE_TELEMETRY_LATENCY_OVERLAY_H

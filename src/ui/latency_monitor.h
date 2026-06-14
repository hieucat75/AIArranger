#ifndef AI_ARRANGER_UI_LATENCY_MONITOR_H
#define AI_ARRANGER_UI_LATENCY_MONITOR_H

#include <cstddef>
#include <cstdint>

// ── Latency monitor (Gate 14 Task D) ─────────────────────────────────
//
// Lightweight rolling latency stats for the telemetry panel: average, max, and
// jitter (max-min) over a fixed window. Fixed-size ring, no allocation.

namespace ai_arranger::ui {

class LatencyMonitor {
public:
    static constexpr size_t kWindow = 128;

    void addSample(double ms) noexcept {
        ring_[head_] = ms;
        head_ = (head_ + 1) % kWindow;
        if (count_ < kWindow) ++count_;
    }

    void reset() noexcept { head_ = 0; count_ = 0; }

    int samples() const noexcept { return static_cast<int>(count_); }

    double average() const noexcept {
        if (count_ == 0) return 0.0;
        double s = 0.0;
        for (size_t i = 0; i < count_; ++i) s += ring_[i];
        return s / static_cast<double>(count_);
    }
    double maximum() const noexcept {
        double m = 0.0;
        for (size_t i = 0; i < count_; ++i) if (ring_[i] > m) m = ring_[i];
        return m;
    }
    double jitter() const noexcept {  // max - min over the window
        if (count_ == 0) return 0.0;
        double mn = ring_[0], mx = ring_[0];
        for (size_t i = 1; i < count_; ++i) {
            if (ring_[i] < mn) mn = ring_[i];
            if (ring_[i] > mx) mx = ring_[i];
        }
        return mx - mn;
    }

private:
    double ring_[kWindow]{};
    size_t head_ = 0;
    size_t count_ = 0;
};

} // namespace ai_arranger::ui

#endif // AI_ARRANGER_UI_LATENCY_MONITOR_H

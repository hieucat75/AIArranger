#include "performance/telemetry/latency_overlay.h"

namespace ai_arranger::performance {

void LatencyOverlay::addSample(double ms) noexcept {
    ring_[head_] = ms;
    head_ = (head_ + 1) % kWindow;
    if (count_ < kWindow) ++count_;
}

double LatencyOverlay::average() const noexcept {
    if (count_ == 0) return 0.0;
    double s = 0.0;
    for (int i = 0; i < count_; ++i) s += ring_[i];
    return s / count_;
}

double LatencyOverlay::maximum() const noexcept {
    double m = 0.0;
    for (int i = 0; i < count_; ++i) if (ring_[i] > m) m = ring_[i];
    return m;
}

double LatencyOverlay::minimum() const noexcept {
    if (count_ == 0) return 0.0;
    double m = ring_[0];
    for (int i = 1; i < count_; ++i) if (ring_[i] < m) m = ring_[i];
    return m;
}

double LatencyOverlay::jitter() const noexcept {
    if (count_ == 0) return 0.0;
    return maximum() - minimum();
}

void LatencyOverlay::histogram(double bucketMs, int out[kBuckets]) const noexcept {
    for (int i = 0; i < kBuckets; ++i) out[i] = 0;
    if (bucketMs <= 0.0) return;
    for (int i = 0; i < count_; ++i) {
        int b = static_cast<int>(ring_[i] / bucketMs);
        if (b < 0) b = 0;
        if (b >= kBuckets) b = kBuckets - 1;  // overflow catch-all
        ++out[b];
    }
}

} // namespace ai_arranger::performance

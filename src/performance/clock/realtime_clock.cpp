#include "performance/clock/realtime_clock.h"
#include <cmath>

namespace ai_arranger::performance {

JitterStats computeJitter(const double* intervals, int count) noexcept {
    JitterStats s;
    if (count <= 0) return s;
    s.samples = count;
    double sum = 0.0;
    for (int i = 0; i < count; ++i) sum += intervals[i];
    s.mean = sum / count;
    double sq = 0.0;
    for (int i = 0; i < count; ++i) {
        const double d = intervals[i] - s.mean;
        const double ad = d < 0 ? -d : d;
        if (ad > s.maxAbsDev) s.maxAbsDev = ad;
        sq += d * d;
    }
    s.stddev = std::sqrt(sq / count);
    return s;
}

} // namespace ai_arranger::performance

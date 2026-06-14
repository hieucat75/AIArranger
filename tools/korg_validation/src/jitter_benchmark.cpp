#include "korg_validation/jitter_benchmark.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace ai_arranger::korg_validation {

JitterResult benchmarkJitter(const MidiCapture& cap,
                             uint64_t gridTicks,
                             double bpm) {
    JitterResult r;
    if (gridTicks == 0) return r;

    std::vector<double> errs;
    double sum = 0.0;
    for (const auto& e : cap.events) {
        if (!e.isNoteOn()) continue;
        const uint64_t rem = e.tick % gridTicks;
        const uint64_t off = rem < (gridTicks - rem) ? rem : (gridTicks - rem);
        const double ms = tickToMs(off, cap.ppqn, bpm);
        errs.push_back(ms);
        sum += ms;
        if (ms > r.max_ms) r.max_ms = ms;
    }

    r.events = static_cast<int>(errs.size());
    if (r.events == 0) return r;

    r.mean_abs_ms = sum / r.events;
    std::sort(errs.begin(), errs.end());
    // p95: smallest value with cumulative rank >= 95%.
    size_t idx = static_cast<size_t>(std::ceil(0.95 * r.events)) - 1;
    if (idx >= errs.size()) idx = errs.size() - 1;
    r.p95_ms = errs[idx];
    return r;
}

} // namespace ai_arranger::korg_validation

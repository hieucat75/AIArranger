#include "korg_validation/timing_analyzer.h"

#include <cmath>
#include <deque>
#include <map>

namespace ai_arranger::korg_validation {

TimingSummary analyzeTiming(const MidiCapture& engine,
                            const MidiCapture& reference,
                            double bpm,
                            double toleranceMs) {
    TimingSummary s;

    // Per-key FIFO of reference event ms, key = (status<<8 | data1).
    std::map<int, std::deque<double>> refQ;
    for (const auto& e : reference.events) {
        const int key = (static_cast<int>(e.status) << 8) | e.data1;
        refQ[key].push_back(tickToMs(e.tick, reference.ppqn, bpm));
    }

    double sum = 0.0, sumSq = 0.0;
    for (const auto& e : engine.events) {
        const int key = (static_cast<int>(e.status) << 8) | e.data1;
        auto it = refQ.find(key);
        if (it == refQ.end() || it->second.empty()) {
            ++s.orphan_engine;
            continue;
        }
        const double refMs = it->second.front();
        it->second.pop_front();
        const double engMs = tickToMs(e.tick, engine.ppqn, bpm);
        const double delta = engMs - refMs;

        s.deltas_ms.push_back(delta);
        ++s.matched;
        if (std::fabs(delta) <= toleranceMs) ++s.within_tolerance;
        sum += delta;
        sumSq += delta * delta;
        if (std::fabs(delta) > s.max_abs_ms) s.max_abs_ms = std::fabs(delta);
    }

    // Any reference events left unmatched.
    for (const auto& kv : refQ) s.orphan_reference += static_cast<int>(kv.second.size());

    if (s.matched > 0) {
        s.mean_ms = sum / s.matched;
        const double var = (sumSq / s.matched) - (s.mean_ms * s.mean_ms);
        s.stddev_ms = var > 0.0 ? std::sqrt(var) : 0.0;
    }
    return s;
}

} // namespace ai_arranger::korg_validation

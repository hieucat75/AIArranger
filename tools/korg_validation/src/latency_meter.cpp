#include "korg_validation/latency_meter.h"

namespace ai_arranger::korg_validation {

LatencyResult measureLatency(const MidiCapture& engine,
                             const std::vector<uint64_t>& chordChangeTicks,
                             double bpm) {
    LatencyResult r;
    double sum = 0.0;

    for (uint64_t change : chordChangeTicks) {
        // First NoteOn at or after the chord change.
        const TimedMidiEvent* hit = nullptr;
        for (const auto& e : engine.events) {
            if (e.tick >= change && e.isNoteOn()) { hit = &e; break; }
        }
        if (!hit) { ++r.missing; continue; }

        const double ms = tickToMs(hit->tick - change, engine.ppqn, bpm);
        r.latencies_ms.push_back(ms);
        ++r.events;
        sum += ms;
        if (ms > r.max_ms) r.max_ms = ms;
    }

    if (r.events > 0) r.mean_ms = sum / r.events;
    return r;
}

} // namespace ai_arranger::korg_validation

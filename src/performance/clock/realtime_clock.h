#ifndef AI_ARRANGER_PERFORMANCE_CLOCK_REALTIME_CLOCK_H
#define AI_ARRANGER_PERFORMANCE_CLOCK_REALTIME_CLOCK_H

#include <cstdint>

// ── Realtime tick-source abstraction (Gate 15 Task A) ────────────────
//
// Abstracts WHAT drives engine ticks (the time source), so the prototype 1ms
// timer can be replaced by a CoreAudio/host-time clock without touching the
// engine. The driver polls elapsed samples and feeds the engine clock. The
// engine's own sample→tick RealtimeClock is unchanged; this is the source above
// it. SyntheticClock (deterministic) backs all CI tests.

namespace ai_arranger::performance {

class IRealtimeClock {
public:
    virtual ~IRealtimeClock() = default;

    virtual void start() noexcept = 0;
    virtual void stop() noexcept = 0;
    virtual bool isRunning() const noexcept = 0;

    // Samples elapsed since the previous poll (0 while stopped). Realtime-safe.
    virtual uint64_t pollElapsedSamples() noexcept = 0;

    virtual uint32_t sampleRate() const noexcept = 0;
};

// ── Jitter statistics over inter-tick intervals (deterministic helper) ──
struct JitterStats {
    int    samples = 0;
    double mean = 0.0;     // mean interval
    double maxAbsDev = 0.0;// max |interval - mean|
    double stddev = 0.0;
};

JitterStats computeJitter(const double* intervals, int count) noexcept;

} // namespace ai_arranger::performance

#endif // AI_ARRANGER_PERFORMANCE_CLOCK_REALTIME_CLOCK_H

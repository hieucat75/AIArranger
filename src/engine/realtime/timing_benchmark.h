#ifndef AI_ARRANGER_TIMING_BENCHMARK_H
#define AI_ARRANGER_TIMING_BENCHMARK_H

#include "engine/realtime/clock.h"
#include "engine/midi/scheduler.h"
#include <vector>
#include <chrono>
#include <string>
#include <cstdint>

namespace ai_arranger::realtime {

/**
 * Timing benchmark for measuring MIDI scheduler jitter.
 *
 * Simulates an audio callback at various buffer sizes and tempos,
 * measuring the delta between expected and actual event delivery time.
 */

struct TimingMetrics {
    double mean_us{0.0};
    double min_us{0.0};
    double max_us{0.0};
    double p50_us{0.0};
    double p90_us{0.0};
    double p99_us{0.0};
    double p999_us{0.0};
    double stddev_us{0.0};
    uint64_t total_events{0};
    uint64_t missed_events{0};
    double test_duration_ms{0.0};
};

struct BenchmarkConfig {
    uint32_t sample_rate = 48000;
    uint32_t tempo_bpm = 120;
    uint32_t buffer_size = 256;      // samples per callback
    uint32_t duration_seconds = 10;  // test duration
};

class TimingBenchmark {
public:
    TimingBenchmark();

    void configure(const BenchmarkConfig& config) noexcept;
    TimingMetrics run() noexcept;

    static std::string metricsToString(const TimingMetrics& metrics) noexcept;
    static std::string generateReport(const TimingMetrics& metrics,
                                       const BenchmarkConfig& config) noexcept;

private:
    BenchmarkConfig config_;
    RealtimeClock clock_;
    midi::MidiScheduler scheduler_;
    std::vector<int64_t> event_delays_us_;  // Measured delays
};

} // namespace ai_arranger::realtime

#endif // AI_ARRANGER_TIMING_BENCHMARK_H

#include "engine/realtime/timing_benchmark.h"
#include <cstdio>
#include <string>

using namespace ai_arranger::realtime;

static int failures = 0;
static int passes = 0;

#define TEST(name, expr) do { \
    if (!(expr)) { \
        std::fprintf(stderr, "  ❌ FAIL: %s\n", name); \
        failures++; \
    } else { \
        std::printf("  ✅ PASS: %s\n", name); \
        passes++; \
    } \
} while(0)

int main() {
    std::printf("Test: TimingBenchmark\n");

    TimingBenchmark bench;
    BenchmarkConfig config;
    config.sample_rate = 48000;
    config.tempo_bpm = 120;
    config.buffer_size = 256;
    config.duration_seconds = 2;

    bench.configure(config);
    auto metrics = bench.run();

    TEST("Benchmark runs without error", metrics.total_events > 0);
    TEST("P50 is reasonable (< 1000 us)", metrics.p50_us < 1000.0);
    TEST("Min >= 0", metrics.min_us >= 0.0);
    TEST("Mean >= Min", metrics.mean_us >= metrics.min_us);
    TEST("Max >= Mean", metrics.max_us >= metrics.mean_us);
    TEST("P50 <= P90", metrics.p50_us <= metrics.p90_us);
    TEST("P90 <= P99", metrics.p90_us <= metrics.p99_us);

    // Test report generation
    auto report = TimingBenchmark::generateReport(metrics, config);
    TEST("Report generated", !report.empty());
    TEST("Report contains PASS/FAIL", report.find("PASS") != std::string::npos || report.find("FAIL") != std::string::npos);

    // Test at different buffer sizes
    BenchmarkConfig smallBuffer;
    smallBuffer.buffer_size = 64;
    smallBuffer.duration_seconds = 2;
    TimingBenchmark smallBench;
    smallBench.configure(smallBuffer);
    auto smallMetrics = smallBench.run();
    TEST("Small buffer works", smallMetrics.total_events > 0);

    BenchmarkConfig largeBuffer;
    largeBuffer.buffer_size = 512;
    largeBuffer.duration_seconds = 2;
    TimingBenchmark largeBench;
    largeBench.configure(largeBuffer);
    auto largeMetrics = largeBench.run();
    TEST("Large buffer works", largeMetrics.total_events > 0);

    std::printf("\nResults: %d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}

#include "engine/realtime/timing_benchmark.h"
#include <iostream>
#include <fstream>

int main() {
    using namespace ai_arranger::realtime;

    std::cout << "AI Arranger — Gate 1 Timing Spike\n";
    std::cout << "Running benchmark...\n\n";

    // Test multiple buffer sizes
    std::vector<BenchmarkConfig> configs = {
        {48000, 120, 64, 5},    // Small buffer (CoreAudio typical)
        {48000, 120, 128, 5},   // Medium buffer
        {48000, 120, 256, 5},   // Large buffer
        {48000, 140, 128, 5},   // Faster tempo
        {48000, 60, 128, 5},    // Slow tempo
    };

    for (const auto& config : configs) {
        TimingBenchmark bench;
        bench.configure(config);
        auto metrics = bench.run();
        std::cout << "Buffer=" << config.buffer_size
                  << " Tempo=" << config.tempo_bpm
                  << "bpm Duration=" << config.duration_seconds << "s\n";
        std::cout << TimingBenchmark::metricsToString(metrics) << "\n";
    }

    // Full report for default config
    BenchmarkConfig defaultConfig;
    TimingBenchmark bench;
    bench.configure(defaultConfig);
    auto metrics = bench.run();
    auto report = TimingBenchmark::generateReport(metrics, defaultConfig);
    std::cout << report;

    // Write report to file
    std::ofstream reportFile("docs/evidence/gate-1/timing-report.md");
    if (reportFile) {
        reportFile << "# GATE 1 Timing Benchmark Report\n\n";
        reportFile << "Generated: " << __DATE__ << " " << __TIME__ << "\n\n";
        reportFile << report;
        reportFile.close();
        std::cout << "Report written to docs/evidence/gate-1/timing-report.md\n";
    }

    return metrics.p99_us < 1000.0 ? 0 : 1;
}

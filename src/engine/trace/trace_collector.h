#ifndef AI_ARRANGER_TRACE_TRACE_COLLECTOR_H
#define AI_ARRANGER_TRACE_TRACE_COLLECTOR_H

#include "engine/trace/latency_trace.h"

// The collector is a NON-realtime consumer. Like the tracer it only exists when
// the build gate is on; production builds never see it.
#if defined(AIARR_LATENCY_TRACE)

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace ai_arranger::trace {

// Drains tracer() off the RT path and exports the captured records to CSV/JSON.
// Two usage modes:
//   - Background: start() spins a low-priority thread that periodically drains
//     the ring into an in-memory buffer; stop() does a final drain + join.
//   - Manual: call drainNow() yourself (e.g. from a test loop) — no thread.
// Neither mode ever runs on an RT thread, so std::vector growth / mutex / file-IO
// here are all fine; the RT side only ever touches the lock-free ring.
class TraceCollector {
public:
    explicit TraceCollector(uint32_t pollMicros = 1000) noexcept
        : poll_micros_(pollMicros) {}
    ~TraceCollector();

    TraceCollector(const TraceCollector&) = delete;
    TraceCollector& operator=(const TraceCollector&) = delete;

    void start();          // begin background draining
    void stop() noexcept;  // stop thread, final drain, join

    // Synchronous drain of everything currently in the ring into the buffer.
    // Returns the number of records moved this call.
    size_t drainNow();

    // Snapshot of everything collected so far (copy; safe to call any time).
    std::vector<TraceRecord> snapshot() const;
    size_t size() const noexcept;

    // Export helpers (non-RT). Return false on file-open failure.
    bool writeCsv(const std::string& path) const;
    bool writeJson(const std::string& path) const;

    // Free-function exporters for arbitrary record sets (used by tools/tests).
    static bool writeCsv(const std::string& path,
                         const std::vector<TraceRecord>& records);
    static bool writeJson(const std::string& path,
                          const std::vector<TraceRecord>& records,
                          const TraceCounters::Snapshot& counters);

private:
    void loop();

    mutable std::mutex       mutex_;
    std::vector<TraceRecord> buffer_;
    std::thread              thread_;
    std::atomic<bool>        running_{false};
    uint32_t                 poll_micros_;
};

} // namespace ai_arranger::trace

#endif // AIARR_LATENCY_TRACE

#endif // AI_ARRANGER_TRACE_TRACE_COLLECTOR_H

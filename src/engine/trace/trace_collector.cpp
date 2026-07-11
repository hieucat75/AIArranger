#include "engine/trace/trace_collector.h"

#if defined(AIARR_LATENCY_TRACE)

#include <chrono>
#include <cstdio>

namespace ai_arranger::trace {

TraceCollector::~TraceCollector() {
    stop();
}

void TraceCollector::start() {
    if (running_.exchange(true, std::memory_order_acq_rel)) return; // already running
    // Prime the trace clock + ring off the RT path and clear warm-up residue
    // BEFORE the first sample can arrive, so capture starts from a clean, warm
    // state. This is a non-RT startup path (start() spawns the drain thread); the
    // call is a no-op when the runtime gate is disabled.
    tracer().warmup();
    thread_ = std::thread([this] { loop(); });
}

void TraceCollector::stop() noexcept {
    if (!running_.exchange(false, std::memory_order_acq_rel)) {
        // Not running as a thread; still do a final synchronous drain so a
        // manual-mode collector captures the tail on shutdown.
        drainNow();
        return;
    }
    if (thread_.joinable()) thread_.join();
    drainNow(); // final drain after the thread has stopped
}

size_t TraceCollector::drainNow() {
    TraceRecord rec;
    size_t n = 0;
    std::lock_guard<std::mutex> lk(mutex_);
    while (tracer().drainOne(rec)) {
        buffer_.push_back(rec);
        ++n;
    }
    return n;
}

void TraceCollector::loop() {
    using namespace std::chrono;
    while (running_.load(std::memory_order_acquire)) {
        drainNow();
        std::this_thread::sleep_for(microseconds(poll_micros_));
    }
}

std::vector<TraceRecord> TraceCollector::snapshot() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return buffer_;
}

size_t TraceCollector::size() const noexcept {
    std::lock_guard<std::mutex> lk(mutex_);
    return buffer_.size();
}

bool TraceCollector::writeCsv(const std::string& path) const {
    return writeCsv(path, snapshot());
}

bool TraceCollector::writeJson(const std::string& path) const {
    return writeJson(path, snapshot(), tracer().counters().snapshot());
}

// ── Static exporters ─────────────────────────────────────────────────

bool TraceCollector::writeCsv(const std::string& path,
                              const std::vector<TraceRecord>& records) {
    std::FILE* f = std::fopen(path.c_str(), "w");
    if (!f) return false;
    std::fprintf(f,
        "timestamp_ns,correlation_id,stage,channel,note,velocity,"
        "chord_root,chord_type,section,tempo_bpm,input_device_id,"
        "output_device_id,lifecycle_tag\n");
    for (const auto& r : records) {
        std::fprintf(f,
            "%llu,%llu,%s,%d,%d,%d,%d,%d,%d,%u,%u,%u,%s\n",
            static_cast<unsigned long long>(r.timestamp_ns),
            static_cast<unsigned long long>(r.correlation_id),
            stageName(r.stage),
            static_cast<int>(r.channel), static_cast<int>(r.note),
            static_cast<int>(r.velocity), static_cast<int>(r.chord_root),
            static_cast<int>(r.chord_type), static_cast<int>(r.section),
            static_cast<unsigned>(r.tempo_bpm),
            static_cast<unsigned>(r.input_device_id),
            static_cast<unsigned>(r.output_device_id),
            lifecycleName(r.lifecycle_tag));
    }
    std::fclose(f);
    return true;
}

bool TraceCollector::writeJson(const std::string& path,
                               const std::vector<TraceRecord>& records,
                               const TraceCounters::Snapshot& c) {
    std::FILE* f = std::fopen(path.c_str(), "w");
    if (!f) return false;
    std::fprintf(f, "{\n  \"counters\": {\n");
    std::fprintf(f,
        "    \"input_callbacks\": %llu,\n"
        "    \"output_events\": %llu,\n"
        "    \"midi_sends\": %llu,\n"
        "    \"dropped_records\": %llu,\n"
        "    \"active_notes\": %lld,\n"
        "    \"panics\": %llu,\n"
        "    \"reconnects\": %llu,\n"
        "    \"queue_overflows\": %llu,\n"
        "    \"late_ticks\": %llu\n  },\n",
        static_cast<unsigned long long>(c.input_callbacks),
        static_cast<unsigned long long>(c.output_events),
        static_cast<unsigned long long>(c.midi_sends),
        static_cast<unsigned long long>(c.dropped_records),
        static_cast<long long>(c.active_notes),
        static_cast<unsigned long long>(c.panics),
        static_cast<unsigned long long>(c.reconnects),
        static_cast<unsigned long long>(c.queue_overflows),
        static_cast<unsigned long long>(c.late_ticks));
    std::fprintf(f, "  \"records\": [\n");
    for (size_t i = 0; i < records.size(); ++i) {
        const auto& r = records[i];
        std::fprintf(f,
            "    {\"timestamp_ns\": %llu, \"correlation_id\": %llu, "
            "\"stage\": \"%s\", \"channel\": %d, \"note\": %d, \"velocity\": %d, "
            "\"chord_root\": %d, \"chord_type\": %d, \"section\": %d, "
            "\"tempo_bpm\": %u, \"input_device_id\": %u, \"output_device_id\": %u, "
            "\"lifecycle_tag\": \"%s\"}%s\n",
            static_cast<unsigned long long>(r.timestamp_ns),
            static_cast<unsigned long long>(r.correlation_id),
            stageName(r.stage),
            static_cast<int>(r.channel), static_cast<int>(r.note),
            static_cast<int>(r.velocity), static_cast<int>(r.chord_root),
            static_cast<int>(r.chord_type), static_cast<int>(r.section),
            static_cast<unsigned>(r.tempo_bpm),
            static_cast<unsigned>(r.input_device_id),
            static_cast<unsigned>(r.output_device_id),
            lifecycleName(r.lifecycle_tag),
            (i + 1 < records.size()) ? "," : "");
    }
    std::fprintf(f, "  ]\n}\n");
    std::fclose(f);
    return true;
}

} // namespace ai_arranger::trace

#endif // AIARR_LATENCY_TRACE

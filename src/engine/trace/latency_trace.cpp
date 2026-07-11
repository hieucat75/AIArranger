#include "engine/trace/latency_trace.h"

// Everything below is compiled ONLY when the build gate is on. With the gate off
// this translation unit is intentionally empty — no ring, no singleton, no
// symbols — so a production build carries zero trace overhead and zero footprint.
#if defined(AIARR_LATENCY_TRACE)

#include "engine/trace/mpsc_ring.h"
#include "engine/trace/trace_clock.h"

namespace ai_arranger::trace {

// The ring lives in this single TU (out of the header) so its ~2 MB backing
// store is defined exactly once. Owned by the process-wide singleton.
struct LatencyTracer::Impl {
    MpscRing<TraceRecord, LatencyTracer::kCapacity> ring;
};

LatencyTracer::Impl& LatencyTracer::impl() noexcept {
    static Impl inst;
    return inst;
}

void LatencyTracer::emit(TraceStage stage, uint8_t channel, uint8_t note,
                         uint8_t velocity, uint8_t chordRoot, uint8_t chordType,
                         LifecycleTag tag) noexcept {
    if (!enabled_.load(std::memory_order_relaxed)) return;

    TraceRecord r;
    r.timestamp_ns     = traceNowNs();
    r.correlation_id   = correlation_.load(std::memory_order_relaxed);
    r.tempo_bpm        = tempo_bpm_.load(std::memory_order_relaxed);
    r.input_device_id  = input_device_.load(std::memory_order_relaxed);
    r.output_device_id = output_device_.load(std::memory_order_relaxed);
    r.stage            = static_cast<uint8_t>(stage);
    r.channel          = channel;
    r.note             = note;
    r.velocity         = velocity;
    r.chord_root       = chordRoot;
    r.chord_type       = chordType;
    r.section          = section_.load(std::memory_order_relaxed);
    r.lifecycle_tag    = static_cast<uint8_t>(tag);

    // Wait-free. A full ring never blocks the RT thread — drop + count.
    if (!impl().ring.try_push(r)) {
        counters_.dropped_records.fetch_add(1, std::memory_order_relaxed);
    }
}

uint64_t LatencyTracer::newCorrelation() noexcept {
    const uint64_t id = correlation_seq_.fetch_add(1, std::memory_order_relaxed) + 1;
    correlation_.store(id, std::memory_order_relaxed);
    return id;
}

uint64_t LatencyTracer::currentCorrelation() const noexcept {
    return correlation_.load(std::memory_order_relaxed);
}

bool LatencyTracer::markAccompFirst() noexcept {
    const uint64_t cur = correlation_.load(std::memory_order_relaxed);
    // cur == 0 means no chord has been confirmed yet — nothing to correlate to.
    if (cur == 0) return false;
    uint64_t prev = accomp_correlation_.load(std::memory_order_relaxed);
    if (cur == prev) return false; // already recorded the first event this chord
    // Single-writer in practice (the audio tick thread), but guard with a CAS so a
    // stray concurrent caller still yields exactly one "true".
    return accomp_correlation_.compare_exchange_strong(
        prev, cur, std::memory_order_relaxed);
}

void LatencyTracer::setTempoBpm(uint16_t bpm) noexcept {
    tempo_bpm_.store(bpm, std::memory_order_relaxed);
}
void LatencyTracer::setSection(uint8_t section) noexcept {
    section_.store(section, std::memory_order_relaxed);
}
void LatencyTracer::setInputDevice(uint16_t id) noexcept {
    input_device_.store(id, std::memory_order_relaxed);
}
void LatencyTracer::setOutputDevice(uint16_t id) noexcept {
    output_device_.store(id, std::memory_order_relaxed);
}

bool LatencyTracer::drainOne(TraceRecord& out) noexcept {
    return impl().ring.try_pop(out);
}

void LatencyTracer::reset() noexcept {
    // Drain any residue, then zero the counters + correlation state. Not RT-safe:
    // intended for tests / between measurement runs when no producer is live.
    TraceRecord scratch;
    while (impl().ring.try_pop(scratch)) { /* discard */ }
    counters_.input_callbacks.store(0, std::memory_order_relaxed);
    counters_.output_events.store(0, std::memory_order_relaxed);
    counters_.midi_sends.store(0, std::memory_order_relaxed);
    counters_.dropped_records.store(0, std::memory_order_relaxed);
    counters_.active_notes.store(0, std::memory_order_relaxed);
    counters_.panics.store(0, std::memory_order_relaxed);
    counters_.reconnects.store(0, std::memory_order_relaxed);
    counters_.queue_overflows.store(0, std::memory_order_relaxed);
    counters_.late_ticks.store(0, std::memory_order_relaxed);
    correlation_.store(0, std::memory_order_relaxed);
    correlation_seq_.store(0, std::memory_order_relaxed);
    accomp_correlation_.store(0, std::memory_order_relaxed);
}

LatencyTracer& tracer() noexcept {
    static LatencyTracer inst;
    return inst;
}

} // namespace ai_arranger::trace

#endif // AIARR_LATENCY_TRACE

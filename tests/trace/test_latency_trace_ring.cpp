// Gate 3 latency harness unit test.
//
// The MPSC ring is a generic container compiled in BOTH build modes, so its
// fill / drain / overflow-counter behaviour is always exercised. The tracer and
// its counters only exist when the build gate is on, so those assertions are
// guarded by AIARR_TRACE_ENABLED — the suite passes either way.

#include "engine/trace/mpsc_ring.h"
#include "engine/trace/trace_record.h"
#include "engine/trace/latency_trace.h"

#include <atomic>
#include <cstdio>
#include <thread>
#include <vector>

using namespace ai_arranger::trace;

static int failures = 0;
static int passes = 0;

#define TEST(name, expr) do {                         \
    if (!(expr)) {                                    \
        std::fprintf(stderr, "  \xE2\x9D\x8C FAIL: %s\n", name); \
        failures++;                                   \
    } else {                                          \
        std::printf("  \xE2\x9C\x85 PASS: %s\n", name);         \
        passes++;                                     \
    }                                                 \
} while (0)

static TraceRecord makeRec(uint64_t id) {
    TraceRecord r{};
    r.timestamp_ns = id * 10;
    r.correlation_id = id;
    r.stage = static_cast<uint8_t>(TraceStage::MidiSend);
    r.note = static_cast<uint8_t>(id & 0x7F);
    return r;
}

static void testRingFillDrain() {
    std::printf("Ring: fill / drain / FIFO order\n");
    MpscRing<TraceRecord, 8> ring; // holds 8 records

    for (uint64_t i = 0; i < 8; ++i) {
        TEST("push within capacity succeeds", ring.try_push(makeRec(i)));
    }
    // 9th push must fail (ring full) — never blocks.
    TEST("push on full ring returns false", !ring.try_push(makeRec(999)));

    // Drain and confirm FIFO order.
    bool orderOk = true;
    for (uint64_t i = 0; i < 8; ++i) {
        TraceRecord out{};
        if (!ring.try_pop(out) || out.correlation_id != i) orderOk = false;
    }
    TEST("drain preserves FIFO order", orderOk);

    TraceRecord out{};
    TEST("pop on empty ring returns false", !ring.try_pop(out));

    // Ring is reusable after a full drain.
    TEST("reuse after drain", ring.try_push(makeRec(42)));
    TEST("pop reused record", ring.try_pop(out) && out.correlation_id == 42);
}

static void testRingOverflowCounter() {
    std::printf("Ring: overflow drop counting\n");
    MpscRing<TraceRecord, 4> ring;
    size_t accepted = 0, dropped = 0;
    for (uint64_t i = 0; i < 10; ++i) {
        if (ring.try_push(makeRec(i))) ++accepted; else ++dropped;
    }
    TEST("accepted == capacity", accepted == 4);
    TEST("dropped == overflow count", dropped == 6);
}

static void testRingConcurrentProducers() {
    std::printf("Ring: concurrent multi-producer safety\n");
    // 4 producer threads hammer a large ring; the single consumer drains. No
    // record is duplicated or corrupted; total drained == total pushed.
    constexpr size_t kCap = 1u << 14;
    MpscRing<TraceRecord, kCap> ring;
    constexpr int kThreads = 4;
    constexpr int kPerThread = 2000;
    std::atomic<int> pushed{0};

    std::vector<std::thread> producers;
    for (int t = 0; t < kThreads; ++t) {
        producers.emplace_back([&ring, &pushed, t] {
            for (int i = 0; i < kPerThread; ++i) {
                TraceRecord r = makeRec(static_cast<uint64_t>(t) * 100000 + i);
                if (ring.try_push(r)) pushed.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    int drained = 0;
    std::atomic<bool> running{true};
    std::thread consumer([&] {
        TraceRecord out{};
        while (running.load(std::memory_order_acquire)) {
            while (ring.try_pop(out)) ++drained;
            std::this_thread::yield();
        }
        while (ring.try_pop(out)) ++drained;
    });
    for (auto& p : producers) p.join();
    running.store(false, std::memory_order_release);
    consumer.join();

    TEST("drained equals pushed (no loss, no dup)", drained == pushed.load());
}

#if AIARR_TRACE_ENABLED
static void testTracerEmitDrain() {
    std::printf("Tracer: emit / drain / correlation / accomp-first\n");
    auto& tr = tracer();
    tr.reset();
    tr.setEnabled(true);

    // A chord confirm opens a correlation; accomp-first fires once per chord.
    const uint64_t c1 = tr.newCorrelation();
    TEST("newCorrelation increments", c1 == 1);
    TEST("markAccompFirst true once", tr.markAccompFirst());
    TEST("markAccompFirst false again", !tr.markAccompFirst());
    const uint64_t c2 = tr.newCorrelation();
    TEST("second correlation", c2 == 2);
    TEST("accomp-first re-arms per chord", tr.markAccompFirst());

    tr.reset();
    // Emit a small journey and drain it back.
    tr.emit(TraceStage::InputNoteOn, 0, 60, 100, kNa, kNa, LifecycleTag::kNone);
    tr.emit(TraceStage::MidiSend, 0, 60, 100, kNa, kNa, LifecycleTag::kNone);
    int drained = 0;
    TraceRecord out{};
    while (tr.drainOne(out)) ++drained;
    TEST("tracer emits are drainable", drained == 2);

    // Runtime gate: disabled emits are no-ops.
    tr.reset();
    tr.setEnabled(false);
    tr.emit(TraceStage::MidiSend, 0, 60, 100, kNa, kNa, LifecycleTag::kNone);
    TEST("runtime-disabled emit drops", !tr.drainOne(out));
    tr.setEnabled(true);

    // Counters.
    tr.reset();
    tr.counters().input_callbacks.fetch_add(3, std::memory_order_relaxed);
    tr.counters().panics.fetch_add(1, std::memory_order_relaxed);
    auto snap = tr.counters().snapshot();
    TEST("counter snapshot reads back", snap.input_callbacks == 3 && snap.panics == 1);
    tr.reset();
}

static void testTracerWarmup() {
    std::printf("Tracer: warmup primes clock/ring then resets to zero\n");
    auto& tr = tracer();
    tr.reset();
    tr.setEnabled(true);

    // Dirty some state so we can prove warmup() clears it: a bumped counter, a
    // live correlation, an un-drained record, and a nonzero live-note gauge.
    tr.counters().input_callbacks.fetch_add(7, std::memory_order_relaxed);
    tr.counters().active_notes.fetch_add(3, std::memory_order_relaxed);
    tr.newCorrelation();
    tr.emit(TraceStage::MidiSend, 0, 60, 100, kNa, kNa, LifecycleTag::kNone);

    tr.warmup();

    auto snap = tr.counters().snapshot();
    TEST("warmup zeroes session counters",
         snap.input_callbacks == 0 && snap.midi_sends == 0 &&
         snap.dropped_records == 0);
    // The live active_notes gauge tracks physically sounding notes and must
    // survive a capture-start warm-up (else a held note's later NoteOff would
    // drive it negative → false stuck-note FAIL).
    TEST("warmup preserves the live active_notes gauge", snap.active_notes == 3);
    TEST("warmup resets correlation", tr.currentCorrelation() == 0);
    TraceRecord out{};
    TEST("warmup leaves no residual records in the ring", !tr.drainOne(out));

    // Runtime-disabled warmup is a pure no-op (zero-overhead contract): it must
    // not emit or otherwise touch the ring.
    tr.reset();
    tr.setEnabled(false);
    tr.warmup();
    TEST("warmup while disabled is a no-op", !tr.drainOne(out));
    tr.setEnabled(true);
    tr.reset();
}

static void testTracerDroppedCounter() {
    std::printf("Tracer: ring-full increments dropped_records\n");
    auto& tr = tracer();
    tr.reset();
    tr.setEnabled(true);
    // Flood well past the ring capacity WITHOUT draining, forcing drops.
    const size_t flood = LatencyTracer::kCapacity + 5000;
    for (size_t i = 0; i < flood; ++i) {
        tr.emit(TraceStage::MidiSend, 0, 60, 100, kNa, kNa, LifecycleTag::kNone);
    }
    auto snap = tr.counters().snapshot();
    TEST("dropped_records counts overflow (no block)", snap.dropped_records > 0);
    tr.reset();
}
#endif // AIARR_TRACE_ENABLED

int main() {
    std::printf("Test: Latency trace harness (AIARR_TRACE_ENABLED=%d)\n",
                AIARR_TRACE_ENABLED);
    std::printf("TraceRecord size = %zu bytes\n", sizeof(TraceRecord));

    testRingFillDrain();
    testRingOverflowCounter();
    testRingConcurrentProducers();
#if AIARR_TRACE_ENABLED
    testTracerEmitDrain();
    testTracerWarmup();
    testTracerDroppedCounter();
#else
    std::printf("(tracer assertions skipped - build gate off)\n");
#endif

    std::printf("\n%d passed, %d failed\n", passes, failures);
    return failures ? 1 : 0;
}

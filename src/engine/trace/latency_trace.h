#ifndef AI_ARRANGER_TRACE_LATENCY_TRACE_H
#define AI_ARRANGER_TRACE_LATENCY_TRACE_H

#include "engine/trace/trace_record.h"

#include <atomic>
#include <cstddef>
#include <cstdint>

// ── Gate 3 latency trace facility ────────────────────────────────────
//
// Central, process-wide latency tracer. Two gates control it:
//
//   1. BUILD gate — AIARR_LATENCY_TRACE. When undefined (the production
//      default), every AIARR_TRACE_* macro expands to `((void)0)`, the
//      LatencyTracer type is not referenced, no ring is allocated, and the whole
//      facility compiles out to nothing (true zero overhead).
//   2. RUNTIME gate — when compiled in, tracer().setEnabled(false) makes emit a
//      single relaxed atomic load + early return, so a debug build can toggle
//      tracing without a rebuild. Default is enabled once compiled in.
//
// RT-safety: emit() only reads a few atomics, builds a 30-byte POD, and does one
// wait-free MpscRing::try_push. No malloc / lock / syscall / string / file-IO on
// any producer thread. A full ring bumps dropped_records and returns — the audio,
// CoreMIDI-read, and dispatch threads never block.

namespace ai_arranger::trace {

#if defined(AIARR_LATENCY_TRACE)

// Gate-3 counters. All atomic, written on RT paths (relaxed), read off-RT.
struct TraceCounters {
    std::atomic<uint64_t> input_callbacks{0}; // input NoteOns observed
    std::atomic<uint64_t> output_events{0};   // events enqueued to MIDI output
    std::atomic<uint64_t> midi_sends{0};       // MIDISend actually invoked
    std::atomic<uint64_t> dropped_records{0};  // trace records dropped (ring full)
    std::atomic<int64_t>  active_notes{0};     // gauge: NoteOn(+1) / NoteOff(-1)
    std::atomic<uint64_t> panics{0};
    std::atomic<uint64_t> reconnects{0};       // disconnect/reconnect events
    std::atomic<uint64_t> queue_overflows{0};  // output/scheduler queue rejects
    std::atomic<uint64_t> late_ticks{0};       // late-tick / catch-up events

    struct Snapshot {
        uint64_t input_callbacks, output_events, midi_sends, dropped_records;
        int64_t  active_notes;
        uint64_t panics, reconnects, queue_overflows, late_ticks;
    };
    Snapshot snapshot() const noexcept {
        return {input_callbacks.load(std::memory_order_relaxed),
                output_events.load(std::memory_order_relaxed),
                midi_sends.load(std::memory_order_relaxed),
                dropped_records.load(std::memory_order_relaxed),
                active_notes.load(std::memory_order_relaxed),
                panics.load(std::memory_order_relaxed),
                reconnects.load(std::memory_order_relaxed),
                queue_overflows.load(std::memory_order_relaxed),
                late_ticks.load(std::memory_order_relaxed)};
    }
};

class LatencyTracer {
public:
    // Ring capacity: 30-byte records → 2^16 slots ≈ 2 MB. Sized so a 60-minute
    // soak drains comfortably at the collector's default cadence without ever
    // filling. Overflow is still handled (drop + counter), never a block.
    static constexpr size_t kCapacity = 1u << 16;

    // ── Producer (RT-safe) ───────────────────────────────────────────
    void emit(TraceStage stage, uint8_t channel, uint8_t note, uint8_t velocity,
              uint8_t chordRoot, uint8_t chordType, LifecycleTag tag) noexcept;

    // Stamp + return a fresh correlation id (called at chord-confirm). RT-safe.
    uint64_t newCorrelation() noexcept;
    uint64_t currentCorrelation() const noexcept;

    // Returns true exactly once per correlation id — the FIRST accompaniment
    // event after a chord confirm. Lets the dispatch loop call the trace macro on
    // every event while only the first-per-chord is recorded. RT-safe.
    bool markAccompFirst() noexcept;

    // ── Cheap RT context setters (relaxed atomics) ───────────────────
    void setTempoBpm(uint16_t bpm) noexcept;
    void setSection(uint8_t section) noexcept;
    void setInputDevice(uint16_t id) noexcept;
    void setOutputDevice(uint16_t id) noexcept;

    // ── Counters ─────────────────────────────────────────────────────
    TraceCounters& counters() noexcept { return counters_; }
    const TraceCounters& counters() const noexcept { return counters_; }

    // ── Runtime gate ─────────────────────────────────────────────────
    void setEnabled(bool on) noexcept { enabled_.store(on, std::memory_order_relaxed); }
    bool enabled() const noexcept { return enabled_.load(std::memory_order_relaxed); }

    // ── Consumer (single non-RT collector) ───────────────────────────
    bool drainOne(TraceRecord& out) noexcept;

    // Reset ring + counters (tests / between measurement runs). NOT RT-safe;
    // call only when no producer is active. When preserveActiveNotes is true the
    // live active_notes gauge is left untouched — it mirrors physically sounding
    // notes (engine NoteOn/NoteOff), not a per-capture count, so clearing it while
    // notes are held would desync it and manufacture a false stuck-note reading.
    void reset(bool preserveActiveNotes = false) noexcept;

    // Prime the trace facility from a NON-RT startup path, before capture begins,
    // so the first real sample never pays a one-time cost on the RT thread. It
    // warms the trace clock's mach_timebase magic-static and forces the ~2 MB
    // ring's lazy construction + first-touch, then clears warm-up residue via
    // reset(preserveActiveNotes=true) — session counters + ring are zeroed but the
    // live active_notes gauge is preserved. A no-op when the runtime gate is off,
    // so a disabled build keeps its zero-overhead startup. NOT RT-safe.
    void warmup() noexcept;

private:
    // Defined in latency_trace.cpp so the large ring lives in one TU.
    struct Impl;
    Impl& impl() noexcept;

    TraceCounters         counters_;
    std::atomic<bool>     enabled_{true};
    std::atomic<uint64_t> correlation_{0};
    std::atomic<uint64_t> correlation_seq_{0};
    std::atomic<uint64_t> accomp_correlation_{0}; // last correlation marked "accomp-first"
    std::atomic<uint16_t> tempo_bpm_{0};
    std::atomic<uint16_t> input_device_{0};
    std::atomic<uint16_t> output_device_{0};
    std::atomic<uint8_t>  section_{kNa};
};

// Process-wide singleton (function-local static → constructed on first use only
// when the build gate is on).
LatencyTracer& tracer() noexcept;

// ── Call-site macros (build gate ON) ─────────────────────────────────
#define AIARR_TRACE_ENABLED 1

#define AIARR_TRACE_INPUT_NOTEON(ch, note, vel)                                   \
    do {                                                                          \
        ::ai_arranger::trace::tracer().counters().input_callbacks.fetch_add(      \
            1, std::memory_order_relaxed);                                        \
        ::ai_arranger::trace::tracer().emit(                                      \
            ::ai_arranger::trace::TraceStage::InputNoteOn, (ch), (note), (vel),   \
            ::ai_arranger::trace::kNa, ::ai_arranger::trace::kNa,                 \
            ::ai_arranger::trace::LifecycleTag::kNone);                           \
    } while (0)

#define AIARR_TRACE_CHORD_CONFIRMED(root, type)                                   \
    do {                                                                          \
        ::ai_arranger::trace::tracer().newCorrelation();                          \
        ::ai_arranger::trace::tracer().emit(                                      \
            ::ai_arranger::trace::TraceStage::ChordConfirmed,                     \
            ::ai_arranger::trace::kNa, ::ai_arranger::trace::kNa,                 \
            ::ai_arranger::trace::kNa, (root), (type),                            \
            ::ai_arranger::trace::LifecycleTag::kChordChange);                    \
    } while (0)

#define AIARR_TRACE_ACCOMP_FIRST(ch, note, vel)                                   \
    do {                                                                          \
        if (::ai_arranger::trace::tracer().markAccompFirst())                     \
            ::ai_arranger::trace::tracer().emit(                                  \
                ::ai_arranger::trace::TraceStage::AccompFirstEvent, (ch), (note), \
                (vel), ::ai_arranger::trace::kNa, ::ai_arranger::trace::kNa,      \
                ::ai_arranger::trace::LifecycleTag::kNone);                       \
    } while (0)

#define AIARR_TRACE_OUTPUT_ENQUEUE(ch, note, vel, ok)                             \
    do {                                                                          \
        ::ai_arranger::trace::tracer().counters().output_events.fetch_add(        \
            1, std::memory_order_relaxed);                                        \
        if (!(ok))                                                                \
            ::ai_arranger::trace::tracer().counters().queue_overflows.fetch_add(  \
                1, std::memory_order_relaxed);                                    \
        ::ai_arranger::trace::tracer().emit(                                      \
            ::ai_arranger::trace::TraceStage::OutputEnqueue, (ch), (note), (vel), \
            ::ai_arranger::trace::kNa, ::ai_arranger::trace::kNa,                 \
            (ok) ? ::ai_arranger::trace::LifecycleTag::kNone                      \
                 : ::ai_arranger::trace::LifecycleTag::kQueueOverflow);           \
    } while (0)

#define AIARR_TRACE_MIDI_SEND(ch, note, vel)                                      \
    do {                                                                          \
        ::ai_arranger::trace::tracer().counters().midi_sends.fetch_add(           \
            1, std::memory_order_relaxed);                                        \
        ::ai_arranger::trace::tracer().emit(                                      \
            ::ai_arranger::trace::TraceStage::MidiSend, (ch), (note), (vel),      \
            ::ai_arranger::trace::kNa, ::ai_arranger::trace::kNa,                 \
            ::ai_arranger::trace::LifecycleTag::kNone);                           \
    } while (0)

#define AIARR_TRACE_SET_TEMPO(bpm)      ::ai_arranger::trace::tracer().setTempoBpm((uint16_t)(bpm))
#define AIARR_TRACE_SET_SECTION(idx)    ::ai_arranger::trace::tracer().setSection((uint8_t)(idx))
#define AIARR_TRACE_SET_INPUT_DEVICE(id)  ::ai_arranger::trace::tracer().setInputDevice((uint16_t)(id))
#define AIARR_TRACE_SET_OUTPUT_DEVICE(id) ::ai_arranger::trace::tracer().setOutputDevice((uint16_t)(id))

#define AIARR_TRACE_NOTE_ON()  ::ai_arranger::trace::tracer().counters().active_notes.fetch_add(1, std::memory_order_relaxed)
#define AIARR_TRACE_NOTE_OFF() ::ai_arranger::trace::tracer().counters().active_notes.fetch_sub(1, std::memory_order_relaxed)
#define AIARR_TRACE_COUNT_PANIC()     ::ai_arranger::trace::tracer().counters().panics.fetch_add(1, std::memory_order_relaxed)
#define AIARR_TRACE_COUNT_RECONNECT() ::ai_arranger::trace::tracer().counters().reconnects.fetch_add(1, std::memory_order_relaxed)
#define AIARR_TRACE_COUNT_LATE_TICK() ::ai_arranger::trace::tracer().counters().late_ticks.fetch_add(1, std::memory_order_relaxed)

#define AIARR_TRACE_LIFECYCLE(tag)                                                \
    ::ai_arranger::trace::tracer().emit(                                          \
        ::ai_arranger::trace::TraceStage::Lifecycle, ::ai_arranger::trace::kNa,   \
        ::ai_arranger::trace::kNa, ::ai_arranger::trace::kNa,                     \
        ::ai_arranger::trace::kNa, ::ai_arranger::trace::kNa, (tag))

#else // ── Build gate OFF: compile every trace point out to nothing ────

#define AIARR_TRACE_ENABLED 0
#define AIARR_TRACE_INPUT_NOTEON(ch, note, vel)          ((void)0)
#define AIARR_TRACE_CHORD_CONFIRMED(root, type)          ((void)0)
#define AIARR_TRACE_ACCOMP_FIRST(ch, note, vel)          ((void)0)
#define AIARR_TRACE_OUTPUT_ENQUEUE(ch, note, vel, ok)    ((void)0)
#define AIARR_TRACE_MIDI_SEND(ch, note, vel)             ((void)0)
#define AIARR_TRACE_SET_TEMPO(bpm)                       ((void)0)
#define AIARR_TRACE_SET_SECTION(idx)                     ((void)0)
#define AIARR_TRACE_SET_INPUT_DEVICE(id)                 ((void)0)
#define AIARR_TRACE_SET_OUTPUT_DEVICE(id)                ((void)0)
#define AIARR_TRACE_NOTE_ON()                            ((void)0)
#define AIARR_TRACE_NOTE_OFF()                           ((void)0)
#define AIARR_TRACE_COUNT_PANIC()                        ((void)0)
#define AIARR_TRACE_COUNT_RECONNECT()                    ((void)0)
#define AIARR_TRACE_COUNT_LATE_TICK()                    ((void)0)
#define AIARR_TRACE_LIFECYCLE(tag)                       ((void)0)

#endif // AIARR_LATENCY_TRACE

} // namespace ai_arranger::trace

#endif // AI_ARRANGER_TRACE_LATENCY_TRACE_H

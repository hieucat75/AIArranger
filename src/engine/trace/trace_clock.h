#ifndef AI_ARRANGER_TRACE_TRACE_CLOCK_H
#define AI_ARRANGER_TRACE_TRACE_CLOCK_H

#include <cstdint>

// ── Monotonic trace clock ────────────────────────────────────────────
//
// A single, realtime-safe, monotonic timestamp source shared by every latency
// trace point. "Monotonic raw" semantics: never steps backwards, never adjusted
// by NTP/settimeofday, and readable from any thread (CoreMIDI read thread, audio
// tick thread, dispatch thread) with no lock, no malloc, no syscall on the hot
// path.
//
//   - Apple  : mach_absolute_time() (userspace, vDSO-fast) scaled to ns via the
//              once-cached mach_timebase_info.
//   - POSIX  : clock_gettime(CLOCK_MONOTONIC_RAW) — the RAW variant is immune to
//              adjtime slewing, matching the "raw" contract above.
//
// The absolute epoch is arbitrary; only *differences* between two readings are
// meaningful, which is all the latency metrics need. This is deliberately kept
// separate from the musical/tempo clocks (engine/realtime/clock.h,
// performance/clock/*) so the vendor-agnostic core can timestamp traces without
// pulling in any Apple framework.

#if defined(__APPLE__)
#include <mach/mach_time.h>
#else
#include <ctime>
#endif

namespace ai_arranger::trace {

#if defined(__APPLE__)
inline uint64_t traceNowNs() noexcept {
    static const mach_timebase_info_data_t tb = [] {
        mach_timebase_info_data_t t{};
        mach_timebase_info(&t);
        return t;
    }();
    const uint64_t ticks = mach_absolute_time();
    // ticks * numer / denom → nanoseconds. numer/denom is 1/1 on Apple Silicon
    // and small integers elsewhere; the intermediate stays well within uint64
    // for any realistic session (centuries before overflow).
    return ticks * tb.numer / tb.denom;
}
#else
inline uint64_t traceNowNs() noexcept {
    struct timespec ts{};
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return static_cast<uint64_t>(ts.tv_sec) * 1000000000ULL +
           static_cast<uint64_t>(ts.tv_nsec);
}
#endif

} // namespace ai_arranger::trace

#endif // AI_ARRANGER_TRACE_TRACE_CLOCK_H

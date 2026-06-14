#include "performance/clock/coreaudio_clock.h"
#include <mach/mach_time.h>

namespace ai_arranger::performance {

namespace {
// Cached mach timebase (host ticks -> nanoseconds).
mach_timebase_info_data_t& timebase() {
    static mach_timebase_info_data_t tb = [] {
        mach_timebase_info_data_t t{};
        mach_timebase_info(&t);
        if (t.denom == 0) { t.numer = 1; t.denom = 1; }
        return t;
    }();
    return tb;
}
} // namespace

void CoreAudioClock::start() noexcept {
    last_host_.store(mach_absolute_time(), std::memory_order_release);
    running_.store(true, std::memory_order_release);
}

uint64_t CoreAudioClock::pollElapsedSamples() noexcept {
    if (!running_.load(std::memory_order_acquire)) return 0;
    const uint64_t now = mach_absolute_time();
    const uint64_t prev = last_host_.exchange(now, std::memory_order_acq_rel);
    if (now <= prev) return 0;
    const auto& tb = timebase();
    // host ticks -> nanoseconds -> samples (true elapsed; no fixed-step drift).
    const long double ns =
        static_cast<long double>(now - prev) * tb.numer / tb.denom;
    const long double samples = ns * sample_rate_ / 1'000'000'000.0L;
    return static_cast<uint64_t>(samples);
}

} // namespace ai_arranger::performance

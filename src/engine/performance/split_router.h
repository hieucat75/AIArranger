#ifndef AI_ARRANGER_PERFORMANCE_SPLIT_ROUTER_H
#define AI_ARRANGER_PERFORMANCE_SPLIT_ROUTER_H

#include <atomic>
#include <cstddef>
#include <cstdint>

// ── Split / lower / manual bass router (Gate 12 Task F) ──────────────
//
// Partitions the keyboard at a split point: notes below the split feed chord
// detection (lower zone); notes at/above feed melody (upper zone). Manual-bass
// mode lets the lowest held lower-zone note override the NTT-derived bass. Pure
// classification only — assigns NO MIDI channel (no channel collision); the
// caller maps zones to roles. Lock-free, no audio.

namespace ai_arranger::performance {

enum class Zone : uint8_t { Lower, Upper };

inline constexpr uint8_t kNoManualBass = 0xFF;

class SplitRouter {
public:
    void setSplitPoint(uint8_t note) noexcept {
        split_.store(note, std::memory_order_release);
    }
    uint8_t splitPoint() const noexcept {
        return split_.load(std::memory_order_acquire);
    }

    void setManualBass(bool on) noexcept {
        manual_bass_.store(on, std::memory_order_release);
    }
    bool manualBass() const noexcept {
        return manual_bass_.load(std::memory_order_acquire);
    }

    // Note below the split point -> Lower; the split note itself and above -> Upper.
    Zone zoneOf(uint8_t note) const noexcept {
        return note < splitPoint() ? Zone::Lower : Zone::Upper;
    }

    // Lowest held lower-zone note when manual bass is on, else kNoManualBass.
    // `lower` is ascending; entries must already be lower-zone.
    uint8_t manualBassNote(const uint8_t* lower, size_t count) const noexcept {
        if (!manualBass() || count == 0) return kNoManualBass;
        return lower[0];
    }

private:
    std::atomic<uint8_t> split_{60};         // C4 default
    std::atomic<bool>    manual_bass_{false};
};

} // namespace ai_arranger::performance

#endif // AI_ARRANGER_PERFORMANCE_SPLIT_ROUTER_H

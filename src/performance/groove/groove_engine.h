#ifndef AI_ARRANGER_PERFORMANCE_GROOVE_GROOVE_ENGINE_H
#define AI_ARRANGER_PERFORMANCE_GROOVE_GROOVE_ENGINE_H

#include "performance/groove/groove_profile.h"
#include "performance/groove/deterministic_rng.h"
#include "engine/uasf/types.h"
#include <cstdint>

// ── Groove engine (Gate 15 Task C) ───────────────────────────────────
//
// Applies a groove profile to note events at the dispatch seam: micro-timing
// offset + velocity humanization. Deterministic — reset() reseeds, and the same
// event sequence then produces a byte-for-byte identical output stream. Non-note
// events pass through unchanged. Realtime-safe (stack-only, no alloc/lock).

namespace ai_arranger::performance {

class GrooveEngine {
public:
    GrooveEngine(uint64_t seed, uint32_t ticksPerBeat) noexcept
        : seed_(seed), tpb_(ticksPerBeat ? ticksPerBeat : 480), rng_(seed) {}

    void setProfile(const IGrooveProfile* p) noexcept { profile_ = p; }
    const IGrooveProfile* profile() const noexcept { return profile_; }

    // Reseed for a fresh, reproducible run.
    void reset() noexcept { rng_.reseed(seed_); }

    // Returns the humanized event. NoteOff keeps velocity 0; non-note unchanged.
    uasf::MidiEvent apply(const uasf::MidiEvent& in) noexcept {
        if (!profile_) return in;
        const bool isNote = in.type == uasf::MidiEventType::NoteOn ||
                            in.type == uasf::MidiEventType::NoteOff;
        if (!isNote) return in;

        const uint32_t phase = static_cast<uint32_t>(in.tick % tpb_);
        const GrooveAdjust a = profile_->adjust(phase, tpb_, in.data2, rng_);

        uasf::MidiEvent out = in;
        int64_t t = static_cast<int64_t>(in.tick) + a.tick_offset;
        if (t < 0) t = 0;
        out.tick = static_cast<uint64_t>(t);

        if (in.type == uasf::MidiEventType::NoteOn && in.data2 > 0) {
            int v = static_cast<int>(in.data2) + a.vel_adjust;
            if (v < 1) v = 1; if (v > 127) v = 127;
            out.data2 = static_cast<uint8_t>(v);
        }
        return out;
    }

private:
    const IGrooveProfile* profile_ = nullptr;
    uint64_t seed_;
    uint32_t tpb_;
    DeterministicRng rng_;
};

} // namespace ai_arranger::performance

#endif // AI_ARRANGER_PERFORMANCE_GROOVE_GROOVE_ENGINE_H

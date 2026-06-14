#include "performance/groove/groove_profile.h"

namespace ai_arranger::performance {

namespace {

bool isOffbeat(uint32_t phase, uint32_t tpb) noexcept {
    // near the &-of-the-beat (half-way through), within an 1/8 window.
    const uint32_t eighth = tpb / 2;
    const uint32_t win = tpb / 8 + 1;
    return phase + win >= eighth && phase <= eighth + win;
}

struct Tight final : IGrooveProfile {
    GrooveAdjust adjust(uint32_t, uint32_t, uint8_t, DeterministicRng& rng) const noexcept override {
        // machine-tight: ±1 tick, ±1 vel — barely-there life.
        return { rng.range(-1, 1), rng.range(-1, 1) };
    }
    const char* name() const noexcept override { return "tight"; }
};

struct LaidBack final : IGrooveProfile {
    GrooveAdjust adjust(uint32_t phase, uint32_t tpb, uint8_t, DeterministicRng& rng) const noexcept override {
        // behind the beat: off-beats pushed late a few ticks, slightly softer.
        const int late = isOffbeat(phase, tpb) ? rng.range(4, 9) : rng.range(0, 3);
        return { late, rng.range(-4, 0) };
    }
    const char* name() const noexcept override { return "laid-back"; }
};

struct SwingLight final : IGrooveProfile {
    GrooveAdjust adjust(uint32_t phase, uint32_t tpb, uint8_t, DeterministicRng& rng) const noexcept override {
        // light swing: delay off-beat 8ths toward a triplet; on-beats steady.
        const int swing = isOffbeat(phase, tpb) ? (int)(tpb / 12) + rng.range(0, 2) : 0;
        return { swing, rng.range(-2, 2) };
    }
    const char* name() const noexcept override { return "swing-light"; }
};

struct LivePop final : IGrooveProfile {
    GrooveAdjust adjust(uint32_t, uint32_t, uint8_t, DeterministicRng& rng) const noexcept override {
        // human pop band: small symmetric timing + velocity humanization.
        return { rng.range(-3, 4), rng.range(-6, 6) };
    }
    const char* name() const noexcept override { return "live-pop"; }
};

struct AcousticSoft final : IGrooveProfile {
    GrooveAdjust adjust(uint32_t phase, uint32_t tpb, uint8_t, DeterministicRng& rng) const noexcept override {
        // gentle, softer dynamics + slight breath on off-beats.
        const int breath = isOffbeat(phase, tpb) ? rng.range(2, 6) : rng.range(-2, 2);
        return { breath, rng.range(-10, -2) };
    }
    const char* name() const noexcept override { return "acoustic-soft"; }
};

const Tight        kTight{};
const LaidBack     kLaidBack{};
const SwingLight   kSwingLight{};
const LivePop      kLivePop{};
const AcousticSoft kAcousticSoft{};

} // namespace

const IGrooveProfile& grooveTight() noexcept        { return kTight; }
const IGrooveProfile& grooveLaidBack() noexcept     { return kLaidBack; }
const IGrooveProfile& grooveSwingLight() noexcept   { return kSwingLight; }
const IGrooveProfile& grooveLivePop() noexcept      { return kLivePop; }
const IGrooveProfile& grooveAcousticSoft() noexcept { return kAcousticSoft; }

const IGrooveProfile& grooveByIndex(int index) noexcept {
    switch (index) {
        case 1: return kLaidBack;
        case 2: return kSwingLight;
        case 3: return kLivePop;
        case 4: return kAcousticSoft;
        default: return kTight;
    }
}

} // namespace ai_arranger::performance

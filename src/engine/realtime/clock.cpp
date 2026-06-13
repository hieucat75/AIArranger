#include "engine/realtime/clock.h"
#include <cmath>

namespace ai_arranger::realtime {

RealtimeClock::RealtimeClock() {
    recalcDerived();
}

void RealtimeClock::setSampleRate(uint32_t sampleRate) noexcept {
    sample_rate_.store(sampleRate, std::memory_order_relaxed);
    recalcDerived();
}

void RealtimeClock::setTempo(uint32_t bpm) noexcept {
    tempo_bpm_.store(bpm, std::memory_order_relaxed);
    recalcDerived();
}

void RealtimeClock::setResolution(uint32_t ticksPerQuarter) noexcept {
    resolution_.store(ticksPerQuarter, std::memory_order_relaxed);
    recalcDerived();
}

void RealtimeClock::setTimeSignature(uint32_t beatsPerBar, uint32_t beatNote) noexcept {
    beats_per_bar_.store(beatsPerBar, std::memory_order_relaxed);
    beat_note_.store(beatNote, std::memory_order_relaxed);
}

void RealtimeClock::reset() noexcept {
    sample_position_.store(0, std::memory_order_release);
}

void RealtimeClock::advance(uint32_t numSamples) noexcept {
    if (!running_.load(std::memory_order_acquire)) return;
    sample_position_.fetch_add(numSamples, std::memory_order_acq_rel);
}

void RealtimeClock::stop() noexcept {
    running_.store(false, std::memory_order_release);
}

void RealtimeClock::start() noexcept {
    running_.store(true, std::memory_order_release);
}

int64_t RealtimeClock::getPosition() const noexcept {
    const int64_t samples = sample_position_.load(std::memory_order_acquire);
    const double tps = ticks_per_sample_.load(std::memory_order_acquire);
    return static_cast<int64_t>(static_cast<double>(samples) * tps);
}

int64_t RealtimeClock::getSamplePosition() const noexcept {
    return sample_position_.load(std::memory_order_acquire);
}

uint32_t RealtimeClock::getTempo() const noexcept {
    return tempo_bpm_.load(std::memory_order_acquire);
}

double RealtimeClock::getTicksPerSample() const noexcept {
    return ticks_per_sample_.load(std::memory_order_acquire);
}

bool RealtimeClock::isRunning() const noexcept {
    return running_.load(std::memory_order_acquire);
}

int32_t RealtimeClock::getCurrentBar(int64_t position) const noexcept {
    const auto spb = samples_per_beat_.load(std::memory_order_acquire);
    const auto bpb = beats_per_bar_.load(std::memory_order_acquire);
    if (spb <= 0 || bpb <= 0) return 0;
    // Convert position to samples, then to bar
    const double tps = ticks_per_sample_.load(std::memory_order_acquire);
    if (tps <= 0.0) return 0;
    const int64_t samplePos = static_cast<int64_t>(static_cast<double>(position) / tps);
    return static_cast<int32_t>(samplePos / (spb * bpb)) + 1;
}

int32_t RealtimeClock::getCurrentBeat(int64_t position) const noexcept {
    const auto spb = samples_per_beat_.load(std::memory_order_acquire);
    if (spb <= 0) return 0;
    const double tps = ticks_per_sample_.load(std::memory_order_acquire);
    if (tps <= 0.0) return 0;
    const int64_t samplePos = static_cast<int64_t>(static_cast<double>(position) / tps);
    return static_cast<int32_t>((samplePos % (spb * beats_per_bar_.load(std::memory_order_acquire))) / spb) + 1;
}

void RealtimeClock::recalcDerived() noexcept {
    const uint32_t sr = sample_rate_.load(std::memory_order_relaxed);
    const uint32_t bpm = tempo_bpm_.load(std::memory_order_relaxed);
    const uint32_t res = resolution_.load(std::memory_order_relaxed);
    const uint32_t bpn = beat_note_.load(std::memory_order_relaxed);

    if (sr == 0 || bpm == 0 || res == 0 || bpn == 0) {
        ticks_per_sample_.store(0.0, std::memory_order_release);
        samples_per_beat_.store(0, std::memory_order_release);
        samples_per_bar_.store(0, std::memory_order_release);
        return;
    }

    // ticks per sample = resolution / (sampleRate * 60 / bpm)
    const double tps = static_cast<double>(res) / (static_cast<double>(sr) * 60.0 / static_cast<double>(bpm));
    ticks_per_sample_.store(tps, std::memory_order_release);

    // samples per beat = sampleRate * 60 / (bpm * (beatNote/4))
    const int64_t spb = static_cast<int64_t>(static_cast<double>(sr) * 60.0 * static_cast<double>(bpn) / (static_cast<double>(bpm) * 4.0));
    samples_per_beat_.store(spb, std::memory_order_release);

    const auto bb = beats_per_bar_.load(std::memory_order_relaxed);
    samples_per_bar_.store(spb * bb, std::memory_order_release);
}

} // namespace ai_arranger::realtime

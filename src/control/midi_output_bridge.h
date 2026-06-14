#ifndef AI_ARRANGER_CONTROL_MIDI_OUTPUT_BRIDGE_H
#define AI_ARRANGER_CONTROL_MIDI_OUTPUT_BRIDGE_H

#include "control/ui_event_queue.h"
#include "midi/midi_output_provider.h"
#include "engine/uasf/types.h"
#include <atomic>
#include <cstddef>

// ── Engine → MIDI output bridge (Gate 14C Task C) ────────────────────
//
// Lock-free SPSC hand-off from the engine/scheduler thread to the MIDI output.
// The engine calls send() (producer, realtime-safe, no alloc/lock); a consumer
// calls pump(provider) to drain and forward to the active provider. Keeps the
// playback path non-blocking and provider-agnostic.

namespace ai_arranger::control {

class MidiOutputBridge {
public:
    // Producer (engine thread). Returns false if the queue is full (dropped).
    bool send(const uasf::MidiEvent& ev) noexcept {
        if (queue_.push(ev)) return true;
        dropped_.fetch_add(1, std::memory_order_relaxed);
        return false;
    }

    // Consumer. Drains all queued events into the provider; returns the number
    // forwarded. (Provider may itself drop if not connected.)
    size_t pump(midi::IMidiOutputProvider& provider) noexcept {
        size_t n = 0;
        uasf::MidiEvent ev;
        while (queue_.pop(ev)) { provider.send(ev); ++n; }
        forwarded_.fetch_add(n, std::memory_order_relaxed);
        return n;
    }

    uint64_t forwarded() const noexcept { return forwarded_.load(std::memory_order_relaxed); }
    uint64_t dropped() const noexcept { return dropped_.load(std::memory_order_relaxed); }
    static constexpr size_t capacity() noexcept { return 8192 - 1; }

private:
    UiEventQueue<uasf::MidiEvent, 8192> queue_;
    std::atomic<uint64_t> forwarded_{0};
    std::atomic<uint64_t> dropped_{0};
};

} // namespace ai_arranger::control

#endif // AI_ARRANGER_CONTROL_MIDI_OUTPUT_BRIDGE_H

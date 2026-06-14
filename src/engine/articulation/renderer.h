#ifndef AI_ARRANGER_ARTICULATION_RENDERER_H
#define AI_ARRANGER_ARTICULATION_RENDERER_H

#include "engine/uasf/types.h"
#include <cstdint>

// ── Articulation render strategy (Gate 10 Task C) ────────────────────
//
// Turns a single (already chord-transposed) MIDI event plus its track's
// articulation hint into the 0..N MIDI events that actually get scheduled.
// This is the seam where keyswitch / MegaVoice-style articulation rendering
// plugs in without the dispatcher needing to know the strategy.
//
// Realtime contract: render() runs on the dispatch / audio path. Implementations
// MUST NOT allocate, lock, or block. Output is pushed through an EventSink the
// caller owns, so no renderer ever needs its own storage.

namespace ai_arranger::articulation {

// Caller-supplied output sink. The renderer only calls emit().
struct EventSink {
    virtual void emit(const uasf::MidiEvent& ev) noexcept = 0;
protected:
    ~EventSink() = default;
};

// Strategy interface.
class IArticulationRenderer {
public:
    virtual ~IArticulationRenderer() = default;

    // Render `ev` (with its track articulation) into `out`. Realtime-safe.
    virtual void render(const uasf::MidiEvent& ev,
                        const uasf::ArticulationMetadata& art,
                        EventSink& out) const noexcept = 0;

    // Stable identifier for logging / mode selection.
    virtual const char* name() const noexcept = 0;
};

// ── Strategy 1: pass-through baseline (default, backward-compatible) ──
class NaiveRenderer final : public IArticulationRenderer {
public:
    void render(const uasf::MidiEvent& ev,
                const uasf::ArticulationMetadata& art,
                EventSink& out) const noexcept override;
    const char* name() const noexcept override { return "naive"; }
};

// ── Strategy 2: keyswitch articulation ───────────────────────────────
// For a NoteOn on a track whose articulation has_keyswitch, emit a short
// keyswitch note (a low key that selects the articulation on a VST/sampler)
// strictly before the main note, then the main note itself. All other events
// (NoteOff, CC, …) pass through unchanged.
class KeyswitchRenderer final : public IArticulationRenderer {
public:
    // `keyswitchAvailable` is decided at setup time (non-realtime) from the
    // playback target's capabilities. When the target cannot honour keyswitch
    // articulation (e.g. a MegaVoice style routed to a device with no keyswitch
    // support), construct with `false` to gracefully degrade to pass-through —
    // the note still sounds, just without the articulation select. Per the
    // no-silent-fallback rule, the CALLER logs that degradation at construction
    // time; render() stays allocation/lock/syscall-free for the audio path.
    explicit KeyswitchRenderer(bool keyswitchAvailable = true) noexcept
        : keyswitch_available_(keyswitchAvailable) {}

    void render(const uasf::MidiEvent& ev,
                const uasf::ArticulationMetadata& art,
                EventSink& out) const noexcept override;
    const char* name() const noexcept override {
        return keyswitch_available_ ? "keyswitch" : "keyswitch-fallback";
    }

    bool keyswitchAvailable() const noexcept { return keyswitch_available_; }

    // Velocity used for the (inaudible) keyswitch trigger note.
    static constexpr uint8_t kKeyswitchVelocity = 1;

private:
    bool keyswitch_available_;
};

// Shared default instance for the dispatcher's backward-compatible default.
const NaiveRenderer& defaultRenderer() noexcept;

} // namespace ai_arranger::articulation

#endif // AI_ARRANGER_ARTICULATION_RENDERER_H

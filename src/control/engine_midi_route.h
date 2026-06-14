#ifndef AI_ARRANGER_CONTROL_ENGINE_MIDI_ROUTE_H
#define AI_ARRANGER_CONTROL_ENGINE_MIDI_ROUTE_H

#include "engine/midi/scheduler.h"
#include "control/midi_output_bridge.h"
#include "midi/midi_output_provider.h"

// ── Engine → MIDI output wiring (Gate 14C Task D) ────────────────────
//
// Connects the engine scheduler's output to the MIDI output path WITHOUT putting
// any CoreMIDI in the engine: the scheduler callback only pushes onto the
// lock-free bridge; the dispatch side drains the bridge into the provider (where
// CoreMIDI lives). Clean ownership — engine emits events, the route forwards,
// the provider sends.

namespace ai_arranger::control {

// Call once at setup. The scheduler output callback becomes a non-blocking push
// onto the bridge (no CoreMIDI, no alloc on the playback path).
inline void attachEngineOutput(midi::MidiScheduler& scheduler,
                               MidiOutputBridge& bridge) noexcept {
    scheduler.setOutputCallback(
        [&bridge](const uasf::MidiEvent& e) { bridge.send(e); });
}

// Call on the dispatch side (per tick / dispatch loop): drain bridge -> provider.
inline size_t pumpEngineOutput(MidiOutputBridge& bridge,
                               midi::IMidiOutputProvider& provider) noexcept {
    return bridge.pump(provider);
}

} // namespace ai_arranger::control

#endif // AI_ARRANGER_CONTROL_ENGINE_MIDI_ROUTE_H

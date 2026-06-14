#ifndef AI_ARRANGER_CONTROL_CONTROL_EVENTS_H
#define AI_ARRANGER_CONTROL_CONTROL_EVENTS_H

#include <cstdint>

// ── Control events (Gate 12 Task G) ──────────────────────────────────
//
// Vendor-neutral performer actions. Any front-end (MIDI, UI, test) emits the
// same ControlEvents; the engine reacts identically. No Korg/Yamaha button maps
// here — adapters translate device specifics into these.

namespace ai_arranger::control {

enum class ControlAction : uint8_t {
    None = 0,
    Start, Stop, SyncArm,
    Fill,
    VariationA, VariationB, VariationC, VariationD,
    Intro, Ending,
    Tap,
    Panic,
};

struct ControlEvent {
    ControlAction action = ControlAction::None;
    int32_t       param = 0;        // action-specific (e.g. fill target)
    uint64_t      timestamp = 0;    // tick or sample stamp (caller-defined)
};

} // namespace ai_arranger::control

#endif // AI_ARRANGER_CONTROL_CONTROL_EVENTS_H

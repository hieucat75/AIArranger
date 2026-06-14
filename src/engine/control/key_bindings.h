#ifndef AI_ARRANGER_CONTROL_KEY_BINDINGS_H
#define AI_ARRANGER_CONTROL_KEY_BINDINGS_H

#include "engine/control/control_events.h"

// ── Keystroke -> ControlEvent mapping (Gate 13 Task G) ───────────────
//
// Pure, vendor-neutral binding used by the korg-playback CLI (and headless
// tests) to turn keyboard input into ControlEvents fed to the performer adapter.
// Unmapped keys return action None. No I/O, no device dependency.

namespace ai_arranger::control {

inline ControlEvent keyToControlEvent(char key) noexcept {
    switch (key) {
        case 's': return {ControlAction::Start,      0, 0};
        case 'x': return {ControlAction::Stop,       0, 0};
        case 'a': return {ControlAction::SyncArm,    0, 0};
        case 'f': return {ControlAction::Fill,       0, 0};
        case 'e': return {ControlAction::Ending,     0, 0};
        case 'p': return {ControlAction::Panic,      0, 0};
        case '1': return {ControlAction::VariationA, 0, 0};
        case '2': return {ControlAction::VariationB, 0, 0};
        case '3': return {ControlAction::VariationC, 0, 0};
        case '4': return {ControlAction::VariationD, 0, 0};
        default:  return {ControlAction::None,       0, 0};
    }
}

} // namespace ai_arranger::control

#endif // AI_ARRANGER_CONTROL_KEY_BINDINGS_H

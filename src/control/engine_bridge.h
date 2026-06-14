#ifndef AI_ARRANGER_CONTROL_ENGINE_BRIDGE_H
#define AI_ARRANGER_CONTROL_ENGINE_BRIDGE_H

#include "control/ui_event_queue.h"
#include "engine/control/control_events.h"
#include <cstdint>

// ── Async UI <-> engine bridge (Gate 14 Task C) ──────────────────────
//
// Two lock-free SPSC rings: UI -> engine control events, and engine -> UI
// telemetry snapshots. The UI thread never blocks on the engine and never calls
// engine logic synchronously; it pushes intent and polls telemetry. No mutex,
// no allocation, no sync render callbacks.

namespace ai_arranger::control {

// Read-only snapshot the engine publishes for the UI to render.
struct EngineTelemetry {
    bool     playing = false;
    int32_t  section = -1;
    uint8_t  chord_root = 0;
    uint8_t  chord_type = 0;
    int64_t  position_ticks = 0;
    int32_t  state = 0;          // PerformerState as int
    uint32_t dispatched_notes = 0;
};

class EngineBridge {
public:
    // ── UI thread ──────────────────────────────────────────────────
    bool sendControl(const ControlEvent& ev) noexcept { return control_.push(ev); }
    bool pollTelemetry(EngineTelemetry& out) noexcept { return telemetry_.pop(out); }

    // ── Engine thread ──────────────────────────────────────────────
    bool nextControl(ControlEvent& out) noexcept { return control_.pop(out); }
    bool publishTelemetry(const EngineTelemetry& t) noexcept { return telemetry_.push(t); }

    static constexpr size_t controlCapacity() noexcept { return 1024 - 1; }
    static constexpr size_t telemetryCapacity() noexcept { return 256 - 1; }

private:
    UiEventQueue<ControlEvent, 1024>   control_;     // UI -> engine
    UiEventQueue<EngineTelemetry, 256> telemetry_;   // engine -> UI
};

} // namespace ai_arranger::control

#endif // AI_ARRANGER_CONTROL_ENGINE_BRIDGE_H

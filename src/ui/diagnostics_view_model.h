#ifndef AI_ARRANGER_UI_DIAGNOSTICS_VIEW_MODEL_H
#define AI_ARRANGER_UI_DIAGNOSTICS_VIEW_MODEL_H

#include "control/engine_bridge.h"
#include <functional>

// ── Diagnostics ViewModel (Gate 14 Task D) ───────────────────────────
//
// Read-only mirror of engine telemetry for the status/diagnostics panel.
// UI-agnostic; updates observable fields and notifies on change. No JUCE.

namespace ai_arranger::ui {

class DiagnosticsViewModel {
public:
    void setOnChanged(std::function<void()> cb) { on_changed_ = std::move(cb); }

    void applyTelemetry(const control::EngineTelemetry& t) {
        last_ = t;
        ++updates_;
        if (on_changed_) on_changed_();
    }

    bool     playing()  const { return last_.playing; }
    int32_t  section()  const { return last_.section; }
    int32_t  state()    const { return last_.state; }
    int64_t  position() const { return last_.position_ticks; }
    uint8_t  chordRoot() const { return last_.chord_root; }
    uint32_t dispatchedNotes() const { return last_.dispatched_notes; }
    uint64_t updateCount() const { return updates_; }

private:
    control::EngineTelemetry last_{};
    std::function<void()> on_changed_;
    uint64_t updates_ = 0;
};

} // namespace ai_arranger::ui

#endif // AI_ARRANGER_UI_DIAGNOSTICS_VIEW_MODEL_H

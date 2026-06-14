#ifndef AI_ARRANGER_UI_TRANSPORT_VIEW_MODEL_H
#define AI_ARRANGER_UI_TRANSPORT_VIEW_MODEL_H

#include "engine/control/control_events.h"
#include "control/engine_bridge.h"
#include <functional>

// ── Transport ViewModel (Gate 14 Task D) ─────────────────────────────
//
// UI-toolkit-agnostic. Translates transport UI actions into ControlEvents (via a
// sink) and mirrors engine telemetry into observable display state. No JUCE, no
// timing, no engine logic — pure intent + state.

namespace ai_arranger::ui {

class TransportViewModel {
public:
    using ControlSink = std::function<void(const control::ControlEvent&)>;

    void setControlSink(ControlSink sink) { sink_ = std::move(sink); }
    void setOnChanged(std::function<void()> cb) { on_changed_ = std::move(cb); }

    // ── UI actions -> ControlEvents ────────────────────────────────
    void start()      { emit(control::ControlAction::Start); }
    void stop()       { emit(control::ControlAction::Stop); }
    void syncArm()    { emit(control::ControlAction::SyncArm); }
    void fill()       { emit(control::ControlAction::Fill); }
    void intro()      { emit(control::ControlAction::Intro); }
    void ending()     { emit(control::ControlAction::Ending); }
    void panic()      { emit(control::ControlAction::Panic); }
    void variation(int idx) {
        if (idx < 0 || idx > 3) return;
        emit(static_cast<control::ControlAction>(
            static_cast<int>(control::ControlAction::VariationA) + idx));
    }

    // ── Telemetry -> observable state ──────────────────────────────
    void applyTelemetry(const control::EngineTelemetry& t) {
        const bool changed = (playing_ != t.playing) || (section_ != t.section);
        playing_ = t.playing;
        section_ = t.section;
        if (changed && on_changed_) on_changed_();
    }

    bool isPlaying() const { return playing_; }
    int  section()   const { return section_; }

private:
    void emit(control::ControlAction a) {
        if (sink_) sink_({a, 0, 0});
    }
    ControlSink sink_;
    std::function<void()> on_changed_;
    bool playing_ = false;
    int  section_ = -1;
};

} // namespace ai_arranger::ui

#endif // AI_ARRANGER_UI_TRANSPORT_VIEW_MODEL_H

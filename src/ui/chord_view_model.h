#ifndef AI_ARRANGER_UI_CHORD_VIEW_MODEL_H
#define AI_ARRANGER_UI_CHORD_VIEW_MODEL_H

#include "engine/arranger/chord_input.h"
#include "control/engine_bridge.h"
#include <functional>
#include <string>

// ── Chord ViewModel (Gate 14 Task D) ─────────────────────────────────
//
// UI-agnostic chord input + display. A chord pad / keyboard entry calls
// inputChord(); the VM forwards the chord through a sink (the app wires it to the
// engine adapter). Telemetry updates the displayed chord name. No JUCE.

namespace ai_arranger::ui {

class ChordViewModel {
public:
    using ChordSink = std::function<void(const arranger::Chord&)>;

    void setChordSink(ChordSink sink) { sink_ = std::move(sink); }
    void setOnChanged(std::function<void()> cb) { on_changed_ = std::move(cb); }

    // UI input -> chord intent (root MIDI note + type).
    void inputChord(uint8_t root, arranger::ChordType type) {
        if (sink_) sink_({type, root, 0, 0});
    }

    // Telemetry -> displayed chord.
    void applyTelemetry(const control::EngineTelemetry& t) {
        const bool changed = (root_ != t.chord_root) || (type_ != t.chord_type);
        root_ = t.chord_root;
        type_ = t.chord_type;
        if (changed && on_changed_) on_changed_();
    }

    // "C", "Fm", "G7", ... (root pitch class + type suffix).
    std::string displayName() const;

    uint8_t root() const { return root_; }
    uint8_t type() const { return type_; }

private:
    ChordSink sink_;
    std::function<void()> on_changed_;
    uint8_t root_ = 0;
    uint8_t type_ = static_cast<uint8_t>(arranger::ChordType::NoChord);
};

} // namespace ai_arranger::ui

#endif // AI_ARRANGER_UI_CHORD_VIEW_MODEL_H

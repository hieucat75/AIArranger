#pragma once

// ChordPanel (Gate 14) — virtual chord pad + display. Chord input is sent to the
// engine through the bridge (ControlAction::Chord), never by touching the engine
// from the UI thread.

#include <juce_gui_extra/juce_gui_extra.h>
#include "ui/chord_view_model.h"
#include "session/engine_session.h"
#include <vector>
#include <memory>

namespace ai_arranger::app {

class ChordPanel : public juce::Component {
public:
    ChordPanel(ui::ChordViewModel& vm, session::EngineSession& session);
    void resized() override;
    void paint(juce::Graphics& g) override;
    void refresh();

private:
    ui::ChordViewModel&     vm_;
    session::EngineSession& session_;
    std::vector<std::unique_ptr<juce::TextButton>> roots_;
    juce::ComboBox quality_;
    juce::Label    display_;
};

} // namespace ai_arranger::app

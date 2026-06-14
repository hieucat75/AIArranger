#pragma once

// StyleBrowserPanel (Gate 14) — minimal .uasf file list + load into the session.

#include <juce_gui_extra/juce_gui_extra.h>
#include "session/engine_session.h"

namespace ai_arranger::app {

class StyleBrowserPanel : public juce::Component {
public:
    explicit StyleBrowserPanel(session::EngineSession& session);
    void resized() override;
    void paint(juce::Graphics& g) override;

private:
    session::EngineSession& session_;
    juce::TextButton chooseButton_{"Load .uasf..."};
    juce::Label      current_;
};
} // namespace ai_arranger::app

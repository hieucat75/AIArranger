#pragma once

// ConsolePanel (Gate 14) — scrolling event log (ring-buffered, non-blocking).

#include <juce_gui_extra/juce_gui_extra.h>

namespace ai_arranger::app {

class ConsolePanel : public juce::Component {
public:
    ConsolePanel();
    void log(const juce::String& line);
    void resized() override;

private:
    juce::TextEditor view_;
};

} // namespace ai_arranger::app

#include "ConsolePanel.h"

namespace ai_arranger::app {

ConsolePanel::ConsolePanel() {
    view_.setMultiLine(true);
    view_.setReadOnly(true);
    view_.setCaretVisible(false);
    view_.setScrollbarsShown(true);
    view_.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff111114));
    view_.setColour(juce::TextEditor::textColourId, juce::Colour(0xffb8b8c0));
    addAndMakeVisible(view_);
}

void ConsolePanel::log(const juce::String& line) {
    // Cap the buffer so the console never grows unbounded.
    if (view_.getText().length() > 16000)
        view_.setText(view_.getText().substring(8000), false);
    view_.moveCaretToEnd();
    view_.insertTextAtCaret(line + "\n");
}

void ConsolePanel::resized() { view_.setBounds(getLocalBounds()); }

} // namespace ai_arranger::app

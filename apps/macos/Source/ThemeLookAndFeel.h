#pragma once

// ThemeLookAndFeel (Gate 15) — maps ThemeManager tokens to JUCE component colours
// + fonts. CI-optional (GUI). Pure presentation; no engine/timing.

#include <juce_gui_extra/juce_gui_extra.h>
#include "ui/theme/theme_manager.h"

namespace ai_arranger::app {

class ThemeLookAndFeel : public juce::LookAndFeel_V4 {
public:
    explicit ThemeLookAndFeel(ui::ThemeManager& tm) : theme_(tm) { apply(); }

    // Re-read tokens and push colours into the LookAndFeel.
    void apply() {
        const auto& t = theme_.tokens();
        auto col = [](uint32_t argb) { return juce::Colour(argb); };
        setColour(juce::ResizableWindow::backgroundColourId, col(t.background));
        setColour(juce::DocumentWindow::backgroundColourId,  col(t.background));
        setColour(juce::TextButton::buttonColourId,          col(t.surface));
        setColour(juce::TextButton::textColourOffId,         col(t.text));
        setColour(juce::Label::textColourId,                 col(t.text));
        setColour(juce::ComboBox::backgroundColourId,        col(t.surface));
        setColour(juce::ComboBox::textColourId,              col(t.text));
        setColour(juce::TextEditor::backgroundColourId,      col(t.surface));
        setColour(juce::TextEditor::textColourId,            col(t.text_dim));
    }

private:
    ui::ThemeManager& theme_;
};

} // namespace ai_arranger::app

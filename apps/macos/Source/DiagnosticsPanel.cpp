#include "DiagnosticsPanel.h"

namespace ai_arranger::app {

DiagnosticsPanel::DiagnosticsPanel(ui::DiagnosticsViewModel& vm, ui::LatencyMonitor& lat)
    : vm_(vm), lat_(lat) {
    text_.setJustificationType(juce::Justification::topLeft);
    text_.setColour(juce::Label::textColourId, juce::Colour(0xffb8b8c0));
    text_.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 13.0f, 0));
    addAndMakeVisible(text_);
}

void DiagnosticsPanel::refresh() {
    juce::String s;
    s << "playing : " << (vm_.playing() ? "yes" : "no") << "\n"
      << "section : " << vm_.section() << "\n"
      << "state   : " << vm_.state() << "\n"
      << "position: " << (juce::int64) vm_.position() << " ticks\n"
      << "chord rt: " << (int) vm_.chordRoot() << "\n"
      << "updates : " << (juce::int64) vm_.updateCount() << "\n"
      << "---- latency ----\n"
      << "avg ms  : " << juce::String(lat_.average(), 3) << "\n"
      << "max ms  : " << juce::String(lat_.maximum(), 3) << "\n"
      << "jitter  : " << juce::String(lat_.jitter(), 3) << "\n";
    text_.setText(s, juce::dontSendNotification);
}

void DiagnosticsPanel::paint(juce::Graphics& g) {
    g.setColour(juce::Colour(0xff222228));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 4.0f);
}

void DiagnosticsPanel::resized() {
    text_.setBounds(getLocalBounds().reduced(10));
}

} // namespace ai_arranger::app

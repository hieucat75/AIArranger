#include "StageMainComponent.h"

namespace ai_arranger::app {

StageMainComponent::StageMainComponent(ui::TransportViewModel& tvm,
                                       ui::ChordViewModel& cvm,
                                       control::EngineBridge& bridge)
    : cvm_(cvm), transport_(tvm, bridge) {
    addAndMakeVisible(transport_);
    chord_.setJustificationType(juce::Justification::centred);
    chord_.setFont(juce::Font(juce::FontOptions(72.0f, juce::Font::bold)));
    chord_.setColour(juce::Label::textColourId, juce::Colour(0xffffffff));
    chord_.setText("--", juce::dontSendNotification);
    addAndMakeVisible(chord_);
}

void StageMainComponent::refresh() {
    transport_.refresh();
    chord_.setText(cvm_.displayName(), juce::dontSendNotification);
}

void StageMainComponent::paint(juce::Graphics& g) { g.fillAll(juce::Colour(0xff000000)); }

void StageMainComponent::resized() {
    auto r = getLocalBounds();
    chord_.setBounds(r.removeFromTop(r.getHeight() / 2));   // huge chord readout
    transport_.setBounds(r.reduced(12));                    // oversized transport
}

} // namespace ai_arranger::app

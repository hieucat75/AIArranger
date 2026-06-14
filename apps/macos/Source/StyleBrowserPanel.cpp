#include "StyleBrowserPanel.h"
#include "engine/uasf/deserializer.h"

namespace ai_arranger::app {

StyleBrowserPanel::StyleBrowserPanel(session::EngineSession& session)
    : session_(session) {
    current_.setText("Built-in demo style", juce::dontSendNotification);
    current_.setColour(juce::Label::textColourId, juce::Colour(0xffb8b8c0));
    addAndMakeVisible(current_);

    chooseButton_.onClick = [this] {
        auto chooser = std::make_shared<juce::FileChooser>(
            "Load a .uasf style", juce::File(), "*.uasf");
        chooser->launchAsync(
            juce::FileBrowserComponent::openMode |
            juce::FileBrowserComponent::canSelectFiles,
            [this, chooser](const juce::FileChooser& fc) {
                const auto file = fc.getResult();
                if (file == juce::File()) return;
                uasf::UasfDeserializer de;
                auto res = de.deserializeFromFile(file.getFullPathName().toStdString());
                if (res.success) {
                    session_.reset();
                    session_.loadStyle(res.style);
                    session_.boot();
                    current_.setText(file.getFileName(), juce::dontSendNotification);
                }
            });
    };
    addAndMakeVisible(chooseButton_);
}

void StyleBrowserPanel::paint(juce::Graphics& g) {
    g.setColour(juce::Colour(0xff222228));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 4.0f);
}

void StyleBrowserPanel::resized() {
    auto r = getLocalBounds().reduced(8);
    chooseButton_.setBounds(r.removeFromTop(28));
    r.removeFromTop(6);
    current_.setBounds(r.removeFromTop(24));
}

} // namespace ai_arranger::app

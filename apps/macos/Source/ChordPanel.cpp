#include "ChordPanel.h"

namespace ai_arranger::app {

using arranger::ChordType;

namespace {
const char* kRootNames[12] =
    {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
}

ChordPanel::ChordPanel(ui::ChordViewModel& vm, session::EngineSession& session)
    : vm_(vm), session_(session) {
    // Quality selector.
    quality_.addItem("maj", 1);
    quality_.addItem("min", 2);
    quality_.addItem("7",   3);
    quality_.addItem("maj7",4);
    quality_.addItem("m7",  5);
    quality_.setSelectedId(1, juce::dontSendNotification);
    addAndMakeVisible(quality_);

    auto qualityType = [this]() -> ChordType {
        switch (quality_.getSelectedId()) {
            case 2: return ChordType::Minor;
            case 3: return ChordType::Dom7;
            case 4: return ChordType::Maj7;
            case 5: return ChordType::Min7;
            default: return ChordType::Major;
        }
    };

    // 12 root pads (one octave). Root MIDI note = 60 + pitch class.
    for (int pc = 0; pc < 12; ++pc) {
        auto b = std::make_unique<juce::TextButton>(kRootNames[pc]);
        b->onClick = [this, pc, qualityType] {
            vm_.inputChord(static_cast<uint8_t>(60 + pc), qualityType());
        };
        addAndMakeVisible(*b);
        roots_.push_back(std::move(b));
    }

    display_.setJustificationType(juce::Justification::centred);
    display_.setFont(juce::Font(28.0f, juce::Font::bold));
    display_.setColour(juce::Label::textColourId, juce::Colour(0xffe0e0e8));
    display_.setText("--", juce::dontSendNotification);
    addAndMakeVisible(display_);
    juce::ignoreUnused(session_);
}

void ChordPanel::refresh() {
    display_.setText(vm_.displayName(), juce::dontSendNotification);
}

void ChordPanel::paint(juce::Graphics& g) {
    g.setColour(juce::Colour(0xff222228));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 4.0f);
}

void ChordPanel::resized() {
    auto r = getLocalBounds().reduced(8);
    display_.setBounds(r.removeFromTop(40));
    quality_.setBounds(r.removeFromTop(26));
    r.removeFromTop(6);
    const int cols = 6;
    const int cellW = r.getWidth() / cols;
    const int cellH = r.getHeight() / 2;
    for (int i = 0; i < (int)roots_.size(); ++i) {
        const int col = i % cols, row = i / cols;
        roots_[i]->setBounds(r.getX() + col * cellW, r.getY() + row * cellH,
                             cellW - 3, cellH - 3);
    }
}

} // namespace ai_arranger::app

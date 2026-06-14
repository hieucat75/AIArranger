#include "PerformerOverlayPanel.h"

namespace ai_arranger::app {

namespace {
const char* kGrooveNames[5] = {"tight","laid-back","swing-light","live-pop","acoustic-soft"};
}

void PerformerOverlayPanel::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xff15151a));
    const int n = 7;
    auto area = getLocalBounds().reduced(8);
    const int rowH = juce::jmax(16, area.getHeight() / n);

    auto dot = [&](juce::Rectangle<int> r, const juce::String& label, bool on,
                   juce::Colour onCol) {
        auto box = r.removeFromLeft(rowH);
        g.setColour(on ? onCol : juce::Colour(0xff3a3a44));
        g.fillEllipse(box.reduced(4).toFloat());
        g.setColour(juce::Colour(0xffededf2));
        g.drawText(label, r.reduced(4, 0), juce::Justification::centredLeft);
    };

    const auto green = juce::Colour(0xff7ad07a);
    const auto amber = juce::Colour(0xffd9a14b);
    const auto red   = juce::Colour(0xffd9714b);
    const auto cyan  = juce::Colour(0xff18d2e6);

    dot(area.removeFromTop(rowH), "Fill queued",      vm_.fillQueued(), amber);
    dot(area.removeFromTop(rowH), "Variation pending",vm_.variationPending(), amber);
    dot(area.removeFromTop(rowH), "Sync armed",       vm_.syncArmed(), cyan);
    dot(area.removeFromTop(rowH), "MIDI active",      vm_.midiActive(), green);
    {
        auto r = area.removeFromTop(rowH);
        const auto l = vm_.latency();
        const auto c = l == ui::LatencyLevel::Good ? green
                     : l == ui::LatencyLevel::Warn ? amber : red;
        dot(r, "Latency", true, c);
    }
    dot(area.removeFromTop(rowH), "Reconnecting",     vm_.reconnecting(), red);
    {
        auto r = area.removeFromTop(rowH);
        const int gi = vm_.grooveProfile() < 5 ? vm_.grooveProfile() : 0;
        dot(r, juce::String("Groove: ") + kGrooveNames[gi], true, cyan);
    }
}

} // namespace ai_arranger::app

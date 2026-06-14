#include "TransportPanel.h"

namespace ai_arranger::app {

namespace {
struct Btn { const char* label; std::function<void(ui::TransportViewModel&)> action; };
}

TransportPanel::TransportPanel(ui::TransportViewModel& vm, control::EngineBridge& bridge)
    : vm_(vm), bridge_(bridge) {
    const std::vector<Btn> defs = {
        {"Start",   [](ui::TransportViewModel& v){ v.start(); }},
        {"Stop",    [](ui::TransportViewModel& v){ v.stop(); }},
        {"Sync",    [](ui::TransportViewModel& v){ v.syncArm(); }},
        {"Intro",   [](ui::TransportViewModel& v){ v.intro(); }},
        {"Fill",    [](ui::TransportViewModel& v){ v.fill(); }},
        {"Ending",  [](ui::TransportViewModel& v){ v.ending(); }},
        {"Var A",   [](ui::TransportViewModel& v){ v.variation(0); }},
        {"Var B",   [](ui::TransportViewModel& v){ v.variation(1); }},
        {"Var C",   [](ui::TransportViewModel& v){ v.variation(2); }},
        {"Var D",   [](ui::TransportViewModel& v){ v.variation(3); }},
        {"PANIC",   [](ui::TransportViewModel& v){ v.panic(); }},
    };
    for (const auto& d : defs) {
        auto b = std::make_unique<juce::TextButton>(d.label);
        auto action = d.action;
        b->onClick = [this, action] { action(vm_); };
        addAndMakeVisible(*b);
        buttons_.push_back(std::move(b));
    }
    status_.setColour(juce::Label::textColourId, juce::Colour(0xff7ad07a));
    status_.setText("stopped", juce::dontSendNotification);
    addAndMakeVisible(status_);
}

void TransportPanel::refresh() {
    juce::String s = vm_.isPlaying() ? "playing" : "stopped";
    s << "  section=" << vm_.section();
    status_.setText(s, juce::dontSendNotification);
}

void TransportPanel::paint(juce::Graphics& g) {
    g.setColour(juce::Colour(0xff222228));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 4.0f);
}

void TransportPanel::resized() {
    auto r = getLocalBounds().reduced(8);
    status_.setBounds(r.removeFromBottom(20));
    const int n = static_cast<int>(buttons_.size());
    const int w = n > 0 ? r.getWidth() / n : r.getWidth();
    for (auto& b : buttons_) b->setBounds(r.removeFromLeft(w).reduced(2));
}

} // namespace ai_arranger::app

#include "MidiOutputPanel.h"

namespace ai_arranger::app {

MidiOutputPanel::MidiOutputPanel(ui::MidiOutputViewModel& vm) : vm_(vm) {
    devices_.setTextWhenNothingSelected("(no MIDI output)");
    devices_.onChange = [this] {
        if (populating_) return;
        // ComboBox ids are 1-based; map to device index (id 1 == "none").
        const int id = devices_.getSelectedId();
        vm_.selectDevice(id - 2);   // id 1 = none (-1), id 2 = device 0, ...
    };
    addAndMakeVisible(devices_);

    refresh_.onClick = [this] { refresh(); };
    addAndMakeVisible(refresh_);

    for (auto* l : {&status_, &lastMsg_}) {
        l->setColour(juce::Label::textColourId, juce::Colour(0xffb8b8c0));
        addAndMakeVisible(*l);
    }
    refresh();
}

void MidiOutputPanel::refresh() {
    populating_ = true;
    devices_.clear(juce::dontSendNotification);
    devices_.addItem("(none)", 1);
    int id = 2;
    for (const auto& d : vm_.devices())
        devices_.addItem(d.name, id++);
    const int sel = vm_.selectedIndex();
    devices_.setSelectedId(sel < 0 ? 1 : sel + 2, juce::dontSendNotification);
    populating_ = false;

    status_.setText(juce::String("status: ") + vm_.connectionText() +
                    "  sent: " + juce::String((juce::int64) vm_.sentCount()),
                    juce::dontSendNotification);
    lastMsg_.setText(juce::String("last: ") + vm_.lastMessage(),
                     juce::dontSendNotification);
}

void MidiOutputPanel::paint(juce::Graphics& g) {
    g.setColour(juce::Colour(0xff222228));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 4.0f);
}

void MidiOutputPanel::resized() {
    auto r = getLocalBounds().reduced(8);
    auto top = r.removeFromTop(28);
    refresh_.setBounds(top.removeFromRight(80));
    devices_.setBounds(top);
    r.removeFromTop(6);
    status_.setBounds(r.removeFromTop(22));
    lastMsg_.setBounds(r.removeFromTop(22));
}

} // namespace ai_arranger::app

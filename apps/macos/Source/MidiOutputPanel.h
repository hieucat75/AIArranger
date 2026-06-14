#pragma once

// MidiOutputPanel (Gate 14C) — output device selector + telemetry. Selecting a
// device goes through the ViewModel sink (→ MidiOutputManager); the panel is
// read-only telemetry otherwise. No engine/CoreMIDI calls from the view.

#include <juce_gui_extra/juce_gui_extra.h>
#include "ui/midi/midi_output_viewmodel.h"

namespace ai_arranger::app {

class MidiOutputPanel : public juce::Component {
public:
    explicit MidiOutputPanel(ui::MidiOutputViewModel& vm);
    void resized() override;
    void paint(juce::Graphics& g) override;
    void refresh();   // pull device list + telemetry from the ViewModel

private:
    ui::MidiOutputViewModel& vm_;
    juce::ComboBox   devices_;
    juce::TextButton refresh_{"Refresh"};
    juce::Label      status_;
    juce::Label      lastMsg_;
    bool             populating_ = false;
};

} // namespace ai_arranger::app

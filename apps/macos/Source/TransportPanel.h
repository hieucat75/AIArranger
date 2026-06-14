#pragma once

// TransportPanel (Gate 14) — transport buttons that emit ControlEvents through
// the bridge (never call the sequencer directly). UI sends intent only.

#include <juce_gui_extra/juce_gui_extra.h>
#include "ui/transport_view_model.h"
#include "control/engine_bridge.h"
#include <vector>
#include <memory>

namespace ai_arranger::app {

class TransportPanel : public juce::Component {
public:
    TransportPanel(ui::TransportViewModel& vm, control::EngineBridge& bridge);
    void resized() override;
    void paint(juce::Graphics& g) override;
    void refresh();   // pull state from the ViewModel for display

private:
    ui::TransportViewModel& vm_;
    control::EngineBridge&  bridge_;
    std::vector<std::unique_ptr<juce::TextButton>> buttons_;
    juce::Label status_;
};

} // namespace ai_arranger::app

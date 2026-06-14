#pragma once

// StageMainComponent (Gate 15) — fullscreen "stage" layout: oversized transport
// + large chord readout, minimal chrome. Reuses the transport ViewModel/bridge;
// no engine/timing in the view. CI-optional (GUI).

#include <juce_gui_extra/juce_gui_extra.h>
#include "ui/transport_view_model.h"
#include "ui/chord_view_model.h"
#include "control/engine_bridge.h"
#include "TransportPanel.h"

namespace ai_arranger::app {

class StageMainComponent : public juce::Component {
public:
    StageMainComponent(ui::TransportViewModel& tvm,
                       ui::ChordViewModel& cvm,
                       control::EngineBridge& bridge);
    void resized() override;
    void paint(juce::Graphics& g) override;
    void refresh();

private:
    ui::ChordViewModel& cvm_;
    TransportPanel transport_;   // reused, given a large bound
    juce::Label    chord_;       // oversized chord readout
};

} // namespace ai_arranger::app

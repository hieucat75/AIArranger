#pragma once

// PerformerOverlayPanel (Gate 15) — 7 live indicators (fill queued, variation
// pending, sync armed, MIDI active, latency, reconnecting, groove profile).
// Read-only view over PerformerOverlay. CI-optional (GUI).

#include <juce_gui_extra/juce_gui_extra.h>
#include "ui/performance/performer_overlay.h"

namespace ai_arranger::app {

class PerformerOverlayPanel : public juce::Component {
public:
    explicit PerformerOverlayPanel(ui::PerformerOverlay& vm) : vm_(vm) {}
    void paint(juce::Graphics& g) override;
    void refresh() { repaint(); }

private:
    ui::PerformerOverlay& vm_;
};

} // namespace ai_arranger::app

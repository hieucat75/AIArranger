#pragma once

// LatencyOverlayPanel (Gate 15) — rolling latency stats + jitter histogram view
// over performance::LatencyOverlay. CI-optional (GUI).

#include <juce_gui_extra/juce_gui_extra.h>
#include "performance/telemetry/latency_overlay.h"

namespace ai_arranger::app {

class LatencyOverlayPanel : public juce::Component {
public:
    explicit LatencyOverlayPanel(performance::LatencyOverlay& src) : src_(src) {}
    void paint(juce::Graphics& g) override;
    void refresh() { repaint(); }

private:
    performance::LatencyOverlay& src_;
};

} // namespace ai_arranger::app

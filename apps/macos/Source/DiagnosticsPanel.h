#pragma once

// DiagnosticsPanel (Gate 14) — read-only status + latency display.

#include <juce_gui_extra/juce_gui_extra.h>
#include "ui/diagnostics_view_model.h"
#include "ui/latency_monitor.h"

namespace ai_arranger::app {

class DiagnosticsPanel : public juce::Component {
public:
    DiagnosticsPanel(ui::DiagnosticsViewModel& vm, ui::LatencyMonitor& lat);
    void resized() override;
    void paint(juce::Graphics& g) override;
    void refresh();

private:
    ui::DiagnosticsViewModel& vm_;
    ui::LatencyMonitor&       lat_;
    juce::Label text_;
};

} // namespace ai_arranger::app

#pragma once

// MainComponent (Gate 14) — hosts the four panels and owns the engine session,
// bridge, driver, and ViewModels. A message-thread Timer polls telemetry from
// the bridge into the ViewModels (no blocking engine calls from the UI).

#include <juce_gui_extra/juce_gui_extra.h>
#include "session/engine_session.h"
#include "control/engine_bridge.h"
#include "ui/transport_view_model.h"
#include "ui/chord_view_model.h"
#include "ui/diagnostics_view_model.h"
#include "ui/latency_monitor.h"
#include "ui/midi/midi_output_viewmodel.h"
#include "EngineDriver.h"
#include "TransportPanel.h"
#include "ChordPanel.h"
#include "DiagnosticsPanel.h"
#include "StyleBrowserPanel.h"
#include "ConsolePanel.h"
#include "MidiOutputPanel.h"

namespace ai_arranger::app {

class MainComponent : public juce::Component, private juce::Timer {
public:
    MainComponent();
    ~MainComponent() override;

    void resized() override;
    void paint(juce::Graphics& g) override;

private:
    void timerCallback() override;  // message thread: poll telemetry -> ViewModels

    session::EngineSession session_;
    control::EngineBridge  bridge_;
    EngineDriver           driver_{session_, bridge_};

    ui::TransportViewModel   transportVm_;
    ui::ChordViewModel       chordVm_;
    ui::DiagnosticsViewModel diagVm_;
    ui::LatencyMonitor       latency_;
    ui::MidiOutputViewModel  midiVm_;

    TransportPanel    transport_{transportVm_, bridge_};
    ChordPanel        chord_{chordVm_, session_};
    DiagnosticsPanel  diagnostics_{diagVm_, latency_};
    StyleBrowserPanel styles_{session_};
    ConsolePanel      console_;
    MidiOutputPanel   midiOut_{midiVm_};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};

} // namespace ai_arranger::app

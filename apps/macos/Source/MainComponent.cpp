#include "MainComponent.h"

namespace ai_arranger::app {

MainComponent::MainComponent() {
    // UI -> engine: ViewModel intent is pushed onto the lock-free bridge; the
    // engine driver thread drains it. The UI never calls the engine directly.
    transportVm_.setControlSink(
        [this](const control::ControlEvent& e) { bridge_.sendControl(e); });

    // Chord intent -> bridge as a Chord ControlEvent (decoded on the engine
    // thread), keeping the UI off the engine's threads.
    chordVm_.setChordSink([this](const arranger::Chord& c) {
        control::ControlEvent e;
        e.action = control::ControlAction::Chord;
        e.param = (static_cast<int32_t>(c.root) & 0xFF) |
                  (static_cast<int32_t>(c.type) << 8);
        bridge_.sendControl(e);
    });

    addAndMakeVisible(transport_);
    addAndMakeVisible(chord_);
    addAndMakeVisible(diagnostics_);
    addAndMakeVisible(styles_);
    addAndMakeVisible(console_);

    driver_.start();
    setSize(960, 640);
    startTimerHz(30);  // UI refresh; engine runs on the driver thread
}

MainComponent::~MainComponent() {
    stopTimer();
    driver_.stop();
}

void MainComponent::timerCallback() {
    // Drain telemetry (lock-free) and fan out to the ViewModels. No engine calls.
    control::EngineTelemetry t;
    bool any = false;
    while (bridge_.pollTelemetry(t)) {
        transportVm_.applyTelemetry(t);
        chordVm_.applyTelemetry(t);
        diagVm_.applyTelemetry(t);
        any = true;
    }
    if (any) { transport_.refresh(); chord_.refresh(); diagnostics_.refresh(); }
}

void MainComponent::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xff1b1b1f));
}

void MainComponent::resized() {
    auto r = getLocalBounds().reduced(8);
    auto top = r.removeFromTop(120);
    transport_.setBounds(top);
    auto mid = r.removeFromTop(180);
    chord_.setBounds(mid.removeFromLeft(mid.getWidth() / 2));
    diagnostics_.setBounds(mid);
    styles_.setBounds(r.removeFromLeft(r.getWidth() / 2));
    console_.setBounds(r);
}

} // namespace ai_arranger::app

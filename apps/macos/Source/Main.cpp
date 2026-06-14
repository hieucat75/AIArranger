// Gate 14 — AI Arranger macOS app entry point (JUCE).
#include <juce_gui_extra/juce_gui_extra.h>
#include "MainComponent.h"

namespace ai_arranger::app {

class AIArrangerApplication : public juce::JUCEApplication {
public:
    const juce::String getApplicationName() override { return "AI Arranger"; }
    const juce::String getApplicationVersion() override { return "0.14.0"; }
    bool moreThanOneInstanceAllowed() override { return false; }

    void initialise(const juce::String&) override {
        mainWindow_ = std::make_unique<MainWindow>("AI Arranger");
    }
    void shutdown() override { mainWindow_ = nullptr; }
    void systemRequestedQuit() override { quit(); }

private:
    class MainWindow : public juce::DocumentWindow {
    public:
        explicit MainWindow(const juce::String& name)
            : DocumentWindow(name,
                             juce::Colours::black,
                             DocumentWindow::allButtons) {
            setUsingNativeTitleBar(true);
            setContentOwned(new MainComponent(), true);
            setResizable(true, true);
            centreWithSize(getWidth(), getHeight());
            setVisible(true);
        }
        void closeButtonPressed() override {
            JUCEApplication::getInstance()->systemRequestedQuit();
        }
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
    };

    std::unique_ptr<MainWindow> mainWindow_;
};

} // namespace ai_arranger::app

START_JUCE_APPLICATION(ai_arranger::app::AIArrangerApplication)

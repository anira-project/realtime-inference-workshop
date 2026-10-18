#include <juce_gui_basics/juce_gui_basics.h>

#include "MainComponent.h"

namespace hands {

class Application : public juce::JUCEApplication {
public:
    const juce::String getApplicationName() override { return JUCE_APPLICATION_NAME_STRING; }
    const juce::String getApplicationVersion() override { return JUCE_APPLICATION_VERSION_STRING; }
    bool moreThanOneInstanceAllowed() override { return false; }

    void initialise(const juce::String&) override { m_window = std::make_unique<MainWindow>(getApplicationName()); }
    void shutdown() override { m_window.reset(); }
    void systemRequestedQuit() override { quit(); }

private:
    class MainWindow : public juce::DocumentWindow {
    public:
        explicit MainWindow(const juce::String& name)
            : DocumentWindow(name, juce::Colour(0xff1c1d21), DocumentWindow::allButtons) {
            setUsingNativeTitleBar(true);
            setContentOwned(new MainComponent(), true);
            setResizable(true, true);
            setResizeLimits(960, 700, 4096, 4096);
            centreWithSize(getWidth(), getHeight());
            setVisible(true);
        }

        void closeButtonPressed() override { JUCEApplication::getInstance()->systemRequestedQuit(); }
    };

    std::unique_ptr<MainWindow> m_window;
};

}  // namespace hands

START_JUCE_APPLICATION(hands::Application)

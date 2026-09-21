// Interactive gallery app: shows one of every zqsfx::ui control against the house
// palette, so a product owner can review the shared look without opening a plugin.

#include <juce_gui_basics/juce_gui_basics.h>
#include <zqsfx_ui/zqsfx_ui.h>
#include "DemoComponent.h"

class ZqsfxUiDemoApplication : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override    { return "ZQ SFX UI Gallery"; }
    const juce::String getApplicationVersion() override  { return "0.1.0"; }
    bool moreThanOneInstanceAllowed() override            { return true; }

    void initialise (const juce::String&) override
    {
        juce::LookAndFeel::setDefaultLookAndFeel (&lookAndFeel);
        mainWindow = std::make_unique<MainWindow> (getApplicationName());
    }

    void shutdown() override
    {
        mainWindow = nullptr;
        juce::LookAndFeel::setDefaultLookAndFeel (nullptr);
    }

private:
    class MainWindow : public juce::DocumentWindow
    {
    public:
        explicit MainWindow (const juce::String& name)
            : juce::DocumentWindow (name, zqsfx::ui::colour::chassisMid, juce::DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar (true);
            setContentOwned (new zqsfx::ui::DemoComponent (processor.apvts), true);
            centreWithSize (getWidth(), getHeight());
            setResizable (true, true);
            setVisible (true);
        }

        void closeButtonPressed() override
        {
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }

    private:
        zqsfx::ui::DemoProcessor processor;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
    };

    zqsfx::ui::LookAndFeel lookAndFeel;
    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION (ZqsfxUiDemoApplication)

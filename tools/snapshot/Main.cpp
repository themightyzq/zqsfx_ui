// Headless PNG renderer of the gallery demo, so the visual result of a change can be
// inspected without opening a window (CI, or a quick look after editing tokens).

#include <iostream>
#include <juce_gui_basics/juce_gui_basics.h>
#include <zqsfx_ui/zqsfx_ui.h>
#include "DemoComponent.h"

namespace
{
bool writePng (const juce::Image& image, const juce::File& file)
{
    file.getParentDirectory().createDirectory();
    std::unique_ptr<juce::FileOutputStream> stream (file.createOutputStream());
    if (stream == nullptr)
        return false;
    stream->setPosition (0);
    stream->truncate();
    juce::PNGImageFormat png;
    return png.writeImageToStream (image, *stream);
}
} // namespace

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    const juce::File outputFile = argc > 1
        ? juce::File::getCurrentWorkingDirectory().getChildFile (juce::String (argv[1]))
        : juce::File::getCurrentWorkingDirectory().getChildFile ("snapshots/gallery.png");
    const juce::File disabledFile = outputFile.getSiblingFile ("gallery_disabled.png");

    zqsfx::ui::LookAndFeel lookAndFeel;
    juce::LookAndFeel::setDefaultLookAndFeel (&lookAndFeel);

    zqsfx::ui::DemoProcessor processor;
    zqsfx::ui::DemoComponent demo (processor.apvts);
    demo.setSize (1100, 720);

    bool ok = true;

    {
        auto image = demo.createComponentSnapshot (demo.getLocalBounds(), true, 2.0f);
        if (writePng (image, outputFile))
            std::cout << "Wrote " << outputFile.getFullPathName() << std::endl;
        else
        {
            std::cerr << "Failed to write " << outputFile.getFullPathName() << std::endl;
            ok = false;
        }
    }

    // grabKeyboardFocus is not reliable headless (no real focus manager without a
    // window on screen), so the second render exercises the disabled/inapplicable
    // state instead of the focus-ring state.
    demo.setAllControlsActive (false);

    {
        auto image = demo.createComponentSnapshot (demo.getLocalBounds(), true, 2.0f);
        if (writePng (image, disabledFile))
            std::cout << "Wrote " << disabledFile.getFullPathName() << std::endl;
        else
        {
            std::cerr << "Failed to write " << disabledFile.getFullPathName() << std::endl;
            ok = false;
        }
    }

    juce::LookAndFeel::setDefaultLookAndFeel (nullptr);

    return ok ? 0 : 1;
}

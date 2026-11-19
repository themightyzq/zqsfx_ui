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

    // The House About box, as a plugin shows it (delay row) and as an app shows it (none),
    // each over a host the size of a small plugin window.
    auto renderAbout = [&] (const juce::String& name, juce::AudioProcessor* proc, const juce::String& id)
    {
        // Stands in for a plugin window, so the dimmed backdrop reads as it will in a Product.
        struct Window : juce::Component
        {
            void paint (juce::Graphics& g) override { g.setGradientFill (zqsfx::ui::gradients::chassis (getLocalBounds().toFloat())); g.fillAll(); }
        } host;
        host.setSize (560, 520);
        zqsfx::ui::AboutInfo info;
        info.productName = proc != nullptr ? "Unravel" : "Chord Foundry";
        info.version = "0.7.0";
        info.credits = { "Fonts: Barlow Condensed, IBM Plex Mono, VT323 (SIL Open Font License)" };
        if (proc != nullptr)
            info.credits.add ("Knobs: CC0 designs from the g200kg KnobGallery");
        info.sourceUrl = "https://github.com/themightyzq/" + id;
        info.processor = proc;
        auto& box = zqsfx::ui::AboutBox::show (host, info, nullptr);
        juce::ignoreUnused (box);
        auto file = outputFile.getSiblingFile (name);
        auto image = host.createComponentSnapshot (host.getLocalBounds(), true, 2.0f);
        if (writePng (image, file))
            std::cout << "Wrote " << file.getFullPathName() << std::endl;
        else
        {
            std::cerr << "Failed to write " << file.getFullPathName() << std::endl;
            ok = false;
        }
    };
    renderAbout ("about_plugin.png", &processor, "unravel");
    renderAbout ("about_app.png", nullptr, "chordfoundry");

    juce::LookAndFeel::setDefaultLookAndFeel (nullptr);

    return ok ? 0 : 1;
}

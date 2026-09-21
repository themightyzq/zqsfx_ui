#pragma once
// Base for every screen-style readout: value fields, meters, tuners, curve displays.
//
// Origin: wraps Broken's (Project_TurboSynth) TsLookAndFeel::drawScreen bezel/glass
// treatment in a reusable Component so a product only has to implement paintContent.

#include <juce_gui_basics/juce_gui_basics.h>
#include "../lookandfeel/LookAndFeel.h"

namespace zqsfx::ui
{
class PhosphorScreen : public juce::Component
{
public:
    bool scanlines = true;

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        LookAndFeel::drawScreen (g, bounds, scanlines);
        auto glass = bounds.reduced (1.0f).toNearestInt();
        paintContent (g, glass);
    }

    // Draws inside the glass area (already inset from the bezel painted above). The
    // default does nothing, so a plain PhosphorScreen is a valid empty screen.
    //
    // PhosphorScreen itself is a container and is not made accessible by default
    // (setAccessible(true) is NOT called here). A subclass that actually shows data
    // must set its own accessible title/description, since a screen reader has no
    // useful name for a bare rectangle of glass.
    virtual void paintContent (juce::Graphics&, juce::Rectangle<int> /*glassArea*/) {}

    ~PhosphorScreen() override = default;
};
} // namespace zqsfx::ui

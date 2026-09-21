#pragma once
// Non-interactive status LED: round, accent when on, dark when off, with an optional
// caption to its right.
//
// Origin: generalised from Broken's (Project_TurboSynth) Theme.h "Light", which hard
// coded the caption text to "TAPE". Here the caption is a plain juce::String member;
// an empty string draws no text at all.

#include <juce_gui_basics/juce_gui_basics.h>
#include "../tokens/Tokens.h"

namespace zqsfx::ui
{
struct Led : public juce::Component, public juce::SettableTooltipClient
{
    juce::String text;               // caption to the right of the lamp; empty draws nothing
    bool on = false;
    int  dia = geom::lightSize;      // lamp diameter in px
    bool centred = false;            // centre the LED+text unit instead of left-hugging

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        const float d = (float) dia;
        const float unitW = d * 2.0f + 6.0f + 34.0f; // LED offset + caption estimate
        const float x0 = centred ? b.getCentreX() - unitW * 0.5f : b.getX();
        auto led = juce::Rectangle<float> (d, d).withCentre ({ x0 + d, b.getCentreY() });
        if (on)
        {
            g.setColour (colour::accent.withAlpha (0.35f));   // soft glow halo
            g.fillEllipse (led.expanded (3.0f));
            g.setColour (colour::accent);
        }
        else
            g.setColour (colour::ledOffFill);
        g.fillEllipse (led);
        g.setColour (on ? juce::Colours::black.withAlpha (0.7f) : colour::ledOffRim);
        g.drawEllipse (led, 1.0f);

        if (text.isNotEmpty())
        {
            g.setColour (on ? colour::silkLabel : colour::silkCaption);
            g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)).withExtraKerningFactor (0.16f));
            g.drawText (text, getLocalBounds().withTrimmedLeft ((int) (x0 - b.getX()) + (int) d * 2 + 6),
                        juce::Justification::centredLeft);
        }
    }
};
} // namespace zqsfx::ui

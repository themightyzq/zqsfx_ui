#pragma once
// LitToggle (a small square LED-button) and TextToggle (a legend button that shows
// its own state), both bound to an APVTS bool parameter.
//
// Origin: ported 1:1 from Broken's (Project_TurboSynth) Controls.h "LitToggle" and
// "TextToggle", same constructor signatures and public members, plus the
// accessibility floor addition: setDescription and setHelpText on the inner control.

#include <juce_audio_processors/juce_audio_processors.h>
#include "../tokens/Tokens.h"

namespace zqsfx::ui
{
using APVTS = juce::AudioProcessorValueTreeState;

// Small square light: filled accent when the bound bool param is on, dark outline
// when off. Used for hard-bypass style module on/off switches.
class LitToggle : public juce::Component
{
public:
    LitToggle (APVTS& apvts, const juce::String& paramId, const juce::String& tooltip)
    {
        const auto accessibleName = tooltip.upToFirstOccurrenceOf (".", false, false);
        button.setClickingTogglesState (true);
        button.setTooltip (tooltip);
        button.setTitle (accessibleName);
        button.setDescription (tooltip);
        button.setHelpText (tooltip);
        addAndMakeVisible (button);
        attachment = std::make_unique<APVTS::ButtonAttachment> (apvts, paramId, button);
    }

    void setActive (bool active)
    {
        setEnabled (active);
        button.setAlpha (active ? 1.0f : geom::dimAlpha);
        setAlpha (1.0f);
    }

    void resized() override { button.setBounds (getLocalBounds()); }

private:
    struct LedButton : public juce::Button
    {
        LedButton() : juce::Button (juce::String()) {}
        void paintButton (juce::Graphics& g, bool, bool) override
        {
            // round rack LED: accent + glow when active, dark well with a faint rim
            // when hard-bypassed
            auto b = getLocalBounds().toFloat();
            const float d = juce::jmin ((float) geom::lightSize, juce::jmin (b.getWidth(), b.getHeight()) - 2.0f);
            auto led = juce::Rectangle<float> (d, d).withCentre (b.getCentre());
            if (getToggleState())
            {
                g.setColour (colour::accent.withAlpha (0.35f));
                g.fillEllipse (led.expanded (3.5f));
                g.setColour (colour::accent);
            }
            else
                g.setColour (colour::ledOffFill);
            g.fillEllipse (led);
            g.setColour (getToggleState() ? juce::Colours::black.withAlpha (0.7f) : colour::ledOffRim);
            g.drawEllipse (led, 1.0f);
        }
    } button;

    std::unique_ptr<APVTS::ButtonAttachment> attachment;
};

// Text toggle button. invertLit=true means "lit" corresponds to toggle-state OFF
// (value 0) rather than ON, for controls where the LookAndFeel should show the
// button lit at the parameter's default (0) state.
class TextToggle : public juce::Component
{
public:
    TextToggle (APVTS& apvts, const juce::String& paramId, const juce::String& text,
                const juce::String& tooltip, bool invertLit = false,
                const juce::String& onText = {})
    {
        button.setButtonText (text);
        button.setClickingTogglesState (true);
        button.setTooltip (tooltip);
        if (onText.isNotEmpty()) // state-labeled toggle: shows what it currently IS
        {
            button.onStateChange = [this, text, onText]
            { button.setButtonText (button.getToggleState() ? onText : text); };
            button.onStateChange();
        }

        if (invertLit)
            button.getProperties().set ("invertLit", true); // LnF flips lit/unlit

        button.setTitle (text);
        button.setDescription (tooltip);
        button.setHelpText (tooltip);
        addAndMakeVisible (button);
        attachment = std::make_unique<APVTS::ButtonAttachment> (apvts, paramId, button);
    }

    void resized() override { button.setBounds (getLocalBounds()); }

    juce::TextButton button;

private:
    std::unique_ptr<APVTS::ButtonAttachment> attachment;
};
} // namespace zqsfx::ui

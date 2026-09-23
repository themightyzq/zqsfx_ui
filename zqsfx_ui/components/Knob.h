#pragma once
// Rotary knob with an optional title label above it, bound to an APVTS parameter.
//
// Origin: ported 1:1 from Broken's (Project_TurboSynth) Controls.h "Knob", same
// constructor signature and public members, plus the accessibility floor addition
// (docs/ZQSFX_UI_STYLE_GUIDE.md section 8): every wrapper also calls setDescription
// and setHelpText on its inner control, not just setTitle.

#include <juce_audio_processors/juce_audio_processors.h>
#include "../lookandfeel/LookAndFeel.h"
#include "../tokens/Tokens.h"

namespace zqsfx::ui
{
using APVTS = juce::AudioProcessorValueTreeState;

class Knob : public juce::Component
{
public:
    // readout/units are additive: default (false, {}) preserves prior behaviour for every
    // existing call site, including the big-knob path (which never consults them).
    Knob (APVTS& apvts, const juce::String& paramId, const juce::String& labelText,
          const juce::String& tooltip, bool big, bool readout = false,
          const juce::String& units = {},
          std::function<juce::String (double)> fmt = nullptr)
    {
        slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        if (big)
        {
            slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 16);
            slider.setNumDecimalPlacesToDisplay (1); // "20000.0", not "20000.0..." or 7-decimal noise
        }
        else
            slider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
        slider.setTooltip (tooltip);
        addAndMakeVisible (slider);

        if (labelText.isNotEmpty())
        {
            title.setText (labelText, juce::dontSendNotification);
            styleControlLabel (title);
            addAndMakeVisible (title);
        }

        // screen readers get the silkscreen name, not an anonymous slider
        const auto accessibleName = labelText.isNotEmpty() ? labelText : paramId;
        slider.setTitle (accessibleName);
        slider.setDescription (tooltip);
        slider.setHelpText (tooltip);
        attachment = std::make_unique<APVTS::SliderAttachment> (apvts, paramId, slider);

        // Double-click resets to the parameter's own default. Must come AFTER the
        // attachment, which is what gives the slider its range. The default is stored
        // normalised, so convert it back into the parameter's real units.
        //
        // This lives here rather than per-product deliberately: every product built on
        // this module inherits it, and before this only three of ten had the behaviour
        // at all, which made it feel arbitrary to anyone using more than one of them.
        if (auto* param = apvts.getParameter (paramId))
            slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));

        if (big)
        {
            // the attachment installs the parameter's own text conversion (7-decimal
            // noise); replace it AFTER attaching so the readout stays one decimal
            slider.textFromValueFunction = [] (double v)
            { if (std::abs (v) < 0.05) v = 0.0; return juce::String (v, 1); }; // no "-0.0"
            slider.updateText();
        }
        else if (readout)
        {
            // small-knob numeric readout (e.g. a millisecond value) - installed after the
            // attachment for the same reason as the big-knob path above.
            slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 16);
            if (fmt != nullptr) // custom formatter, e.g. 0..1 params shown as %
                slider.textFromValueFunction = std::move (fmt);
            else
                slider.textFromValueFunction = [units] (double v)
                { if (std::abs (v) < 0.5) v = 0.0; return juce::String (v, 0) + " " + units; }; // no "-0"
            slider.updateText();
        }
    }

    // screen-reader name override for panels where two knobs share a silk label
    void setAccessibleTitle (const juce::String& t) { slider.setTitle (t); }

    void setActive (bool active)
    {
        setEnabled (active);
        // dim the CONTROL to spec, but keep the label half-readable: a new user must be
        // able to learn the control exists even when the current mode disables it
        slider.setAlpha (active ? 1.0f : geom::dimAlpha);
        title.setAlpha (active ? 1.0f : 0.55f);
        setAlpha (1.0f);
        repaint(); // the tick ring in paint() dims with the slider
    }

    // The silkscreen tick ring is printed on the PANEL around the knob, not on the knob
    // image itself, so it lives here in the component's paint, using the slider's real
    // rotary bounds from the LookAndFeel.
    void paint (juce::Graphics& g) override
    {
        auto layout = slider.getLookAndFeel().getSliderLayout (slider);
        auto r = layout.sliderBounds.toFloat();
        const float dial = juce::jmin (r.getWidth(), r.getHeight());
        auto square = r.withSizeKeepingCentre (dial, dial).translated ((float) slider.getX(), (float) slider.getY());
        juce::Graphics::ScopedSaveState ss (g);
        g.setOpacity (slider.getAlpha());
        LookAndFeel::drawTickRing (g, square);
    }

    void resized() override
    {
        auto b = getLocalBounds();
        if (title.getText().isNotEmpty())
            title.setBounds (b.removeFromTop (13));
        slider.setBounds (b);
    }

    juce::Slider slider;

private:
    juce::Label title;
    std::unique_ptr<APVTS::SliderAttachment> attachment;
};
} // namespace zqsfx::ui

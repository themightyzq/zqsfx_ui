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
#include "Dial.h"
#include "FocusRings.h"

namespace zqsfx::ui
{
// The default reader for a small knob's custom readout, so what a user sees can be typed back:
// "50 %", "10.3 kHz", "2k", "440 Hz", "-6 dB", "-inf". The number is the leading numeric part;
// a k or K right after it (spaces allowed) means thousands; a % divides by 100 only when the
// range lies inside -1..1. "-inf" and text with no digits give the range start (ValueEntry
// decides whether such text is acceptable).
inline double parseReadoutText (const juce::String& text, juce::Range<double> range)
{
    const auto t = text.trim();
    if (t.containsIgnoreCase ("-inf") || ! t.containsAnyOf ("0123456789"))
        return range.getStart();

    // The first number anywhere in the text, so a label before it ("Tonal 50", "x2.0") is
    // skipped: the first digit, or a point right before a digit.
    const int n = t.length();
    const auto isDigitAt = [&] (int k) { return k >= 0 && k < n && juce::CharacterFunctions::isDigit (t[k]); };
    int i = 0;
    while (i < n && ! (isDigitAt (i) || (t[i] == '.' && isDigitAt (i + 1))))
        ++i;

    // A sign before it, with spaces allowed between ("-6", "- 5"), but not one glued to a word
    // ("x-2" keeps its sign; "5-10" never gets here, its first digit is the 5).
    juce::String number;
    {
        int j = i - 1;
        while (j >= 0 && juce::CharacterFunctions::isWhitespace (t[j])) --j;
        if (j >= 0 && (t[j] == '-' || t[j] == '+'))
            number << t[j];
    }

    // Copy the number. A comma is a thousands separator only when exactly three digits follow it
    // and no further digit ("1,000", "1,000,000"); any other comma is a decimal point ("10,5").
    bool seenPoint = false;
    while (i < n)
    {
        if (isDigitAt (i))
            number << t[i];
        else if (t[i] == ',' && isDigitAt (i - 1) && isDigitAt (i + 1) && isDigitAt (i + 2)
                 && isDigitAt (i + 3) && ! isDigitAt (i + 4) && ! seenPoint)
            ; // thousands separator: dropped
        else if ((t[i] == '.' || t[i] == ',') && ! seenPoint && isDigitAt (i + 1))
        {
            number << '.';
            seenPoint = true;
        }
        else
            break;
        ++i;
    }

    double v = number.getDoubleValue();
    while (i < n && juce::CharacterFunctions::isWhitespace (t[i])) ++i;
    if (i < n && (t[i] == 'k' || t[i] == 'K'))
        v *= 1000.0;
    if (t.substring (i).containsChar ('%') && range.getStart() >= -1.0 && range.getEnd() <= 1.0)
        v /= 100.0;
    return v;
}

using APVTS = juce::AudioProcessorValueTreeState;

class Knob : public juce::Component
{
public:
    // The inner slider (now the namespace-level zqsfx::ui::Dial; this alias keeps the
    // Knob::Dial spelling that products already use).
    using Dial = zqsfx::ui::Dial;

    // readout/units are additive: default (false, {}) preserves prior behaviour for every
    // existing call site, including the big-knob path (which never consults them).
    Knob (APVTS& apvts, const juce::String& paramId, const juce::String& labelText,
          const juce::String& tooltip, bool big, bool readout = false,
          const juce::String& units = {},
          std::function<juce::String (double)> fmt = nullptr,
          std::function<double (const juce::String&)> parse = nullptr)
    {
        slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        markOwnFocusRing (slider); // Knob paints its own ring around the dial face
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
        setDoubleClickDefault (slider, apvts, paramId);

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
            const bool customFormat = fmt != nullptr;
            if (customFormat) // custom formatter, e.g. 0..1 params shown as %
                slider.textFromValueFunction = std::move (fmt);
            else
                slider.textFromValueFunction = [units] (double v)
                { if (std::abs (v) < 0.5) v = 0.0; return juce::String (v, 0) + " " + units; }; // no "-0"
            // Typed text must read back what the readout shows; the attachment's parser knows
            // only the parameter's own text. The plain "<number> <units>" readout already
            // matches the parameter's units, so it keeps the attachment's parser: the default
            // reader would turn "12 kHz" on a kHz parameter into 12000.
            if (parse != nullptr)
                slider.valueFromTextFunction = std::move (parse);
            else if (customFormat)
                slider.valueFromTextFunction = [this] (const juce::String& t)
                { return parseReadoutText (t, slider.getRange()); };
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

        // Keyboard focus ring (workspace rule: every interactive control has a visible focus
        // indicator). Accent is the house "active" colour; it carries no data meaning here.
        if (slider.hasKeyboardFocus (false))
        {
            g.setColour (colour::accent);
            g.drawEllipse (square.expanded (3.0f), 1.5f);
        }
    }

    void resized() override
    {
        auto b = getLocalBounds();
        if (title.getText().isNotEmpty())
            title.setBounds (b.removeFromTop (13));
        slider.setBounds (b);
    }

    Dial slider;

private:
    juce::Label title;
    std::unique_ptr<APVTS::SliderAttachment> attachment;
};
} // namespace zqsfx::ui

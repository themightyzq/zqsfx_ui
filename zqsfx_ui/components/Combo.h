#pragma once
// Labelled ComboBox bound to an APVTS choice parameter.
//
// Origin: ported 1:1 from Broken's (Project_TurboSynth) Controls.h "Combo", same
// constructor signature and public members, plus the accessibility floor addition:
// setDescription and setHelpText on the inner box, not just setTitle.

#include <juce_audio_processors/juce_audio_processors.h>
#include "../tokens/Tokens.h"

namespace zqsfx::ui
{
using APVTS = juce::AudioProcessorValueTreeState;

class Combo : public juce::Component
{
public:
    // Item order MUST match the StringArray order (ids 1..N).
    Combo (APVTS& apvts, const juce::String& paramId, const juce::StringArray& items,
           const juce::String& labelText, const juce::String& tooltip)
    {
        for (int i = 0; i < items.size(); ++i)
            box.addItem (items[i], i + 1);
        box.setTooltip (tooltip);
        addAndMakeVisible (box);

        if (labelText.isNotEmpty())
        {
            title.setText (labelText, juce::dontSendNotification);
            styleControlLabel (title);
            addAndMakeVisible (title);
        }

        const auto accessibleName = labelText.isNotEmpty() ? labelText : paramId;
        box.setTitle (accessibleName);
        box.setDescription (tooltip);
        box.setHelpText (tooltip);
        attachment = std::make_unique<APVTS::ComboBoxAttachment> (apvts, paramId, box);
    }

    void setActive (bool active)
    {
        setEnabled (active);
        box.setAlpha (active ? 1.0f : geom::dimAlpha);
        title.setAlpha (active ? 1.0f : 0.55f);   // label stays learnable (see Knob)
        setAlpha (1.0f);
    }

    void resized() override
    {
        auto b = getLocalBounds();
        if (title.getText().isNotEmpty())
            title.setBounds (b.removeFromTop (13));
        box.setBounds (b);
    }

    juce::ComboBox box;

private:
    juce::Label title;
    std::unique_ptr<APVTS::ComboBoxAttachment> attachment;
};
} // namespace zqsfx::ui

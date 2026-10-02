#pragma once
// House rotary dial: a juce::Slider that takes keyboard focus, shows a focus ring, and adds
// Shift+arrow as a fine step. It is a drop-in replacement for a juce::Slider member, so a
// product gets the house behaviour without re-laying out its editor; Knob uses it inside.
//
// juce::Slider refuses keyboard focus by default and steps only on unmodified arrow keys, so
// before 0.4.0 a house knob was not keyboard-operable at all. Dial keeps JUCE's plain-arrow
// step (the parameter interval, or 1 % of the range) and adds Shift+arrow at one tenth of it.
//
// Pair it with setDoubleClickDefault() after the APVTS attachment exists, so a double-click
// returns the control to its parameter's default.

#include <juce_audio_processors/juce_audio_processors.h>

namespace zqsfx::ui
{
class Dial : public juce::Slider
{
public:
    Dial()
    {
        setWantsKeyboardFocus (true);
        // The LookAndFeel's createFocusOutlineForComponent draws the ring. Knob turns this off
        // because it paints its own ring around the dial face.
        setHasFocusOutline (true);
    }

    // The fine step Shift+arrow applies: a tenth of the plain-arrow step.
    double getFineStep() const
    {
        const double coarse = juce::approximatelyEqual (getInterval(), 0.0)
                                ? getRange().getLength() * 0.01 : getInterval();
        return coarse * 0.1;
    }

    bool keyPressed (const juce::KeyPress& key) override
    {
        const auto mods = key.getModifiers();
        if (mods.isShiftDown() && ! mods.isCommandDown() && ! mods.isCtrlDown() && ! mods.isAltDown())
        {
            double delta = 0.0;
            if (key.isKeyCode (juce::KeyPress::rightKey) || key.isKeyCode (juce::KeyPress::upKey))
                delta = getFineStep();
            else if (key.isKeyCode (juce::KeyPress::leftKey) || key.isKeyCode (juce::KeyPress::downKey))
                delta = -getFineStep();
            if (delta == 0.0)
                return juce::Slider::keyPressed (key);
            setValue (getValue() + delta, juce::sendNotificationSync);
            return true;
        }
        return juce::Slider::keyPressed (key);
    }

    void focusGained (FocusChangeType) override { repaintParentForFocus(); }
    void focusLost (FocusChangeType) override { repaintParentForFocus(); }

private:
    void repaintParentForFocus()
    {
        if (auto* parent = getParentComponent()) parent->repaint();
    }
};

// Double-click returns the slider to the parameter's own default. Call it AFTER the slider
// attachment exists: the attachment is what gives the slider its range. The default is
// stored normalised, so it is converted back into the parameter's real units.
inline void setDoubleClickDefault (juce::Slider& slider, const juce::RangedAudioParameter& param)
{
    slider.setDoubleClickReturnValue (true, param.convertFrom0to1 (param.getDefaultValue()));
}

inline void setDoubleClickDefault (juce::Slider& slider,
                                   juce::AudioProcessorValueTreeState& apvts,
                                   const juce::String& paramId)
{
    if (auto* param = apvts.getParameter (paramId))
        setDoubleClickDefault (slider, *param);
}

// For controls that are not bound to an APVTS parameter: the default in the slider's units.
inline void setDoubleClickDefault (juce::Slider& slider, double defaultValue)
{
    slider.setDoubleClickReturnValue (true, defaultValue);
}
} // namespace zqsfx::ui

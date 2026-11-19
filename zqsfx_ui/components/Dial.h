#pragma once
// House rotary dial: a juce::Slider that takes keyboard focus, shows a focus ring, and adds
// Shift+arrow as a fine step. It is a drop-in replacement for a juce::Slider member, so a
// product gets the house behaviour without re-laying out its editor; Knob uses it inside.
//
// juce::Slider refuses keyboard focus by default and steps only on unmodified arrow keys, so
// before 0.4.0 a house knob was not keyboard-operable at all. Dial keeps JUCE's plain-arrow
// step (the parameter interval, or 1 % of the range) and adds Shift+arrow at one tenth of it
// on continuous controls, or one interval on stepped ones.
//
// Pair it with setDoubleClickDefault() after the APVTS attachment exists, so a double-click
// returns the control to its parameter's default. Right-click opens the House menu and Return
// opens the typing box (see ValueEntry.h); a subclass that overrides mouseDown keeps its own menu.

#include <juce_audio_processors/juce_audio_processors.h>

#include "ValueEntry.h"

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

    // Typing a value. Public so a Product's own menu can add the House items to it.
    ValueEntry valueEntry { *this, ValueEntry::dialHandlesInput };

    // The fine step Shift+arrow applies: a tenth of the plain-arrow step (1 % of the range) on
    // a continuous control. A stepped control cannot move by less than its interval, so there
    // Shift+arrow moves one interval instead of a tenth that setValue would snap back to zero.
    double getFineStep() const
    {
        if (! juce::approximatelyEqual (getInterval(), 0.0))
            return getInterval();
        return getRange().getLength() * 0.01 * 0.1;
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu())
        {
            // Slider::mouseDown is skipped, so its drag state is stale: swallow the drag and
            // release that follow so this click cannot move the value.
            swallowingPopupClick = true;
            valueEntry.showMenu();
            return;
        }
        swallowingPopupClick = false;
        juce::Slider::mouseDown (e);
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (! swallowingPopupClick) juce::Slider::mouseDrag (e);
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (! swallowingPopupClick) juce::Slider::mouseUp (e);
        swallowingPopupClick = false;
    }

    bool keyPressed (const juce::KeyPress& key) override
    {
        if (key == juce::KeyPress::returnKey)
        {
            valueEntry.openEditor();
            return true;
        }
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
    bool swallowingPopupClick = false;

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

// The non-const form also tells a Dial's ValueEntry which parameter to send gestures to.
inline void setDoubleClickDefault (juce::Slider& slider, juce::RangedAudioParameter& param)
{
    setDoubleClickDefault (slider, static_cast<const juce::RangedAudioParameter&> (param));
    if (auto* dial = dynamic_cast<Dial*> (&slider))
        dial->valueEntry.setParameter (&param);
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

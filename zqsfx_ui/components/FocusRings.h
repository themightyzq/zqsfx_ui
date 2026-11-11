#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// Keyboard focus rings for every focusable control.
//
// juce::Desktop asks the LookAndFeel for a focus outline (zqsfx::ui::LookAndFeel draws the house
// accent ring) only for components that opted in with setHasFocusOutline (true). JUCE's own
// buttons, combo boxes and toggles never opt in, so without this a keyboard user tabbing through
// an editor sees nothing on them. Call enableFocusRings (editor) at the end of an editor's
// constructor, and again after building controls at run time.

namespace zqsfx::ui
{
// A component that draws its own focus indication (Knob's ring around the dial face, the logo
// mark lighting up) carries this property so enableFocusRings leaves it alone.
inline const juce::Identifier ownFocusRingProperty { "zqsfxOwnFocusRing" };

inline void markOwnFocusRing (juce::Component& c)
{
    c.getProperties().set (ownFocusRingProperty, true);
    c.setHasFocusOutline (false);
}

// Opts every component under root into the house focus ring. The ring appears only while a
// component has keyboard focus, so opting in a label or panel that never takes focus changes
// nothing. Every component is opted in, not just those that want focus now, because JUCE reports
// a disabled control as not wanting focus: a button that starts disabled would otherwise never
// get its ring once enabled. root itself is skipped: an editor or window takes focus only to
// catch keyboard shortcuts, and a ring around the whole window would mark nothing.
inline void enableFocusRings (juce::Component& root)
{
    for (auto* child : root.getChildren())
    {
        if (! child->getProperties().contains (ownFocusRingProperty))
            child->setHasFocusOutline (true);
        enableFocusRings (*child);
    }
}
} // namespace zqsfx::ui

// FocusRingTests: enableFocusRings opts focusable controls into the house focus ring, skips the
// root and anything marked as drawing its own focus. Exit 0 on pass, 1 on any failure.

#include <iostream>
#include <zqsfx_ui/zqsfx_ui.h>

namespace
{
int failures = 0;
void check (bool ok, const char* what)
{
    std::cout << (ok ? "PASS " : "FAIL ") << what << "\n";
    if (! ok) ++failures;
}
} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI gui;

    juce::Component root, panel;
    juce::TextButton button ("Play");
    juce::ComboBox combo;
    juce::ToggleButton toggle ("Loop");
    juce::Label label; // takes no keyboard focus
    zqsfx::ui::Dial ownRing;
    zqsfx::ui::markOwnFocusRing (ownRing);
    zqsfx::ui::LogoMark logo ("Test");

    root.setWantsKeyboardFocus (true); // like an editor catching shortcuts
    combo.setWantsKeyboardFocus (true);
    root.addAndMakeVisible (panel);
    panel.addAndMakeVisible (button);
    panel.addAndMakeVisible (combo);
    panel.addAndMakeVisible (toggle);
    panel.addAndMakeVisible (label);
    panel.addAndMakeVisible (ownRing);
    root.addAndMakeVisible (logo);

    check (! button.hasFocusOutline() && ! combo.hasFocusOutline() && ! toggle.hasFocusOutline(),
           "JUCE buttons, combo boxes and toggles start without a focus ring (why this helper exists)");

    zqsfx::ui::enableFocusRings (root);

    check (button.hasFocusOutline(), "a TextButton nested one level down gets the ring");
    check (combo.hasFocusOutline(), "a ComboBox gets the ring");
    check (toggle.hasFocusOutline(), "a ToggleButton gets the ring");
    check (! label.hasFocusOutline(), "a Label, which takes no focus, is left alone");
    check (! root.hasFocusOutline(), "the root (an editor catching shortcuts) gets no ring around itself");
    check (! ownRing.hasFocusOutline(), "a control marked with markOwnFocusRing keeps its own focus drawing");
    check (! logo.hasFocusOutline(), "the logo mark keeps its own focus drawing (it lights up)");

    std::cout << failures << " failure(s)\n";
    return failures == 0 ? 0 : 1;
}

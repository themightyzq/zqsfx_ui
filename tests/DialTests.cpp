// DialTests: zqsfx::ui::Dial keyboard behaviour and setDoubleClickDefault.
// Exit 0 on pass, 1 on any failure.

#include <cmath>
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
bool near (double a, double b) { return std::abs (a - b) < 1e-9; }

juce::KeyPress key (int code, int mods = 0) { return juce::KeyPress (code, juce::ModifierKeys (mods), 0); }
} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI gui;

    {
        zqsfx::ui::Dial d;
        d.setRange (0.0, 100.0, 1.0);
        d.setValue (50.0, juce::dontSendNotification);
        check (d.getWantsKeyboardFocus(), "Dial wants keyboard focus");
        check (d.hasFocusOutline(), "Dial shows a focus outline by default");

        d.keyPressed (key (juce::KeyPress::rightKey));
        check (near (d.getValue(), 51.0), "plain Right steps by the interval (50 -> 51)");
        d.keyPressed (key (juce::KeyPress::rightKey, juce::ModifierKeys::shiftModifier));
        check (near (d.getValue(), 52.0), "stepped: Shift+Right moves one interval, not a snapped-away tenth (51 -> 52)");
        check (near (d.getFineStep(), 1.0), "stepped: fine step is the interval");
    }

    {
        zqsfx::ui::Dial d; // continuous: the plain step is 1 % of the range
        d.setRange (0.0, 10.0, 0.0);
        d.setValue (5.0, juce::dontSendNotification);
        d.keyPressed (key (juce::KeyPress::upKey, juce::ModifierKeys::shiftModifier));
        check (near (d.getValue(), 5.01), "continuous: Shift+Up adds 0.1 % of the range (5 -> 5.01)");
        d.keyPressed (key (juce::KeyPress::downKey, juce::ModifierKeys::shiftModifier));
        d.keyPressed (key (juce::KeyPress::downKey, juce::ModifierKeys::shiftModifier));
        check (near (d.getValue(), 4.99), "continuous: Shift+Down subtracts the same step");
        const bool handled = d.keyPressed (key (juce::KeyPress::rightKey, juce::ModifierKeys::shiftModifier | juce::ModifierKeys::commandModifier));
        check (near (d.getValue(), 4.99) || ! handled, "Shift+Cmd+arrow is not treated as a fine step");
    }

    {
        juce::AudioParameterFloat p (juce::ParameterID { "gain", 1 }, "Gain",
                                     juce::NormalisableRange<float> (-60.0f, 12.0f), -6.0f);
        zqsfx::ui::Dial d;
        d.setRange (-60.0, 12.0, 0.0);
        zqsfx::ui::setDoubleClickDefault (d, p);
        check (d.isDoubleClickReturnEnabled(), "setDoubleClickDefault enables double-click return");
        check (std::abs (d.getDoubleClickReturnValue() - (-6.0)) < 1e-4, "double-click returns to the parameter default (-6)");

        zqsfx::ui::setDoubleClickDefault (d, 3.0);
        check (near (d.getDoubleClickReturnValue(), 3.0), "the plain-value overload sets the given default");
    }

    {
        // Knob keeps the Knob::Dial spelling and draws its own ring, so its dial has no outline.
        static_assert (std::is_same_v<zqsfx::ui::Knob::Dial, zqsfx::ui::Dial>, "Knob::Dial alias");
        check (true, "Knob::Dial is zqsfx::ui::Dial");
    }

    std::cout << "DialTests: " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}

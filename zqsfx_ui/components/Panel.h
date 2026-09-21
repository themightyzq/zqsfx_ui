#pragma once
// Titled rack panel: gradient face, hard border, silk title with a hairline rule.
//
// Origin: Broken's (Project_TurboSynth) Theme.h "Block" component, renamed for the
// shared module. Paint is identical; the only change is that the title font is taken
// from the active LookAndFeel when it is a zqsfx::ui::LookAndFeel, so the panel picks
// up the shared silkscreen typeface instead of a fallback system font.

#include <juce_gui_basics/juce_gui_basics.h>
#include "../lookandfeel/LookAndFeel.h"
#include "../tokens/Tokens.h"

namespace zqsfx::ui
{
class Panel : public juce::Component
{
public:
    explicit Panel (juce::String titleIn) : title (std::move (titleIn))
    {
        setInterceptsMouseClicks (false, true); // the panel is chrome; children interact
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setGradientFill (gradients::panel (r));
        g.fillRect (r);
        g.setColour (juce::Colours::white.withAlpha (0.04f)); // inner top highlight
        g.fillRect (r.withHeight (1.0f).translated (0.0f, 1.0f));
        g.setColour (colour::panelBorder);
        g.drawRect (r, 1.0f);

        auto head = getLocalBounds().reduced (8, 0).removeFromTop (20);
        g.setColour (colour::silkTitle);

        if (auto* lnf = dynamic_cast<LookAndFeel*> (&getLookAndFeel()))
            g.setFont (lnf->silkFont (14.0f, true).withExtraKerningFactor (0.27f));
        else
            g.setFont (juce::Font (juce::FontOptions (14.0f, juce::Font::bold)).withExtraKerningFactor (0.27f));

        g.drawText (title.toUpperCase(), head.translated (0, 4), juce::Justification::centredLeft);
        g.setColour (colour::ruleTitle);
        g.fillRect (juce::Rectangle<int> (8, 24, getWidth() - 16, 1));
    }

private:
    juce::String title;
};
} // namespace zqsfx::ui

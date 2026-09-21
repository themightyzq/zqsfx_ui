#pragma once
// The ZQ SFX company mark (inverted triangle enclosing "ZQ"): a quiet maker's mark on
// every product, and also the About-box trigger.
//
// Origin: new to this module (the mark is a company asset, not something Broken
// drew). Path data is docs/brand/zqsfx_logo_path.txt, an even-odd fill SVG path
// lifted unmodified from the owner's brand-guidelines artwork (the shape only, no
// "ZQ SFX" lettering). docs/ZQSFX_UI_STYLE_GUIDE.md section 5 is the spec of record.

#include <juce_gui_basics/juce_gui_basics.h>
#include "../tokens/Tokens.h"

namespace zqsfx::ui
{
class LogoMark : public juce::Button
{
public:
    // productName sets the tooltip, accessible title, and description to
    // "About <productName>". This is a real control (a real About-box trigger, not
    // decoration): consumers wire their About box through onClick.
    explicit LogoMark (const juce::String& productName)
        : juce::Button (productName + " Logo")
    {
        const auto aboutText = "About " + productName;
        setTooltip (aboutText);
        setTitle (aboutText);
        setDescription (aboutText);
        setHelpText (aboutText);
    }

    // The mark's inner letterforms close up below this height (style guide section 5).
    static int minimumHeight() { return geom::logoMinHeightPx; }

    // Builds the path once (a function-local static, initialised on first use) so every
    // instance and every repaint reuses the same parsed geometry instead of re-parsing
    // the SVG path string.
    static juce::Path createPath()
    {
        static const juce::Path cached = []
        {
            juce::Path p = juce::Drawable::parseSVGPath (pathData);
            p.setUsingNonZeroWinding (false); // EVEN-ODD: without this the mark fills solid
            return p;
        }();
        return cached;
    }

    void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool) override
    {
        auto area = getLocalBounds().toFloat();
        // Below minimumHeight() the inner letterforms will not read clearly, but this
        // is a layout problem for the caller to fix, not a reason to refuse to draw
        // (deliberately no jassert/no-op here: a too-small mark still draws, just small).
        auto path = createPath();
        path.applyTransform (path.getTransformToScaleToFit (area, true, juce::Justification::centred));

        const bool lit = shouldDrawButtonAsHighlighted || hasKeyboardFocus (true);
        g.setColour (lit ? colour::logoBright : colour::silkCaption); // never accent
        g.fillPath (path);
    }

private:
    static constexpr const char* pathData =
        "M995.95 802.79l-466.83 -802.79 933.66 0 -466.83 802.79zm369.85 -747l-353.07 0 0 607.16c"
        "117.88,-202.71 235.26,-404.57 353.07,-607.16zm-739.71 0l353.07 0 0 90.41 -133.32 165.83 "
        "72.16 124.09 0 -118.16 61.16 -79.44 0 424.43 -214.39 -368.68 153.23 -177.33 -185.6 0 53.74"
        " 91.55 -43.29 48.09 -116.77 -200.8zm517.94 258.47l-52.34 -90.71 70.97 0 17.22 29.44 79.6 "
        "-136.03 -185.6 0 0 317.19 70.14 -119.88z";
};
} // namespace zqsfx::ui

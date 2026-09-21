#include "LookAndFeel.h"
#include "ZqsfxUiAssets.h"

namespace zqsfx::ui
{
LookAndFeel::LookAndFeel()
{
    silk     = load (ZqsfxUiAssets::BarlowCondensedMedium_ttf,   ZqsfxUiAssets::BarlowCondensedMedium_ttfSize);
    silkBold = load (ZqsfxUiAssets::BarlowCondensedSemiBold_ttf, ZqsfxUiAssets::BarlowCondensedSemiBold_ttfSize);
    lcd      = load (ZqsfxUiAssets::VT323Regular_ttf,            ZqsfxUiAssets::VT323Regular_ttfSize);
    stamp    = load (ZqsfxUiAssets::IBMPlexMonoRegular_ttf,      ZqsfxUiAssets::IBMPlexMonoRegular_ttfSize);

    // the house knobs (CC0, assets/knobs/PROVENANCE.md); a product may replace them with setKnobStrips
    setKnobStripsFromMemory (ZqsfxUiAssets::strip_a_png, ZqsfxUiAssets::strip_a_pngSize,
                             ZqsfxUiAssets::strip_b_png, ZqsfxUiAssets::strip_b_pngSize,
                             ZqsfxUiAssets::strip_c_png, ZqsfxUiAssets::strip_c_pngSize);

    setColour (juce::ResizableWindow::backgroundColourId, colour::chassisMid);
    setColour (juce::Label::textColourId, colour::silkLabel);

    // combos are LCD fields
    setColour (juce::ComboBox::backgroundColourId, colour::lcdBg);
    setColour (juce::ComboBox::outlineColourId, colour::lcdBorder);
    setColour (juce::ComboBox::textColourId, colour::lcdText);
    setColour (juce::ComboBox::arrowColourId, colour::lcdDim);
    setColour (juce::PopupMenu::backgroundColourId, colour::lcdBg);
    setColour (juce::PopupMenu::textColourId, colour::lcdDim);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, colour::lcdBorder);
    setColour (juce::PopupMenu::highlightedTextColourId, colour::lcdText);

    setColour (juce::TextButton::buttonColourId, colour::btnBot);      // gradient drawn below
    setColour (juce::TextButton::buttonOnColourId, colour::accent);
    setColour (juce::TextButton::textColourOffId, colour::btnText);
    setColour (juce::TextButton::textColourOnId, colour::accentInk);

    // slider text boxes are LCD readouts
    setColour (juce::Slider::textBoxTextColourId, colour::lcdText);
    setColour (juce::Slider::textBoxBackgroundColourId, colour::lcdBg);
    setColour (juce::Slider::textBoxOutlineColourId, colour::lcdBorder);

    setColour (juce::TooltipWindow::backgroundColourId, colour::panelBot);
    setColour (juce::TooltipWindow::textColourId, colour::btnText);
    setColour (juce::TooltipWindow::outlineColourId, colour::panelBorder);

    setColour (juce::AlertWindow::backgroundColourId, colour::panelBot);
    setColour (juce::AlertWindow::textColourId, colour::silkLabel);
    setColour (juce::TextEditor::backgroundColourId, colour::lcdBg);
    setColour (juce::TextEditor::textColourId, colour::lcdText);
    setColour (juce::TextEditor::outlineColourId, colour::lcdBorder);
}

// ---- typography -----------------------------------------------------------------------
juce::Font LookAndFeel::silkFont (float px, bool bold) const
{
    auto t = bold ? silkBold : silk;
    return t != nullptr ? juce::Font (juce::FontOptions (t).withPointHeight (px))
                        : juce::Font (juce::FontOptions (px, juce::Font::bold));
}

juce::Font LookAndFeel::lcdFont (float px) const
{
    return lcd != nullptr ? juce::Font (juce::FontOptions (lcd).withPointHeight (px))
                          : juce::Font (juce::FontOptions (px, juce::Font::plain));
}

juce::Font LookAndFeel::stampFont (float px) const
{
    return stamp != nullptr ? juce::Font (juce::FontOptions (stamp).withPointHeight (px))
                            : juce::Font (juce::FontOptions (px, juce::Font::plain));
}

juce::Font LookAndFeel::getLabelFont (juce::Label& l)
{
    // keep whatever size the component chose, but in the silkscreen face
    return silkFont (l.getFont().getHeight() * 0.92f, true).withExtraKerningFactor (0.10f);
}

juce::Font LookAndFeel::getComboBoxFont (juce::ComboBox&)      { return lcdFont (16.0f); }
juce::Font LookAndFeel::getPopupMenuFont()                     { return lcdFont (16.0f); }

juce::Font LookAndFeel::getTextButtonFont (juce::TextButton&, int h)
{
    return silkFont (juce::jmin (14.0f, (float) h * 0.6f), true).withExtraKerningFactor (0.12f);
}

// ---- rotary knob ------------------------------------------------------------------------
void LookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                    float sliderPos, float, float, juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height);
    const float dial = juce::jmin (bounds.getWidth(), bounds.getHeight());
    auto square = bounds.withSizeKeepingCentre (dial, dial);

    const juce::Image& strip = stripFor (slider, dial);
    if (strip.isValid())
    {
        const int fw     = strip.getWidth();
        const int frames = juce::jmax (1, strip.getHeight() / fw);
        const int frame  = juce::jlimit (0, frames - 1,
                                         juce::roundToInt (sliderPos * (float) (frames - 1)));
        g.drawImage (strip, square.toNearestInt().getX(), square.toNearestInt().getY(),
                     (int) dial, (int) dial, 0, frame * fw, fw, fw);
    }
    else
    {
        drawVectorKnob (g, square, sliderPos, slider.isEnabled());
    }
}

void LookAndFeel::drawVectorKnob (juce::Graphics& g, juce::Rectangle<float> square,
                                  float sliderPos, bool enabled)
{
    const auto centre = square.getCentre();
    const float radius = square.getWidth() * 0.5f;

    if (square.getWidth() >= 44.0f)
    {
        // 12-tooth scalloped ring: alternating 30 degree conic segments
        for (int i = 0; i < 12; ++i)
        {
            const float startDeg = (float) i * 30.0f;
            juce::Path segment;
            segment.addPieSegment (square, juce::degreesToRadians (startDeg),
                                    juce::degreesToRadians (startDeg + 30.0f), 0.0f);
            g.setColour ((i % 2 == 0) ? colour::knobTooth : colour::knobGap);
            g.fillPath (segment);
        }
    }
    else
    {
        // small dials: plain ring, no scallops, so the knob stays legible
        g.setColour (colour::knobGap);
        g.fillEllipse (square);
        g.setColour (colour::knobTooth);
        g.drawEllipse (square.reduced (1.0f), 1.0f);
    }

    // cap: radial gradient offset to 35%/28% of the square, covering ~68% of the radius
    const float capRadius = radius * 0.68f;
    const juce::Point<float> capOffset (square.getX() + square.getWidth() * 0.35f,
                                        square.getY() + square.getHeight() * 0.28f);
    juce::ColourGradient capGrad (colour::knobCapHi, capOffset.x, capOffset.y,
                                  colour::knobCapLo, capOffset.x + capRadius, capOffset.y, true);
    capGrad.addColour (0.56, colour::knobCapMid);
    capGrad.addColour (0.66, colour::knobCapLo);
    auto capBounds = juce::Rectangle<float> (capRadius * 2.0f, capRadius * 2.0f).withCentre (centre);
    g.setGradientFill (capGrad);
    g.fillEllipse (capBounds);
    g.setColour (colour::knobBorder);
    g.drawEllipse (capBounds.reduced (1.0f), 2.0f);

    // pointer: 3px bar, ~30% of the radius long, from the top edge of the cap,
    // sweeping -135 .. +135 degrees (min = lower left, max = lower right)
    const float pointerLen = radius * 0.30f;
    const float angle = juce::MathConstants<float>::pi * (-0.75f + 1.5f * sliderPos);
    const juce::Point<float> start (centre.x + capRadius * std::sin (angle),
                                    centre.y - capRadius * std::cos (angle));
    const juce::Point<float> end (centre.x + (capRadius + pointerLen) * std::sin (angle),
                                  centre.y - (capRadius + pointerLen) * std::cos (angle));
    g.setColour (enabled ? colour::pointer : colour::pointer.withAlpha (0.4f));
    g.drawLine ({ start, end }, 3.0f);
}

void LookAndFeel::setKnobStrips (juce::Image xl, juce::Image m, juce::Image s)
{
    stripXL = std::move (xl);
    stripM  = std::move (m);
    stripS  = std::move (s);
}

// Printed tick ring AROUND a knob (silkscreen on the panel, not on the knob).
void LookAndFeel::drawTickRing (juce::Graphics& g, juce::Rectangle<float> dialSquare)
{
    const auto c = dialSquare.getCentre();
    const float r0 = dialSquare.getWidth() * 0.5f + 3.0f;
    const float r1 = r0 + 4.0f;
    g.setColour (colour::tick);
    for (int i = 0; i <= 10; ++i)
    {
        // 11 ticks across the -135..+135 sweep
        const float a = juce::degreesToRadians (-135.0f + 27.0f * (float) i);
        g.drawLine (c.x + r0 * std::sin (a), c.y - r0 * std::cos (a),
                    c.x + r1 * std::sin (a), c.y - r1 * std::cos (a), 1.0f);
    }
}

// ---- the phosphor screen treatment -------------------------------------------------------
void LookAndFeel::drawScreen (juce::Graphics& g, juce::Rectangle<float> r, bool scanlines)
{
    // bezel: 1px hard dark rim + 1px inner shadow so the glass sits BELOW the panel
    g.setColour (colour::screenBezel);
    g.drawRect (r, 1.0f);
    auto glass = r.reduced (1.0f);
    g.setColour (colour::lcdBg);
    g.fillRect (glass);
    // phosphor wash: the tube is faintly alive even where nothing is drawn
    juce::ColourGradient wash (colour::lcdText.withAlpha (0.05f), glass.getCentreX(), glass.getCentreY(),
                               juce::Colours::transparentBlack, glass.getX(), glass.getY(), true);
    g.setGradientFill (wash);
    g.fillRect (glass);
    if (scanlines)
    {
        g.setColour (juce::Colours::black.withAlpha (0.22f));
        for (float y = glass.getY() + 2.0f; y < glass.getBottom(); y += 3.0f)
            g.drawHorizontalLine ((int) y, glass.getX(), glass.getRight());
    }
    // top inner shadow (recessed) + border glow line
    g.setColour (juce::Colours::black.withAlpha (0.40f));
    g.fillRect (glass.withHeight (2.0f));
    g.setColour (colour::lcdBorder);
    g.drawRect (glass, 1.0f);
}

void LookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool,
                                int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<float> (0, 0, (float) width, (float) height);
    drawScreen (g, r, height > 30); // scanlines only where the field is tall enough to carry them
    // caret
    g.setColour (box.isEnabled() ? colour::lcdDim : colour::lcdFaint2);
    g.setFont (lcdFont (12.0f));
    g.drawText (juce::String::fromUTF8 ("\xe2\x96\xbe"),
                juce::Rectangle<int> (width - 18, 0, 14, height), juce::Justification::centred);
}

void LookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (7, 1, box.getWidth() - 26, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
    label.setColour (juce::Label::textColourId, colour::lcdText);
}

// Slider value boxes are Labels parented to their Slider; give them the screen glass
// and glowing text instead of a flat fill.
void LookAndFeel::drawLabel (juce::Graphics& g, juce::Label& l)
{
    if (dynamic_cast<juce::Slider*> (l.getParentComponent()) != nullptr)
    {
        auto r = l.getLocalBounds().toFloat();
        drawScreen (g, r, false); // too small for scanlines
        if (! l.isBeingEdited())
            drawLcdText (g, l.getText(), l.getLocalBounds(), (float) l.getHeight() * 0.92f,
                         juce::Justification::centred);
        return;
    }
    LookAndFeel_V4::drawLabel (g, l);
}

// ---- gradient buttons, hover -> accent legend ---------------------------------------------
void LookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&,
                                        bool, bool isDown)
{
    auto r = b.getLocalBounds().toFloat();
    const bool invert = (bool) b.getProperties()["invertLit"];
    const bool on = b.getToggleState() != invert; // e.g. MONO/POLY: value 0 shows lit
    if (on)
    {
        g.setColour (colour::accent);
        g.fillRect (r);
        // inset bottom shadow on active toggles, per the handoff
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.fillRect (r.withTop (r.getBottom() - 2.0f));
    }
    else
    {
        g.setGradientFill (gradients::button (r, b.isEnabled()));
        g.fillRect (r);
        g.setColour (juce::Colours::white.withAlpha (b.isEnabled() ? 0.07f : 0.0f));
        g.fillRect (r.removeFromTop (1.0f));  // inset top highlight
    }
    g.setColour (colour::btnBorder);
    g.drawRect (b.getLocalBounds().toFloat(), 1.0f);
    if (isDown)
    {
        g.setColour (juce::Colours::black.withAlpha (0.25f));
        g.fillRect (b.getLocalBounds().toFloat());
    }
}

void LookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool isOver, bool)
{
    g.setFont (getTextButtonFont (b, b.getHeight()));
    const bool invert = (bool) b.getProperties()["invertLit"];
    const bool lit = b.getToggleState() != invert;
    const auto col = ! b.isEnabled() ? colour::silkCaption
                   : lit ? colour::accentInk
                   : isOver ? colour::accent
                            : colour::btnText;
    g.setColour (col);
    g.drawText (b.getButtonText(), b.getLocalBounds().reduced (2, 0),
                juce::Justification::centred);
}

void LookAndFeel::drawLcdText (juce::Graphics& g, const juce::String& text, juce::Rectangle<int> area,
                               float px, juce::Justification just, juce::Colour col) const
{
    g.setFont (lcdFont (px));
    // four-pass halo reads as phosphor bloom at these sizes without a GlowEffect's cost
    g.setColour (col.withAlpha (0.16f));
    for (auto d : { juce::Point<int> (1, 0), { -1, 0 }, { 0, 1 }, { 0, -1 } })
        g.drawText (text, area.translated (d.x, d.y), just);
    g.setColour (col);
    g.drawText (text, area, just);
}

// ---- keyboard focus ------------------------------------------------------------------------
std::unique_ptr<juce::FocusOutline> LookAndFeel::createFocusOutlineForComponent (juce::Component&)
{
    // A visible keyboard-focus ring drawn just outside the focused component's bounds
    // (ported from Unravel's CustomLookAndFeel::createFocusOutlineForComponent). Rotary
    // sliders outline their component bounds too; this never tries to hug the dial.
    struct FocusRingProperties final : public juce::FocusOutline::OutlineWindowProperties
    {
        juce::Rectangle<int> getOutlineBounds (juce::Component& c) override
        {
            return c.getScreenBounds().expanded (3);
        }

        void drawOutline (juce::Graphics& g, int width, int height) override
        {
            auto bounds = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height)
                              .reduced (geom::focusRingPx * 0.5f + 1.0f);
            g.setColour (colour::accent);
            g.drawRect (bounds, geom::focusRingPx);
        }
    };

    return std::make_unique<juce::FocusOutline> (std::make_unique<FocusRingProperties>());
}

// ---- private helpers -------------------------------------------------------------------------
juce::Typeface::Ptr LookAndFeel::load (const void* data, int size)
{
    return juce::Typeface::createSystemTypefaceFor (data, (size_t) size);
}

const juce::Image& LookAndFeel::stripFor (juce::Slider& s, float dial) const
{
    const auto prop = s.getProperties()["zqsfxStrip"].toString();
    if (prop == "xl") return stripXL;
    if (prop == "m")  return stripM;
    if (prop == "s")  return stripS;
    if (dial >= 56.0f) return stripXL;   // 66 px primaries
    if (dial >= 42.0f) return stripM;    // 46/44
    return stripS;                       // 40
}
} // namespace zqsfx::ui

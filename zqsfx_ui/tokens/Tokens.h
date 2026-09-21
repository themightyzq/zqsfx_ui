#pragma once
// Shared design tokens for every ZQ SFX product.
//
// Origin: lifted from Broken's (Project_TurboSynth) Theme.h, which remains the visual
// spec of record for the values below (see docs/ZQSFX_UI_STYLE_GUIDE.md sections 2 and 3
// in the workspace this module was extracted from). Values here are byte-identical to
// that file; this module never "improves" a colour. The five legacy compatibility
// aliases Theme.h carried for its own call sites (bg, panel, border, text, textDim) are
// dropped here since nothing in this module depends on them.

#include <array>
#include <juce_graphics/juce_graphics.h>

namespace zqsfx::ui
{
namespace colour
{
    // chassis + panels (gradients built via helpers below; these are the stops)
    inline const juce::Colour chassisTop    { 0xff0e1011 };
    inline const juce::Colour chassisMid    { 0xff0a0b0c };   // at 60 %
    inline const juce::Colour chassisBot    { 0xff0d0f10 };
    inline const juce::Colour panelTop      { 0xff1d2022 };
    inline const juce::Colour panelBot      { 0xff121416 };
    inline const juce::Colour panelBorder   { 0xff060707 };

    // silkscreen text
    inline const juce::Colour silkTitle     { 0xff66b7ae };   // section titles
    inline const juce::Colour silkLabel     { 0xff8fb3ae };   // knob/control labels
    inline const juce::Colour silkCaption   { 0xff7a9a94 };   // sub-captions, disabled text

    // hairlines
    inline const juce::Colour ruleTitle     { 0xff22272a };   // under panel titles
    inline const juce::Colour ruleInner     { 0xff1b1f21 };   // inner dividers

    // LCD / VFD displays (readouts, dropdowns, waveform screens, tuners, meter well)
    inline const juce::Colour lcdBg         { 0xff0c150e };
    inline const juce::Colour lcdBorder     { 0xff1f2b21 };
    inline const juce::Colour lcdText       { 0xff8fe89a };   // primary values (glow drawn by LnF)
    inline const juce::Colour lcdDim        { 0xff63b871 };   // secondary (presets, carets)
    inline const juce::Colour lcdFaint      { 0xff4e8a5a };   // hints, scale numerals
    inline const juce::Colour lcdFaint2     { 0xff3f7a4a };   // needles, ghost strokes
    inline const juce::Colour lcdScreenDark { 0xff0a120c };   // waveform stripes, meter well
    inline const juce::Colour lcdGlow       { 0x805adc6e };   // 0 0 7px glow tint (drawn soft)

    // the recessed hard rim drawn around every phosphor screen (drawScreen's literal)
    inline const juce::Colour screenBezel   { 0xff040605 };

    // buttons
    inline const juce::Colour btnTop        { 0xff212326 };
    inline const juce::Colour btnBot        { 0xff141618 };
    inline const juce::Colour btnBorder     { 0xff0a0b0c };
    inline const juce::Colour btnText       { 0xffc9d4d2 };
    inline const juce::Colour btnText2      { 0xffb9c7c4 };
    inline const juce::Colour btnDisTop     { 0xff17191b };
    inline const juce::Colour btnDisBot     { 0xff101214 };

    // the ONE accent + friends
    inline const juce::Colour accent        { 0xffe8622a };   // LEDs, active toggles, hover text
    inline const juce::Colour accentInk     { 0xff140d07 };   // text ON an accent fill
    inline const juce::Colour accentDim     { 0xff8a4a28 };   // dimmed accent (blink-off state)
    inline const juce::Colour ledOffFill    { 0xff151719 };
    inline const juce::Colour ledOffRim     { 0xff3d4448 };   // bright enough that an OFF lamp is still findable
    inline const juce::Colour recDot        { 0xff5a2018 };   // a REC-style dark-red dot, unlit

    // knob furniture (vector knobs; see LookAndFeel::drawVectorKnob)
    inline const juce::Colour tick          { 0x737abab2 };   // rgba(122,186,178,.45) tick rings + meter scale
    inline const juce::Colour pointer       { 0xffded6c2 };   // knob pointer
    inline const juce::Colour knobTooth     { 0xff2e2e2e };   // scallop ring tooth
    inline const juce::Colour knobGap       { 0xff070707 };   // scallop ring gap
    inline const juce::Colour knobCapHi     { 0xff505050 };   // cap radial gradient, near stop
    inline const juce::Colour knobCapMid    { 0xff242424 };   // cap radial gradient, 56 %
    inline const juce::Colour knobCapLo     { 0xff080808 };   // cap radial gradient, 66 %
    inline const juce::Colour knobBorder    { 0xff040404 };   // cap border, 2 px

    // meter fill stops (bottom -> top)
    inline const juce::Colour meterLo       { 0xff2f8f45 };
    inline const juce::Colour meterMid      { 0xff6fd57e };   // at 70 %
    inline const juce::Colour meterHi       { 0xff8fe89a };
    inline const juce::Colour meterHot      { 0xffd9a441 };   // above -6 dB shifts amber
    inline const juce::Colour meterClip     { 0xffdd4433 };

    inline const juce::Colour logoBright    { 0xffe2e5e8 };
    inline const juce::Colour footer        { 0xff45524f };
    inline const juce::Colour warn          { 0xffdd4433 };
} // namespace colour

// Complementary, colour-blind-safe channels (style guide section 3). Five channels is the
// ceiling; a product that needs more encodes the extra ones by shape, pattern, or position,
// never by a sixth hue.
namespace comp
{
    inline const juce::Colour sky    { 0xff56b4e9 };   // Okabe-Ito sky blue
    inline const juce::Colour yellow { 0xfff0e442 };   // Okabe-Ito yellow
    inline const juce::Colour purple { 0xffcc79a7 };   // Okabe-Ito reddish purple
    inline const juce::Colour green  = colour::lcdText;
    inline const juce::Colour white  = colour::logoBright;

    inline const std::array<juce::Colour, 5> all { sky, yellow, purple, green, white };

    inline juce::Colour channel (int index)
    {
        auto n = index % (int) all.size();
        if (n < 0)
            n += (int) all.size();
        return all[(size_t) n];
    }
} // namespace comp

namespace gradients
{
    inline juce::ColourGradient chassis (juce::Rectangle<float> r)
    {
        juce::ColourGradient g (colour::chassisTop, r.getTopLeft(), colour::chassisBot, r.getBottomLeft(), false);
        g.addColour (0.6, colour::chassisMid);
        return g;
    }
    inline juce::ColourGradient panel (juce::Rectangle<float> r)
    {
        return { colour::panelTop, r.getTopLeft(), colour::panelBot, r.getBottomLeft(), false };
    }
    inline juce::ColourGradient button (juce::Rectangle<float> r, bool enabled)
    {
        return { enabled ? colour::btnTop : colour::btnDisTop, r.getTopLeft(),
                 enabled ? colour::btnBot : colour::btnDisBot, r.getBottomLeft(), false };
    }
} // namespace gradients

namespace geom
{
    // knob dial sizes per the handoff
    constexpr int knobXL    = 66;
    constexpr int knobL     = 46;
    constexpr int knobM     = 44;
    constexpr int knobS     = 40;
    constexpr int lightSize = 10;   // round LEDs, 9-10 px
    constexpr int headerH   = 16;   // panel title strip metric
    constexpr float dimAlpha = 0.30f; // inapplicable controls

    constexpr float focusRingPx    = 2.0f;   // keyboard focus outline stroke width
    constexpr int   minHitTargetPx = 22;     // accessibility floor
    constexpr int   logoMinHeightPx = 24;    // the mark's inner letterforms close up below this
} // namespace geom

// Section title: Barlow Condensed 600 13px ls 3.5, uppercase - the LookAndFeel supplies the
// typeface; these helpers only set colour/justification so they stay usable before the
// font loads (fallback face).
inline void styleSectionLabel (juce::Label& l)
{
    l.setFont (juce::Font (juce::FontOptions (14.0f, juce::Font::bold)));
    l.setColour (juce::Label::textColourId, colour::silkTitle);
    l.setJustificationType (juce::Justification::centredLeft);
}

inline void styleControlLabel (juce::Label& l)
{
    l.setFont (juce::Font (juce::FontOptions (11.5f, juce::Font::bold)));
    l.setColour (juce::Label::textColourId, colour::silkLabel);
    l.setJustificationType (juce::Justification::centred);
}
} // namespace zqsfx::ui

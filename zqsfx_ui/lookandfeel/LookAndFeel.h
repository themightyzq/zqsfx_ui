#pragma once
// The shared ZQ SFX house LookAndFeel.
//
// Origin: ported from Broken's (Project_TurboSynth) TsLookAndFeel.h, which paired a
// filmstrip-image rotary knob with the same LCD/silkscreen/button treatment kept here.
// THE HOUSE KNOBS are three CC0 filmstrips embedded in this module (assets/knobs/, provenance in
// PROVENANCE.md): silver cap in a black lobed skirt for dials 56 px and up, black with a white
// pointer for 42 px and up, brushed silver cap below that. They load by default. A product with
// its own art (Broken keeps its licensed Noisehead strips) replaces them with setKnobStrips() /
// setKnobStripsFromMemory(). If an image ever fails to load, drawVectorKnob is the fallback, so a
// control is never invisible.
// Keyboard focus is drawn via createFocusOutlineForComponent (ported from Unravel's
// CustomLookAndFeel), not by hand in each draw call, per the accessibility floor in
// docs/ZQSFX_UI_STYLE_GUIDE.md section 8.

#include <juce_gui_basics/juce_gui_basics.h>
#include "../tokens/Tokens.h"

namespace zqsfx::ui
{
class LookAndFeel : public juce::LookAndFeel_V4
{
public:
    LookAndFeel();

    // ---- typography -----------------------------------------------------------------
    juce::Font silkFont (float px, bool bold = false) const;
    juce::Font lcdFont (float px) const;
    juce::Font stampFont (float px) const;

    juce::Font getLabelFont (juce::Label& l) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getPopupMenuFont() override;
    juce::Font getTextButtonFont (juce::TextButton&, int h) override;

    // ---- rotary knob ------------------------------------------------------------------
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider& slider) override;

    // Vector knob per the handoff spec (ClaudeDesign/design_handoff_broken_ui/README.md,
    // "Knobs"): 12-tooth scalloped ring, radial-gradient cap, 3px pointer. No wear/chip
    // decoration (that stays with Broken). Dials under 44 px draw a plain ring instead
    // of scallops so small knobs stay legible.
    void drawVectorKnob (juce::Graphics& g, juce::Rectangle<float> square, float sliderPos, bool enabled);

    // Optional filmstrip image knobs so a product (Broken) can keep its own licensed
    // knob art. When a slider's "zqsfxStrip" property or its dial size selects a valid
    // strip, drawRotarySlider blits that filmstrip frame exactly as Broken did;
    // otherwise it falls back to drawVectorKnob.
    void setKnobStrips (juce::Image xl, juce::Image m, juce::Image s);

    // Same, straight from binary data (what zqsfx_ui_add_knob_strips() in CMake generates):
    //   lnf.setKnobStripsFromMemory (ZqsfxKnobStrips::strip_a_png, ZqsfxKnobStrips::strip_a_pngSize, ...b..., ...c...);
    void setKnobStripsFromMemory (const void* xl, int xlSize, const void* m, int mSize, const void* s, int sSize)
    {
        setKnobStrips (juce::ImageCache::getFromMemory (xl, xlSize),
                       juce::ImageCache::getFromMemory (m, mSize),
                       juce::ImageCache::getFromMemory (s, sSize));
    }

    // Printed tick ring AROUND a knob (silkscreen on the panel, not on the knob).
    // Kept here so ring and knob agree; called from Knob::paint with the dial square.
    static void drawTickRing (juce::Graphics& g, juce::Rectangle<float> dialSquare);

    // ---- the phosphor screen treatment -------------------------------------------------
    // One treatment for everything that should read as a screen rather than hardware:
    // dropdowns, value readouts, meters, custom displays.
    static void drawScreen (juce::Graphics& g, juce::Rectangle<float> r, bool scanlines = true);

    void drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH,
                       juce::ComboBox& box) override;
    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override;
    void drawLabel (juce::Graphics& g, juce::Label& l) override;

    // ---- gradient buttons, hover -> accent legend --------------------------------------
    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted, bool isDown) override;
    void drawButtonText (juce::Graphics& g, juce::TextButton& b, bool isOver, bool isDown) override;

    // LCD glow helper (readouts/screens call this for their value text).
    void drawLcdText (juce::Graphics& g, const juce::String& text, juce::Rectangle<int> area,
                      float px, juce::Justification just = juce::Justification::centred,
                      juce::Colour col = colour::lcdText) const;

    // Keyboard focus: a geom::focusRingPx outline in colour::accent around the
    // component's bounds. Rotary sliders get the same treatment as everything else
    // (outline the component bounds; it does not try to hug the dial).
    std::unique_ptr<juce::FocusOutline> createFocusOutlineForComponent (juce::Component& component) override;

private:
    static juce::Typeface::Ptr load (const void* data, int size);
    const juce::Image& stripFor (juce::Slider& s, float dial) const;

    juce::Typeface::Ptr silk, silkBold, lcd, stamp;
    juce::Image stripXL, stripM, stripS;
};
} // namespace zqsfx::ui

#pragma once
// The gallery demo: a minimal AudioProcessor to own an APVTS, and a Component that
// shows one of every zqsfx::ui control against the house palette. Used by both the
// interactive demo app (demo/Main.cpp) and the headless snapshot tool
// (tools/snapshot/Main.cpp).

#include <juce_audio_processors/juce_audio_processors.h>
#include <cmath>
#include <zqsfx_ui/zqsfx_ui.h>

namespace zqsfx::ui
{
// A minimal AudioProcessor whose only job is to own the APVTS the demo controls bind
// to. All pure virtuals are implemented trivially; this is never hosted as a plugin.
class DemoProcessor : public juce::AudioProcessor
{
public:
    DemoProcessor() : apvts (*this, nullptr, "PARAMS", createLayout()) {}

    const juce::String getName() const override { return "ZqsfxUiDemoProcessor"; }
    void prepareToPlay (double, int) override {}
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override { buffer.clear(); }

    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override {}
    void setStateInformation (const void*, int) override {}

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

        auto addFloat = [&params] (const char* id, float defaultValue)
        {
            params.push_back (std::make_unique<juce::AudioParameterFloat> (
                juce::ParameterID { id, 1 }, id, juce::NormalisableRange<float> (0.0f, 1.0f), defaultValue));
        };
        addFloat ("knobXL", 0.70f);
        addFloat ("knobL", 0.30f);
        addFloat ("knobM", 0.55f);
        addFloat ("knobS", 0.85f);
        addFloat ("spare1", 0.50f);
        addFloat ("spare2", 0.50f);

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "choiceParam", 1 }, "choiceParam",
            juce::StringArray { "Alpha", "Beta", "Gamma" }, 0));

        auto addBool = [&params] (const char* id, bool defaultValue)
        {
            params.push_back (std::make_unique<juce::AudioParameterBool> (
                juce::ParameterID { id, 1 }, id, defaultValue));
        };
        addBool ("boolTextOn", true);
        addBool ("boolTextOff", false);
        addBool ("boolLitOn", true);
        addBool ("boolLitOff", false);

        return { params.begin(), params.end() };
    }
};

// A phosphor screen that draws a sine wave and an LCD caption, standing in for a
// scope/tuner/curve display.
class SineScreen : public PhosphorScreen
{
public:
    SineScreen()
    {
        setTitle ("Waveform preview");
        setDescription ("A demo sine wave drawn on a phosphor screen.");
        setAccessible (true);
    }

    void paintContent (juce::Graphics& g, juce::Rectangle<int> glassArea) override
    {
        auto b = glassArea.toFloat().reduced (4.0f, 10.0f);
        juce::Path wave;
        constexpr int steps = 128;
        for (int i = 0; i <= steps; ++i)
        {
            const float t = (float) i / (float) steps;
            const float x = b.getX() + b.getWidth() * t;
            const float y = b.getCentreY() + std::sin (t * juce::MathConstants<float>::twoPi * 2.0f) * b.getHeight() * 0.35f;
            if (i == 0)
                wave.startNewSubPath (x, y);
            else
                wave.lineTo (x, y);
        }
        g.setColour (colour::lcdText);
        g.strokePath (wave, juce::PathStrokeType (1.5f));

        if (auto* lnf = dynamic_cast<LookAndFeel*> (&getLookAndFeel()))
            lnf->drawLcdText (g, "SIG", glassArea.removeFromBottom (16), 12.0f, juce::Justification::centredLeft);
    }
};

// One swatch: a colour, its token name, and a distinct shape marker, per the "never
// colour alone" rule (style guide section 3, rule 2).
class ChannelSwatch : public juce::Component
{
public:
    enum Shape { circle, square, triangle, diamond, cross };

    ChannelSwatch (juce::Colour colourIn, juce::String nameIn, Shape shapeIn)
        : swatchColour (colourIn), tokenName (std::move (nameIn)), shape (shapeIn) {}

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        auto labelArea = b.removeFromBottom (16.0f);
        drawShape (g, b.reduced (6.0f));

        g.setColour (colour::silkCaption);
        // silkscreen face when the house LookAndFeel is active, so the gallery shows the real type
        if (auto* lnf = dynamic_cast<LookAndFeel*> (&getLookAndFeel()))
            g.setFont (lnf->silkFont (12.0f, true).withExtraKerningFactor (0.10f));
        else
            g.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
        g.drawText (tokenName.toUpperCase(), labelArea, juce::Justification::centred);
    }

private:
    void drawShape (juce::Graphics& g, juce::Rectangle<float> area) const
    {
        auto d = juce::jmin (area.getWidth(), area.getHeight());
        auto sq = area.withSizeKeepingCentre (d, d);
        g.setColour (swatchColour);
        switch (shape)
        {
            case circle: g.fillEllipse (sq); break;
            case square: g.fillRect (sq); break;
            case triangle:
            {
                juce::Path p;
                p.addTriangle (sq.getCentreX(), sq.getY(), sq.getRight(), sq.getBottom(), sq.getX(), sq.getBottom());
                g.fillPath (p);
                break;
            }
            case diamond:
            {
                juce::Path p;
                p.startNewSubPath (sq.getCentreX(), sq.getY());
                p.lineTo (sq.getRight(), sq.getCentreY());
                p.lineTo (sq.getCentreX(), sq.getBottom());
                p.lineTo (sq.getX(), sq.getCentreY());
                p.closeSubPath();
                g.fillPath (p);
                break;
            }
            case cross:
            default:
            {
                const float t = juce::jmax (2.0f, sq.getWidth() * 0.2f);
                g.fillRect (sq.withSizeKeepingCentre (sq.getWidth(), t));
                g.fillRect (sq.withSizeKeepingCentre (t, sq.getHeight()));
                break;
            }
        }
    }

    juce::Colour swatchColour;
    juce::String tokenName;
    Shape shape;
};

// One thin lane per complementary channel, with no knob and no shape - just the
// "lane order" positional signal the style guide allows alongside colour.
class LaneStripRow : public juce::Component
{
public:
    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        const float laneW = b.getWidth() / (float) comp::all.size();
        for (auto& c : comp::all)
        {
            auto lane = b.removeFromLeft (laneW).reduced (2.0f, 0.0f);
            g.setColour (c);
            g.fillRect (lane);
        }
    }
};

class DemoComponent : public juce::Component
{
public:
    explicit DemoComponent (juce::AudioProcessorValueTreeState& apvtsIn)
        : apvts (apvtsIn),
          logo ("ZQ SFX UI Gallery"),
          controlsPanel ("CONTROLS"),
          screensPanel ("SCREENS"),
          channelsPanel ("COMPLEMENTARY CHANNELS"),
          knobXL (apvts, "knobXL", "DRIVE", "Drive amount.", true),
          knobL (apvts, "knobL", "MOD", "Modulation amount.", false),
          knobM (apvts, "knobM", "FILTER", "Filter cutoff.", false),
          knobS (apvts, "knobS", "RES", "Resonance.", false),
          combo (apvts, "choiceParam", { "Alpha", "Beta", "Gamma" }, "MODE", "Operating mode."),
          toggleOn (apvts, "boolTextOn", "ON", "Always-on demo toggle."),
          toggleOff (apvts, "boolTextOff", "OFF", "Always-off demo toggle."),
          litOn (apvts, "boolLitOn", "Lit demo toggle."),
          litOff (apvts, "boolLitOff", "Unlit demo toggle.")
    {
        addAndMakeVisible (logo);
        addAndMakeVisible (controlsPanel);
        addAndMakeVisible (screensPanel);
        addAndMakeVisible (channelsPanel);

        controlsPanel.addAndMakeVisible (knobXL);
        controlsPanel.addAndMakeVisible (knobL);
        controlsPanel.addAndMakeVisible (knobM);
        controlsPanel.addAndMakeVisible (knobS);
        controlsPanel.addAndMakeVisible (combo);
        controlsPanel.addAndMakeVisible (toggleOn);
        controlsPanel.addAndMakeVisible (toggleOff);
        disabledButton.setButtonText ("N/A");
        disabledButton.setEnabled (false);
        disabledButton.setTitle ("Unavailable control");
        controlsPanel.addAndMakeVisible (disabledButton);
        controlsPanel.addAndMakeVisible (litOn);
        controlsPanel.addAndMakeVisible (litOff);
        // LitToggle's lamp reflects its bound APVTS bool parameter (boolLitOn/boolLitOff
        // default to true/false respectively), not a plain settable field like Led.
        ledOn.on = true;
        ledOff.on = false;
        controlsPanel.addAndMakeVisible (ledOn);
        controlsPanel.addAndMakeVisible (ledOff);

        screensPanel.addAndMakeVisible (sineScreen);
        meter.setLevelDb (-9.0f);
        screensPanel.addAndMakeVisible (meter);

        static const std::array<const char*, 5> names { "sky", "yellow", "purple", "green", "white" };
        static const std::array<ChannelSwatch::Shape, 5> shapes {
            ChannelSwatch::circle, ChannelSwatch::square, ChannelSwatch::triangle,
            ChannelSwatch::diamond, ChannelSwatch::cross
        };
        for (size_t i = 0; i < comp::all.size(); ++i)
        {
            swatches[i] = std::make_unique<ChannelSwatch> (comp::all[i], juce::String (names[i]), shapes[i]);
            channelsPanel.addAndMakeVisible (*swatches[i]);
        }
        channelsPanel.addAndMakeVisible (laneStrip);

        // setSize is last: it synchronously triggers resized(), which lays out every
        // member above (including the swatches array), so they must all exist first.
        setSize (1100, 720);
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setGradientFill (gradients::chassis (r));
        g.fillRect (r);

        auto header = getLocalBounds().reduced (pad).removeFromTop (headerHeight);
        header.removeFromRight (220); // leave room for the logo
        if (auto* lnf = dynamic_cast<LookAndFeel*> (&getLookAndFeel()))
            g.setFont (lnf->silkFont (20.0f, true).withExtraKerningFactor (0.18f));
        else
            g.setFont (juce::Font (juce::FontOptions (20.0f, juce::Font::bold)));
        g.setColour (colour::silkTitle);
        g.drawText ("ZQ SFX UI GALLERY", header, juce::Justification::centredLeft);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (pad);

        auto header = area.removeFromTop (headerHeight);
        auto logoArea = header.removeFromRight (220);
        logo.setBounds (logoArea.withSizeKeepingCentre (160, juce::jmax (LogoMark::minimumHeight(), 32)));
        area.removeFromTop (gap);

        auto topRow = area.removeFromTop (360);
        auto controlsArea = topRow.removeFromLeft (660);
        topRow.removeFromLeft (gap);
        auto screensArea = topRow;
        controlsPanel.setBounds (controlsArea);
        screensPanel.setBounds (screensArea);

        area.removeFromTop (gap);
        channelsPanel.setBounds (area);

        layoutControls();
        layoutScreens();
        layoutChannels();
    }

    // Used by the snapshot tool to render a "gallery_disabled.png" companion image.
    void setAllControlsActive (bool active)
    {
        knobXL.setActive (active);
        knobL.setActive (active);
        knobM.setActive (active);
        knobS.setActive (active);
        combo.setActive (active);
        toggleOn.setEnabled (active);
        toggleOff.setEnabled (active);
        litOn.setEnabled (active);
        litOff.setEnabled (active);
    }

private:
    void layoutControls()
    {
        auto b = controlsPanel.getLocalBounds().reduced (12);
        b.removeFromTop (16); // clear the panel's title/hairline

        // Exact dial sizes matter here (66/46/44/40 px), so each Knob gets bounds sized
        // precisely: 13 px label + the dial + (for the one with a readout) 16 px textbox,
        // rather than dividing a row evenly, which would size the dial to whatever
        // remains instead of the required px value.
        auto knobRow = b.removeFromTop (13 + 66 + 16 + 4);
        int x = knobRow.getX();
        auto placeKnob = [&] (Knob& k, int size, bool withTextBox)
        {
            const int h = 13 + size + (withTextBox ? 16 : 0);
            k.setBounds (x, knobRow.getY(), size, h);
            x += size + 18;
        };
        placeKnob (knobXL, 66, true);
        placeKnob (knobL, 46, false);
        placeKnob (knobM, 44, false);
        placeKnob (knobS, 40, false);

        b.removeFromTop (10);
        auto comboRow = b.removeFromTop (13 + 26);   // 13 px label + a 26 px LCD field (style guide: 22 to 26)
        combo.setBounds (comboRow.removeFromLeft (200));

        b.removeFromTop (10);
        auto toggleRow = b.removeFromTop (30);
        toggleOn.setBounds (toggleRow.removeFromLeft (90));
        toggleRow.removeFromLeft (8);
        toggleOff.setBounds (toggleRow.removeFromLeft (90));
        toggleRow.removeFromLeft (8);
        disabledButton.setBounds (toggleRow.removeFromLeft (90));

        b.removeFromTop (10);
        auto litRow = b.removeFromTop (30);
        litOn.setBounds (litRow.removeFromLeft (24));
        litRow.removeFromLeft (10);
        litOff.setBounds (litRow.removeFromLeft (24));
        litRow.removeFromLeft (24);
        ledOn.setBounds (litRow.removeFromLeft (70));
        litRow.removeFromLeft (10);
        ledOff.setBounds (litRow.removeFromLeft (70));
    }

    void layoutScreens()
    {
        auto b = screensPanel.getLocalBounds().reduced (12);
        b.removeFromTop (16);
        sineScreen.setBounds (b.removeFromTop (170));
        b.removeFromTop (12);
        meter.setBounds (b.removeFromLeft (16).withHeight (juce::jmin (b.getHeight(), 130)));
    }

    void layoutChannels()
    {
        auto b = channelsPanel.getLocalBounds().reduced (12);
        b.removeFromTop (16);
        auto swatchRow = b.removeFromTop (90);
        const int w = swatchRow.getWidth() / (int) swatches.size();
        for (auto& s : swatches)
            s->setBounds (swatchRow.removeFromLeft (w).reduced (8));
        b.removeFromTop (10);
        laneStrip.setBounds (b.removeFromTop (24));
    }

    static constexpr int pad = 18;
    static constexpr int gap = 10;
    static constexpr int headerHeight = 48;

    juce::AudioProcessorValueTreeState& apvts;

    LogoMark logo;
    Panel controlsPanel, screensPanel, channelsPanel;

    Knob knobXL, knobL, knobM, knobS;
    Combo combo;
    TextToggle toggleOn, toggleOff;
    juce::TextButton disabledButton;
    LitToggle litOn, litOff;
    Led ledOn, ledOff;

    SineScreen sineScreen;
    PeakMeter meter;

    std::array<std::unique_ptr<ChannelSwatch>, 5> swatches;
    LaneStripRow laneStrip;
};
} // namespace zqsfx::ui

#pragma once
// Vertical output peak meter. Range -60..+6 dB, shifts to meterHot above -6 dB.
//
// Origin: ported from Broken's (Project_TurboSynth) PeakMeter.h, made product-agnostic:
// the original polled a TurboSynthProcessor field on a Timer; this version has no
// processor dependency and no timer of its own. The owner feeds it a level (from its
// own audio-thread-to-UI relay, however that product does it) via setLevelDb().

#include <juce_gui_basics/juce_gui_basics.h>
#include "../tokens/Tokens.h"

namespace zqsfx::ui
{
class PeakMeter : public juce::Component, public juce::SettableTooltipClient
{
public:
    PeakMeter() { setTooltip ("Output level."); }

    // Push the current level in dBFS. Values outside -60..+6 dB are clamped. Call this
    // from whatever cadence the host product already uses to relay meter data to its UI
    // (a Timer, an AsyncUpdater, a repaint hook) - this component owns none of that.
    void setLevelDb (float db)
    {
        db = juce::jlimit (loDb, hiDb, db);
        if (! juce::approximatelyEqual (db, lastDb))
        {
            lastDb = db;
            repaint();
        }
    }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();

        // Reserve a thin strip on the left for the printed tick scale; the well itself
        // is whatever's left of that.
        auto tickArea = b.removeFromLeft (juce::jmin (3.0f, b.getWidth() * 0.35f));
        auto well = b;

        g.setColour (colour::lcdScreenDark);
        g.fillRect (well);
        g.setColour (colour::lcdBorder);
        g.drawRect (well, 1.0f);

        auto inner = well.reduced (1.0f);

        // small marks at 0/-6/-12/-24/-48 dB, same loDb..hiDb mapping as the fill below
        g.setColour (colour::tick);
        for (float markDb : { 0.0f, -6.0f, -12.0f, -24.0f, -48.0f })
        {
            float markFrac = (markDb - loDb) / (hiDb - loDb);
            float y = well.getBottom() - well.getHeight() * markFrac;
            g.drawHorizontalLine ((int) y, tickArea.getX(), tickArea.getRight());
        }

        float frac = (lastDb - loDb) / (hiDb - loDb);
        float fillH = inner.getHeight() * frac;
        auto fillRect = juce::Rectangle<float> (inner.getX(), inner.getBottom() - fillH,
                                                 inner.getWidth(), fillH);

        float threshFrac = (threshDb - loDb) / (hiDb - loDb);
        float threshY = inner.getBottom() - inner.getHeight() * threshFrac;

        // Gradient stops anchored to the WELL's full range (not the current fill height)
        // so the colour at a given dB position never shifts as the level moves - like a
        // printed gradient behind glass, revealed by the fill.
        juce::ColourGradient grad (colour::meterLo, inner.getBottomLeft(), colour::meterHi, inner.getTopLeft(), false);
        grad.addColour (0.70, colour::meterMid);
        g.setGradientFill (grad);
        g.fillRect (fillRect);

        if (fillRect.getY() < threshY)
        {
            // colour shifts to amber above -6 dB
            g.setColour (colour::meterHot);
            g.fillRect (fillRect.withBottom (threshY));
        }
    }

    void resized() override {}

private:
    static constexpr float loDb = -60.0f, hiDb = 6.0f, threshDb = -6.0f;
    float lastDb = loDb;
};
} // namespace zqsfx::ui

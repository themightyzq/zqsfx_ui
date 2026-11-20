#pragma once
// Vertical output peak meter. Range -60..+6 dB, shifts to meterHot above -6 dB.
//
// Origin: ported from Broken's (Project_TurboSynth) PeakMeter.h, made product-agnostic:
// the original polled a TurboSynthProcessor field on a Timer; this version has no
// processor dependency and no timer of its own. The owner feeds it a level (from its
// own audio-thread-to-UI relay, however that product does it) via setLevelDb().

#include <functional>
#include <optional>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../tokens/Tokens.h"

namespace zqsfx::ui
{
class PeakMeter : public juce::Component, public juce::SettableTooltipClient
{
public:
    // IEC-style "fast" fall, the House default for Products that opt in via setFallDbPerSecond().
    static constexpr float houseFallDbPerSecond = 20.0f;

    // The tooltip names what this meter measures; the accessible title says the same thing, so a
    // screen reader and a hover agree.
    explicit PeakMeter (juce::String tooltip = "Output level.")
    {
        setTooltip (tooltip);
        setTitle (tooltip);
    }

    // 0 (the default) means no fall: the level follows setLevelDb() exactly. Above 0 the bar
    // jumps up at once and falls no faster than this, measured in real time between calls so
    // the ballistics do not depend on how often the owner's timer fires.
    void setFallDbPerSecond (float dbPerSec) { fallDbPerSec = juce::jmax (0.0f, dbPerSec); }

    // While resting the meter draws an empty well: a meter is too thin for drawRestingMark().
    // The owning display's RestClock calls this. Resting also drops the level, so waking
    // starts from the floor instead of a stale bar.
    void setResting (bool shouldRest)
    {
        if (resting == shouldRest)
            return;
        resting = shouldRest;
        if (resting)
        {
            lastDb = loDb;
            haveLastTime = false;
        }
        repaint();
    }

    // Push the current level in dBFS. Values outside -60..+6 dB are clamped. Call this
    // from whatever cadence the host product already uses to relay meter data to its UI
    // (a Timer, an AsyncUpdater, a repaint hook) - this component owns none of that.
    void setLevelDb (float db)
    {
        db = juce::jlimit (loDb, hiDb, db);

        if (fallDbPerSec > 0.0f)
        {
            const double t = nowSeconds();
            if (haveLastTime && db < lastDb)
                db = juce::jmax (db, lastDb - fallDbPerSec * (float) (t - lastTimeSec));
            lastTimeSec = t;
            haveLastTime = true;
        }

        if (! juce::approximatelyEqual (db, lastDb))
        {
            lastDb = db;
            repaint();
        }
    }

    // A solid fill colour instead of the meterLo/Mid/Hi gradient, for a meter whose colour carries
    // meaning (a channel, a category). The meterHot part above -6 dB still draws. std::nullopt
    // returns to the gradient.
    void setFillColour (std::optional<juce::Colour> c)
    {
        if (c == fillColour)
            return;
        fillColour = c;
        repaint();
    }

    // The level as drawn (after any fall), in dBFS.
    float getLevelDb() const noexcept { return lastDb; }

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

        if (resting)
            return;

        float frac = (lastDb - loDb) / (hiDb - loDb);
        float fillH = inner.getHeight() * frac;
        auto fillRect = juce::Rectangle<float> (inner.getX(), inner.getBottom() - fillH,
                                                 inner.getWidth(), fillH);

        float threshFrac = (threshDb - loDb) / (hiDb - loDb);
        float threshY = inner.getBottom() - inner.getHeight() * threshFrac;

        // Gradient stops anchored to the WELL's full range (not the current fill height)
        // so the colour at a given dB position never shifts as the level moves - like a
        // printed gradient behind glass, revealed by the fill.
        if (fillColour.has_value())
            g.setColour (*fillColour);
        else
        {
            juce::ColourGradient grad (colour::meterLo, inner.getBottomLeft(), colour::meterHi, inner.getTopLeft(), false);
            grad.addColour (0.70, colour::meterMid);
            g.setGradientFill (grad);
        }
        g.fillRect (fillRect);

        if (fillRect.getY() < threshY)
        {
            // colour shifts to amber above -6 dB
            g.setColour (colour::meterHot);
            g.fillRect (fillRect.withBottom (threshY));
        }
    }

    void resized() override {}

   #if ZQSFX_UI_TESTING
    void setClockForTesting (std::function<double()> nowSecondsFn) { testClock = std::move (nowSecondsFn); }
   #endif

private:
    double nowSeconds() const
    {
       #if ZQSFX_UI_TESTING
        if (testClock)
            return testClock();
       #endif
        return juce::Time::getMillisecondCounterHiRes() * 0.001;
    }

    static constexpr float loDb = -60.0f, hiDb = 6.0f, threshDb = -6.0f;
    float lastDb = loDb;
    float fallDbPerSec = 0.0f;
    bool resting = false;
    std::optional<juce::Colour> fillColour;
    bool haveLastTime = false;
    double lastTimeSec = 0.0;
   #if ZQSFX_UI_TESTING
    std::function<double()> testClock;
   #endif
};
} // namespace zqsfx::ui

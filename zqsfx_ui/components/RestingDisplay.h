#pragma once
// Resting displays: lets a live-sound display (meter, scope, spectrum, waveform) stop animating
// when nobody can see it or nothing is playing, and draw a quiet company mark instead.
//
//   ActivityFlag    audio thread writes the block peak, message thread reads it.
//   RestClock       decides, four times a second, whether the display should rest.
//   drawRestingMark the dim mark a resting display of live sound shows.
//
// The flag belongs in the processor (it outlives the editor); RestClock holds it by pointer and
// is owned by the display, so destroying the display stops its timer and nothing can call back.
//
// Origin: new to this module.

#include <functional>
#include <optional>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "LogoMark.h"
#include "../audio/ActivityFlag.h"
#include "../tokens/Tokens.h"

namespace zqsfx::ui
{
class RestClock : private juce::Timer
{
public:
    static constexpr float silenceDb = ActivityFlag::silenceDb;
    static constexpr double holdSeconds = 2.0;
    static constexpr int checkHz = 4;

    // flag == nullptr: the display rests only while hidden. Starts awake; the first evaluation is the first timer tick or an explicit poll().
    RestClock (juce::Component& displayToWatch, ActivityFlag* activity, std::function<void (bool resting)> onChangeCallback)
        : display (displayToWatch), descriptionTarget (&displayToWatch), flag (activity), onChange (std::move (onChangeCallback))
    {
        // No poll here: onChange must not run while the owning display is still being constructed.
        lastLoud = now();
        if (flag != nullptr)
            lastCount = flag->loudBlockCount();
        startTimerHz (checkHz);
    }

    ~RestClock() override
    {
        stopTimer();
        // A target other than the display may outlive this clock: give it its own text back.
        if (describedAsSilent && descriptionTarget != nullptr)
            descriptionTarget->setDescription (savedDescription);
    }

    bool isResting() const noexcept { return resting; }

    // Which component's accessible description says "No sound" (default: the watched display).
    // nullptr: no description is ever changed. Changing it while "No sound" is applied puts the
    // old target's own description back first. A target deleted first is simply forgotten.
    void setDescriptionTarget (juce::Component* c)
    {
        if (c == descriptionTarget)
            return;
        if (describedAsSilent && descriptionTarget != nullptr)
            descriptionTarget->setDescription (savedDescription);
        describedAsSilent = false;
        descriptionTarget = c;
    }

    // Call with true on the clock of the display that reads the flag's takePeak() (a meter):
    // exactly one such clock per flag. While resting, each poll drops the held peak unless that
    // poll wakes the display, so on waking takePeak() returns the hit that woke it and nothing
    // older (a peak played while hidden is not shown). Turning it on also drops the peak held
    // before this clock existed: a reopened editor starts clean.
    void setPeakReader (bool isTheMeter)
    {
        peakReader = isTheMeter && flag != nullptr;
        if (peakReader)
            flag->takePeak();
    }

    void poll()
    {
        const double t = now();
        // Take before reading the count: notePeak counts a block before it stores the peak, so a
        // hit is either in `dropped` and counted, or still held. Awake, the meter reads the peak.
        const bool dropping = peakReader && resting;
        const float dropped = dropping ? flag->takePeak() : 0.0f;
        bool loud = true; // no flag: only hiding can rest the display
        if (flag != nullptr)
        {
            const auto count = flag->loudBlockCount();
            loud = count != lastCount;
            lastCount = count;
        }
        if (loud)
            lastLoud = t;

        const bool showing = isShowing();
        const bool silent = t - lastLoud >= holdSeconds;
        const bool shouldRest = ! showing || silent;
        // Waking: give back the hit that woke it, before onChange. Not on the poll that first shows
        // the display: what it took was played while hidden.
        if (dropping && ! shouldRest && wasShowing)
            flag->returnPeak (dropped);
        wasShowing = showing;

        // "No sound" only when silence is the reason; a hidden display leaves its description alone.
        if (shouldRest && showing && ! describedAsSilent && descriptionTarget != nullptr)
        {
            savedDescription = descriptionTarget->getDescription();
            descriptionTarget->setDescription ("No sound");
            describedAsSilent = true;
        }
        else if (! shouldRest && describedAsSilent)
        {
            if (descriptionTarget != nullptr)
                descriptionTarget->setDescription (savedDescription);
            describedAsSilent = false;
        }

        if (shouldRest != resting)
        {
            resting = shouldRest;
            if (onChange)
                onChange (resting);
        }
    }

    // A product's tests build without a message loop and call poll() themselves; these make that
    // deterministic. Always available, not only under ZQSFX_UI_TESTING.
    void setClockForTesting (std::function<double()> nowSeconds) { testClock = std::move (nowSeconds); lastLoud = now(); }
    void setShowingForTesting (std::optional<bool> showing) { testShowing = showing; }

private:
    void timerCallback() override { poll(); }

    double now() const
    {
        if (testClock)
            return testClock();
        return juce::Time::getMillisecondCounterHiRes() * 0.001;
    }

    bool isShowing() const
    {
        if (testShowing.has_value())
            return *testShowing;
        return display.isShowing();
    }

    juce::Component& display;
    juce::Component::SafePointer<juce::Component> descriptionTarget; // goes null if the target is deleted first
    ActivityFlag* flag;
    std::function<void (bool)> onChange;
    bool resting = false;
    bool describedAsSilent = false;
    bool peakReader = false;
    bool wasShowing = true;
    juce::String savedDescription;
    double lastLoud = 0.0;
    uint32_t lastCount = 0;
    std::function<double()> testClock;
    std::optional<bool> testShowing;
};

// The mark, centred, small and dim, for a display of live sound that is resting.
inline void drawRestingMark (juce::Graphics& g, juce::Rectangle<float> area)
{
    const float side = juce::jmin (area.getHeight() * 0.35f, area.getWidth() * 0.35f, 48.0f);
    if (side <= 0.0f)
        return;

    const auto path = LogoMark::createPath();
    const auto box = juce::Rectangle<float> (side, side).withCentre (area.getCentre());
    g.setColour (colour::silkCaption.withAlpha (0.5f));
    g.fillPath (path, path.getTransformToScaleToFit (box, true));
}
} // namespace zqsfx::ui

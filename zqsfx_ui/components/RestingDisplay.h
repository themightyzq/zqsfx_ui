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
        : display (displayToWatch), flag (activity), onChange (std::move (onChangeCallback))
    {
        // No poll here: onChange must not run while the owning display is still being constructed.
        lastLoud = now();
        if (flag != nullptr)
            lastCount = flag->loudBlockCount();
        startTimerHz (checkHz);
    }

    ~RestClock() override { stopTimer(); }

    bool isResting() const noexcept { return resting; }

    void poll()
    {
        const double t = now();
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

        // "No sound" only when silence is the reason; a hidden display leaves its description alone.
        if (shouldRest && showing && ! describedAsSilent)
        {
            savedDescription = display.getDescription();
            display.setDescription ("No sound");
            describedAsSilent = true;
        }
        else if (! shouldRest && describedAsSilent)
        {
            display.setDescription (savedDescription);
            describedAsSilent = false;
        }

        if (shouldRest != resting)
        {
            resting = shouldRest;
            if (onChange)
                onChange (resting);
        }
    }

   #if ZQSFX_UI_TESTING
    void setClockForTesting (std::function<double()> nowSeconds) { testClock = std::move (nowSeconds); lastLoud = now(); }
    void setShowingForTesting (std::optional<bool> showing) { testShowing = showing; }
   #endif

private:
    void timerCallback() override { poll(); }

    double now() const
    {
       #if ZQSFX_UI_TESTING
        if (testClock)
            return testClock();
       #endif
        return juce::Time::getMillisecondCounterHiRes() * 0.001;
    }

    bool isShowing() const
    {
       #if ZQSFX_UI_TESTING
        if (testShowing.has_value())
            return *testShowing;
       #endif
        return display.isShowing();
    }

    juce::Component& display;
    ActivityFlag* flag;
    std::function<void (bool)> onChange;
    bool resting = false;
    bool describedAsSilent = false;
    juce::String savedDescription;
    double lastLoud = 0.0;
    uint32_t lastCount = 0;
   #if ZQSFX_UI_TESTING
    std::function<double()> testClock;
    std::optional<bool> testShowing;
   #endif
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

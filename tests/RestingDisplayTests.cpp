// RestingDisplayTests: ActivityFlag, RestClock and drawRestingMark.
// Exit 0 on pass, 1 on any failure.

// Deliberately first and alone: ActivityFlag.h must compile without zqsfx_ui.h or any GUI header.
#include <zqsfx_ui/audio/ActivityFlag.h>

#include <atomic>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <new>
#include <zqsfx_ui/zqsfx_ui.h>
#include <zqsfx_ui/components/RestingDisplay.h>

// Counting allocator: counts only while a measured window is open.
namespace
{
std::atomic<bool> countingAllocations { false };
std::atomic<long> allocationCount { 0 };

void* countedAlloc (std::size_t n)
{
    if (countingAllocations.load()) ++allocationCount;
    if (void* p = std::malloc (n ? n : 1)) return p;
    throw std::bad_alloc();
}
} // namespace

void* operator new (std::size_t n) { return countedAlloc (n); }
void* operator new[] (std::size_t n) { return countedAlloc (n); }
void operator delete (void* p) noexcept { std::free (p); }
void operator delete[] (void* p) noexcept { std::free (p); }
void operator delete (void* p, std::size_t) noexcept { std::free (p); }
void operator delete[] (void* p, std::size_t) noexcept { std::free (p); }

namespace
{
int failures = 0;
void check (bool ok, const char* what)
{
    std::cout << (ok ? "PASS " : "FAIL ") << what << "\n";
    if (! ok) ++failures;
}
} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI gui;
    using namespace zqsfx::ui;

    {
        ActivityFlag standalone;
        standalone.notePeak (0.5f);
        check (standalone.loudBlockCount() == 1, "ActivityFlag.h alone: notePeak counts a loud peak");
    }

    {
        juce::Component display;
        display.setDescription ("Meter");
        ActivityFlag flag;
        int rests = 0, wakes = 0;
        double now = 0.0;
        RestClock clock (display, &flag, [&] (bool resting) { resting ? ++rests : ++wakes; });
        clock.setClockForTesting ([&] { return now; });

        clock.setShowingForTesting (false);
        clock.poll();
        check (clock.isResting(), "a hidden display rests");

        clock.setShowingForTesting (true);
        flag.notePeak (0.5f);
        clock.poll();
        check (! clock.isResting(), "showing with a loud peak is awake");
        rests = wakes = 0;

        now = 1.0;
        clock.poll();
        check (! clock.isResting(), "1.0 s of silence is still awake");
        now = 2.1;
        clock.poll();
        check (clock.isResting(), "2.1 s of silence rests");
        check (rests == 1 && wakes == 0, "onChange (true) fired exactly once");
        check (display.getDescription() == "No sound", "resting for silence says No sound");
        now = 2.2;
        clock.poll();
        check (rests == 1, "a further silent poll does not fire onChange again");

        now = 2.3;
        flag.notePeak (0.5f);
        clock.poll();
        check (! clock.isResting() && wakes == 1, "a loud peak wakes it, once");
        check (display.getDescription() == "Meter", "waking restores the earlier description");

        now = 10.0;
        flag.notePeak (juce::Decibels::decibelsToGain (-79.0f));
        clock.poll();
        check (! clock.isResting(), "-79 dBFS counts as sound");
        now = 20.0;
        flag.notePeak (juce::Decibels::decibelsToGain (-81.0f));
        clock.poll();
        check (clock.isResting(), "-81 dBFS counts as silence");
    }

    {
        juce::Component display;
        double now = 0.0;
        RestClock clock (display, nullptr, [] (bool) {});
        clock.setClockForTesting ([&] { return now; });
        clock.setShowingForTesting (true);
        now = 100.0;
        clock.poll();
        check (! clock.isResting(), "no flag: showing display never rests for silence");
        clock.setShowingForTesting (false);
        clock.poll();
        check (clock.isResting(), "no flag: hidden display rests");
    }

    {
        ActivityFlag flag;
        juce::AudioBuffer<float> buf (2, 512);
        buf.clear();
        buf.setSample (1, 100, -0.25f);
        flag.noteBlock (buf);
        check (flag.loudBlockCount() == 1, "noteBlock sees a loud sample in channel 1");
        check (flag.loudBlockCount() == 1, "reading the count consumes nothing");
        buf.clear();
        flag.noteBlock (buf);
        check (flag.loudBlockCount() == 1, "a silent block is not counted");

        allocationCount = 0;
        countingAllocations = true;
        for (int i = 0; i < 1000; ++i)
        {
            flag.noteBlock (buf);
            flag.notePeak (0.1f);
        }
        countingAllocations = false;
        check (allocationCount.load() == 0, "noteBlock and notePeak allocate nothing");
    }

    {
        // The constructor must not call back, even for a hidden display.
        juce::Component display;
        int calls = 0;
        RestClock clock (display, nullptr, [&] (bool) { ++calls; });
        check (calls == 0 && ! clock.isResting(), "constructor never calls onChange and starts awake");
        clock.poll();
        check (calls == 1 && clock.isResting(), "the first poll evaluates (hidden rests)");
    }

    {
        // Two displays on one flag both see the same loud block.
        juce::Component d1, d2;
        ActivityFlag flag;
        double now = 0.0;
        RestClock c1 (d1, &flag, [] (bool) {}), c2 (d2, &flag, [] (bool) {});
        c1.setClockForTesting ([&] { return now; });
        c2.setClockForTesting ([&] { return now; });
        c1.setShowingForTesting (true);
        c2.setShowingForTesting (true);
        now = 5.0;
        flag.notePeak (0.5f);
        c1.poll();
        c2.poll();
        check (! c1.isResting() && ! c2.isResting(), "two clocks on one flag are both awake after a loud block");
        now = 8.0;
        c1.poll();
        c2.poll();
        check (c1.isResting() && c2.isResting(), "both rest after the hold with no new loud block");
    }

    {
        // Review Focus 5: a leaked timer would tick into onChange. The clock is awake and showing,
        // and the test clock is past the hold, so any tick that still ran would report a rest.
        juce::Component display;
        ActivityFlag flag;
        int calls = 0;
        double now = 0.0;
        auto clock = std::make_unique<RestClock> (display, &flag, [&] (bool) { ++calls; });
        clock->setClockForTesting ([&] { return now; });
        clock->setShowingForTesting (true);
        flag.notePeak (0.5f);
        clock->poll();
        check (! clock->isResting(), "focus 5: clock is awake before destruction");
        now = RestClock::holdSeconds + 1.0;
        clock.reset();
        const int before = calls;
        juce::MessageManager::getInstance()->runDispatchLoopUntil (600);
        check (calls == before, "focus 5: no callback after the clock is destroyed");
    }

    {
        juce::Image img (juce::Image::ARGB, 200, 100, true);
        juce::Graphics g (img);
        drawRestingMark (g, { 0.0f, 0.0f, 200.0f, 100.0f });
        bool centreInked = false;
        for (int y = 35; y < 65; ++y)
            for (int x = 85; x < 115; ++x)
                centreInked |= img.getPixelAt (x, y).getAlpha() > 0;
        check (centreInked, "drawRestingMark inks the centre");
        check (img.getPixelAt (0, 0).getAlpha() == 0 && img.getPixelAt (199, 0).getAlpha() == 0
                   && img.getPixelAt (0, 99).getAlpha() == 0 && img.getPixelAt (199, 99).getAlpha() == 0,
               "drawRestingMark leaves the corners transparent");
    }

    std::cout << (failures == 0 ? "ALL PASS" : "FAILURES") << "\n";
    return failures == 0 ? 0 : 1;
}

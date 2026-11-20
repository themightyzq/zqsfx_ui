// PeakMeterTests: constructor text, fall ballistics, resting well.
// Exit 0 on pass, 1 on any failure.

#include <iostream>
#include <zqsfx_ui/zqsfx_ui.h>
#include <zqsfx_ui/components/PeakMeter.h>

namespace
{
int failures = 0;
void check (bool ok, const char* what)
{
    std::cout << (ok ? "PASS " : "FAIL ") << what << "\n";
    if (! ok) ++failures;
}
bool near (float a, float b) { return std::abs (a - b) < 0.01f; }
} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI gui;
    using zqsfx::ui::PeakMeter;

    {
        PeakMeter m ("Input level.");
        check (m.getTooltip() == "Input level.", "constructor sets the tooltip");
        check (m.getTitle() == "Input level.", "constructor sets the title to the same text");
    }

    {
        PeakMeter m;
        check (m.getTooltip() == "Output level.", "default meter keeps the tooltip Output level.");
        check (m.getTitle() == "Output level.", "default meter title is Output level.");
    }

    {
        PeakMeter m;
        double now = 0.0;
        m.setClockForTesting ([&] { return now; });
        m.setLevelDb (0.0f);
        now = 0.5;
        m.setLevelDb (-60.0f);
        check (near (m.getLevelDb(), -60.0f), "fall 0: level follows down instantly");
        m.setLevelDb (-12.0f);
        check (near (m.getLevelDb(), -12.0f), "fall 0: level follows up instantly");
    }

    {
        PeakMeter m;
        double now = 0.0;
        m.setClockForTesting ([&] { return now; });
        m.setFallDbPerSecond (20.0f);
        m.setLevelDb (0.0f);
        check (near (m.getLevelDb(), 0.0f), "fall 20: a rise is instant");
        now = 0.5;
        m.setLevelDb (-60.0f);
        check (near (m.getLevelDb(), -10.0f), "fall 20: 0 dB then -60 dB 0.5 s later reads -10 dB");
        now = 0.6;
        m.setLevelDb (-5.0f);
        check (near (m.getLevelDb(), -5.0f), "fall 20: a rise after a fall is instant");
        now = 0.7;
        m.setLevelDb (-6.0f);
        check (near (m.getLevelDb(), -6.0f), "fall 20: a small drop within the allowed fall lands exactly");
        check (PeakMeter::houseFallDbPerSecond == 20.0f, "house fall default is 20 dB/s");
    }

    {
        const auto wellPixel = zqsfx::ui::colour::lcdScreenDark;
        const int x = 10, y = 100; // inside the bar when the level is 0 dB

        auto render = [] (PeakMeter& m)
        {
            m.setBounds (0, 0, 16, 130);
            juce::Image img (juce::Image::ARGB, 16, 130, true);
            juce::Graphics g (img);
            m.paint (g);
            return img;
        };

        PeakMeter live;
        live.setLevelDb (0.0f);
        check (render (live).getPixelAt (x, y) != wellPixel, "control: a live 0 dB meter draws a bar pixel");

        PeakMeter m;
        m.setLevelDb (0.0f);
        m.setResting (true);
        check (render (m).getPixelAt (x, y) == wellPixel, "resting: no bar, the pixel is the empty well");
        m.setResting (false);
        m.setLevelDb (0.0f);
        check (render (m).getPixelAt (x, y) != wellPixel, "waking draws the bar again");
    }

    {
        PeakMeter m;
        m.setBounds (0, 0, 16, 130);
        m.setLevelDb (-20.0f);
        // -20 dB: bar top is well below the -6 dB threshold; sample inside the fill near the bottom.
        auto pixel = [&]
        {
            juce::Image img (juce::Image::ARGB, 16, 130, true);
            juce::Graphics g (img);
            m.paint (g);
            return img.getPixelAt (10, 120);
        };
        m.setFillColour (juce::Colours::purple);
        check (pixel() == juce::Colours::purple, "fill colour: a fill pixel below -6 dB is the solid colour");
        m.setFillColour (std::nullopt);
        check (pixel() != juce::Colours::purple, "fill colour: nullopt returns to the gradient");
    }

    std::cout << (failures == 0 ? "ALL PASS" : "FAILURES") << "\n";
    return failures == 0 ? 0 : 1;
}

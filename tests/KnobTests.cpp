// KnobTests: the small-knob readout and typing it back (parseReadoutText, Knob's parser).
// Exit 0 on pass, 1 on any failure.

#include <cmath>
#include <iostream>
#include <zqsfx_ui/zqsfx_ui.h>

namespace
{
int failures = 0;
void check (bool ok, const char* what)
{
    std::cout << (ok ? "PASS " : "FAIL ") << what << "\n";
    if (! ok) ++failures;
}
bool near (double a, double b) { return std::abs (a - b) < 1e-3; }

struct Proc : juce::AudioProcessor
{
    Proc() : AudioProcessor (BusesProperties()), apvts (*this, nullptr, "P", layout()) {}
    static juce::AudioProcessorValueTreeState::ParameterLayout layout()
    {
        juce::AudioProcessorValueTreeState::ParameterLayout l;
        l.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "mix", 1 }, "Mix",
                   juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));
        l.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "freq", 1 }, "Freq",
                   juce::NormalisableRange<float> (20.0f, 20000.0f), 1000.0f));
        return l;
    }
    const juce::String getName() const override { return "P"; }
    void prepareToPlay (double, int) override {}
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
    double getTailLengthSeconds() const override { return 0; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override {}
    void setStateInformation (const void*, int) override {}
    juce::AudioProcessorValueTreeState apvts;
};
} // namespace

int main()
{
    std::cout << std::unitbuf;
    juce::ScopedJuceInitialiser_GUI gui;
    using zqsfx::ui::parseReadoutText;
    using R = juce::Range<double>;

    check (near (parseReadoutText ("50 %", R (0.0, 1.0)), 0.5), "parse: '50 %' on 0..1 is 0.5");
    check (near (parseReadoutText ("50 %", R (0.0, 100.0)), 50.0), "parse: '50 %' on 0..100 is 50");
    check (near (parseReadoutText ("10.3 kHz", R (20.0, 20000.0)), 10300.0), "parse: '10.3 kHz' is 10300");
    check (near (parseReadoutText ("2k", R (20.0, 20000.0)), 2000.0), "parse: '2k' is 2000");
    check (near (parseReadoutText ("440 Hz", R (20.0, 20000.0)), 440.0), "parse: '440 Hz' is 440");
    check (near (parseReadoutText ("-6 dB", R (-60.0, 6.0)), -6.0), "parse: '-6 dB' is -6");
    check (near (parseReadoutText ("-inf", R (-60.0, 6.0)), -60.0), "parse: '-inf' is the range start");
    check (near (parseReadoutText ("-INF dB", R (-60.0, 6.0)), -60.0), "parse: '-INF dB' is the range start");
    check (near (parseReadoutText ("abc", R (-60.0, 6.0)), -60.0), "parse: no digits is the range start");
    check (near (parseReadoutText ("10,5", R (-60.0, 6.0)), 10.5), "parse: '10,5' is a decimal comma, 10.5");
    check (near (parseReadoutText ("1,000,000", R (0.0, 2e6)), 1e6), "parse: '1,000,000' is one million");
    check (near (parseReadoutText ("- 5", R (-60.0, 6.0)), -5.0), "parse: '- 5' keeps its sign");
    check (near (parseReadoutText ("+3 dB", R (-60.0, 6.0)), 3.0), "parse: '+3 dB' is 3");
    check (near (parseReadoutText ("5-10", R (0.0, 20.0)), 5.0), "parse: '5-10' is 5");
    check (near (parseReadoutText ("Tonal 50", R (0.0, 100.0)), 50.0), "parse: a label before the number is skipped");
    check (near (parseReadoutText ("x2.0", R (0.0, 4.0)), 2.0), "parse: 'x2.0' is 2");
    check (near (parseReadoutText ("1,000 Hz", R (20.0, 20000.0)), 1000.0), "parse: '1,000 Hz' is 1000");
    check (near (parseReadoutText ("L 30 %", R (-1.0, 1.0)), 0.3), "parse: 'L 30 %' reads the number (side needs its own parse)");
    check (near (parseReadoutText ("-.5", R (-1.0, 1.0)), -0.5), "parse: '-.5' is -0.5");

    {
        Proc proc;
        zqsfx::ui::Knob knob (proc.apvts, "mix", "Mix", "Mix", false, true, {},
                              [] (double v) { return juce::String (juce::roundToInt (v * 100.0)) + " %"; });
        bool allSame = true;
        for (double v : { 0.0, 0.1, 0.25, 0.5, 0.73, 1.0 })
        {
            knob.slider.setValue (v, juce::dontSendNotification);
            const auto text = knob.slider.getTextFromValue (v);
            const auto back = knob.slider.getValueFromText (text);
            const bool same = knob.slider.getTextFromValue (back) == text;
            std::cout << "  " << text << " -> " << back << "\n";
            allSame = allSame && same;
        }
        check (allSame, "knob 0..1 shown as %: typing the readout back reads the same value");
    }

    {
        Proc proc;
        zqsfx::ui::Knob knob (proc.apvts, "freq", "Freq", "Freq", false, true, {},
                              [] (double v) { return v >= 1000.0 ? juce::String (v / 1000.0, 1) + " kHz"
                                                                 : juce::String (juce::roundToInt (v)) + " Hz"; });
        bool allSame = true;
        for (double v : { 20.0, 440.0, 999.0, 1000.0, 5000.0, 10300.0, 20000.0 })
        {
            const auto text = knob.slider.getTextFromValue (v);
            const auto back = knob.slider.getValueFromText (text);
            const bool same = knob.slider.getTextFromValue (back) == text;
            std::cout << "  " << text << " -> " << back << "\n";
            allSame = allSame && same;
        }
        check (allSame, "knob 20..20000 Hz with kHz: typing the readout back reads the same value");
    }

    {
        Proc proc;
        zqsfx::ui::Knob knob (proc.apvts, "freq", "Freq", "Freq", false, true, "Hz");
        check (near (knob.slider.getValueFromText (knob.slider.getTextFromValue (440.0)), 440.0),
               "knob with units: '440 Hz' reads back 440");
    }

    {
        // A parameter already measured in kHz: its plain readout "500 kHz" must type back as 500,
        // not 500000 (the custom-format reader's k rule must not reach the units path).
        Proc proc;
        zqsfx::ui::Knob knob (proc.apvts, "freq", "Freq", "Freq", false, true, "kHz");
        check (near (knob.slider.getValueFromText ("500 kHz"), 500.0), "knob with kHz units: '500 kHz' reads back 500");
    }

    {
        Proc proc;
        zqsfx::ui::Knob knob (proc.apvts, "mix", "Mix", "Mix", false, true, {},
                              [] (double v) { return juce::String (v, 1) + "x"; },
                              [] (const juce::String&) { return 0.25; });
        check (near (knob.slider.getValueFromText ("anything"), 0.25), "knob: an explicit parse argument wins over the default");
    }

    std::cout << "KnobTests: " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}

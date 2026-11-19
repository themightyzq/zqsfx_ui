// ValueEntryTests: typing a value into a knob (ValueEntry, wired into Dial).
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
juce::String utf8 (const char* s) { return juce::String (juce::CharPointer_UTF8 (s)); }

struct Proc : juce::AudioProcessor
{
    Proc() : AudioProcessor (BusesProperties()), apvts (*this, nullptr, "P", layout()) {}
    static juce::AudioProcessorValueTreeState::ParameterLayout layout()
    {
        auto toText = [] (float v, int) { return v >= 1000.0f ? juce::String (v / 1000.0f, 1) + " kHz" : juce::String (v, 0) + " Hz"; };
        auto fromText = [] (const juce::String& t)
        {
            auto n = t.getFloatValue();
            return t.containsIgnoreCase ("k") ? n * 1000.0f : n;
        };
        juce::AudioProcessorValueTreeState::ParameterLayout l;
        l.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "freq", 1 }, "Freq",
                   juce::NormalisableRange<float> (20.0f, 20000.0f), 1000.0f,
                   juce::AudioParameterFloatAttributes().withStringFromValueFunction (toText).withValueFromStringFunction (fromText)));
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

struct GestureLog : juce::AudioProcessorParameter::Listener
{
    int begins = 0, ends = 0;
    void parameterValueChanged (int, float) override {}
    void parameterGestureChanged (int, bool starting) override { (starting ? begins : ends)++; }
};

struct OwnMenuDial : zqsfx::ui::Dial
{
    int hits = 0;
    void mouseDown (const juce::MouseEvent&) override { ++hits; }
};

juce::MouseEvent rightEvent (juce::Component& c, juce::Point<float> p, juce::ModifierKeys extra = {})
{
    auto src = juce::Desktop::getInstance().getMainMouseSource();
    const auto mods = juce::ModifierKeys (juce::ModifierKeys::rightButtonModifier | extra.getRawFlags());
    return juce::MouseEvent (src, p, mods, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, &c, &c,
                             juce::Time::getCurrentTime(), p, juce::Time::getCurrentTime(), 1, false);
}
} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI gui;
    using zqsfx::ui::Dial;
    using zqsfx::ui::ValueEntry;

    {
        Proc proc;
        auto* param = proc.apvts.getParameter ("freq");
        GestureLog log;
        param->addListener (&log);

        juce::Component parent;
        parent.setSize (200, 100);
        Dial dial;
        dial.setTitle ("Cutoff");
        parent.addAndMakeVisible (dial);
        dial.setBounds (60, 20, 60, 60);
        juce::AudioProcessorValueTreeState::SliderAttachment att (proc.apvts, "freq", dial);
        zqsfx::ui::setDoubleClickDefault (dial, proc.apvts, "freq");

        check (dial.valueEntry.applyText ("2 kHz"), "applyText accepts '2 kHz'");
        check (near (dial.getValue(), 2000.0), "'2 kHz' gives 2000");
        check (log.begins == 1 && log.ends == 1, "typing sends exactly one gesture begin and one end");

        dial.valueEntry.applyText ("50000");
        check (near (dial.getValue(), 20000.0), "out-of-range text is limited to the range");

        dial.valueEntry.applyText ("500");
        const bool bad = dial.valueEntry.applyText ("banana");
        check (! bad && near (dial.getValue(), 500.0), "unreadable text returns false and leaves the value");
        const bool empty = dial.valueEntry.applyText ("");
        check (! empty && near (dial.getValue(), 500.0), "empty text returns false and leaves the value");

        dial.valueEntry.resetToDefault();
        check (near (dial.getValue(), 1000.0), "resetToDefault returns to 1000");

        juce::PopupMenu menu;
        dial.valueEntry.addItems (menu);
        std::vector<std::pair<int, juce::String>> items;
        std::vector<bool> enabled;
        for (juce::PopupMenu::MenuItemIterator it (menu); it.next();)
        {
            items.emplace_back (it.getItem().itemID, it.getItem().text);
            enabled.push_back (it.getItem().isEnabled);
        }
        check (items.size() == 2 && items[0].first == 1 && items[1].first == 2, "menu has two items with ids 1 and 2");
        check (items.size() == 2 && items[0].second == utf8 ("Type a value\xe2\x80\xa6") && items[1].second == "Reset to default",
               "menu item texts");
        check (enabled.size() == 2 && enabled[1], "Reset to default is enabled when a default exists");

        check (dial.keyPressed (juce::KeyPress (juce::KeyPress::returnKey)), "Return on a Dial is handled");
        auto* ed = dial.valueEntry.getOpenEditor();
        check (ed != nullptr, "Return opens the editor");
        if (ed != nullptr)
        {
            check (ed->getText() == dial.getTextFromValue (dial.getValue()), "editor starts with the knob's text");
            check (ed->getTitle() == "Type a value for Cutoff", "editor title names the knob");
            ed->setText ("5 kHz", false);
            ed->onReturnKey();
            check (near (dial.getValue(), 5000.0), "Return applies the typed value");
            check (dial.valueEntry.getOpenEditor() == nullptr, "Return closes the editor");
        }

        dial.valueEntry.openEditor();
        dial.valueEntry.openEditor();
        check (dial.valueEntry.getOpenEditor() != nullptr, "opening twice keeps one editor");
        if (auto* e2 = dial.valueEntry.getOpenEditor())
        {
            e2->setText ("100", false);
            e2->onEscapeKey();
        }
        check (near (dial.getValue(), 5000.0) && dial.valueEntry.getOpenEditor() == nullptr, "Escape cancels and closes");

        dial.valueEntry.openEditor();
        if (auto* e3 = dial.valueEntry.getOpenEditor())
        {
            e3->setText ("700", false);
            e3->onFocusLost();
        }
        check (near (dial.getValue(), 700.0) && dial.valueEntry.getOpenEditor() == nullptr, "losing focus applies and closes");

        param->removeListener (&log);
    }

    {
        Dial plain; // no double-click default
        plain.setRange (0.0, 10.0, 0.0);
        juce::PopupMenu menu;
        plain.valueEntry.addItems (menu);
        bool resetEnabled = true;
        for (juce::PopupMenu::MenuItemIterator it (menu); it.next();)
            if (it.getItem().itemID == 2) resetEnabled = it.getItem().isEnabled;
        check (! resetEnabled, "Reset to default is disabled with no default");
    }

    {
        juce::Component parent;
        parent.setSize (200, 100);
        Dial tiny;
        tiny.setRange (0.0, 1.0);
        parent.addAndMakeVisible (tiny);
        tiny.setBounds (170, 35, 30, 30);
        tiny.valueEntry.openEditor();
        auto* ed = tiny.valueEntry.getOpenEditor();
        check (ed != nullptr && ed->getWidth() >= 64 && ed->getHeight() >= 22, "editor is at least 64x22 on a 30 px knob");
        check (ed != nullptr && parent.getLocalBounds().contains (ed->getBounds()), "editor stays inside the parent at the edge");
    }

    {
        // A knob inside a narrow wrapper (Knob holds its Dial in a component about as wide as the
        // dial): the box goes on the window, not the wrapper, so it keeps its full size.
        juce::Component window, wrapper;
        window.setSize (400, 300);
        window.addAndMakeVisible (wrapper);
        wrapper.setBounds (100, 100, 40, 50);
        Dial knob;
        knob.setRange (0.0, 1.0);
        wrapper.addAndMakeVisible (knob);
        knob.setBounds (0, 13, 40, 37);
        knob.valueEntry.openEditor();
        auto* ed = knob.valueEntry.getOpenEditor();
        check (ed != nullptr && ed->getParentComponent() == &window, "editor goes on the window, not the knob's wrapper");
        check (ed != nullptr && ed->getWidth() >= 64 && ed->getHeight() >= 22, "editor keeps 64x22 inside a narrow wrapper");
        const auto knobCentre = window.getLocalArea (&wrapper, knob.getBounds()).getCentre();
        check (ed != nullptr && ed->getBounds().contains (knobCentre), "editor sits over the knob");
        if (ed != nullptr) ed->onEscapeKey();
    }

    {
        // A plain Dial: right-click opens the House menu and does not move the value.
        Dial d;
        d.setRange (0.0, 100.0);
        d.setValue (40.0, juce::dontSendNotification);
        const int before = ValueEntry::menusShownForTesting;
        const auto down = rightEvent (d, { 5.0f, 5.0f });
        d.mouseDown (down);
        d.mouseDrag (rightEvent (d, { 5.0f, 60.0f }));
        d.mouseUp (rightEvent (d, { 5.0f, 60.0f }));
        check (ValueEntry::menusShownForTesting == before + 1, "right-click on a plain Dial opens the House menu");
        check (near (d.getValue(), 40.0), "right-click and drag on a Dial leaves the value");
    }

    {
        // Review Focus 1: a subclass with its own mouseDown keeps its own menu.
        OwnMenuDial d;
        d.setRange (0.0, 1.0);
        const int before = ValueEntry::menusShownForTesting;
        d.mouseDown (rightEvent (d, { 5.0f, 5.0f }));
        check (d.hits == 1 && ValueEntry::menusShownForTesting == before && d.valueEntry.getOpenEditor() == nullptr,
              "a Dial subclass's own mouseDown gets the click and no House menu opens");
    }

    {
        // Review Focus 2: right-click on a plain Slider must not move the value.
        juce::Component parent;
        parent.setSize (200, 100);
        juce::Slider s (juce::Slider::LinearHorizontal, juce::Slider::NoTextBox);
        s.setRange (0.0, 100.0, 0.0);
        s.setValue (25.0, juce::dontSendNotification);
        parent.addAndMakeVisible (s);
        s.setBounds (0, 0, 200, 30);
        ValueEntry ve (s);
        const int before = ValueEntry::menusShownForTesting;
        const auto down = rightEvent (s, { 150.0f, 15.0f });
        const auto drag = rightEvent (s, { 190.0f, 15.0f });
        // A window would deliver each event to the slider and then to its listeners.
        s.mouseDown (down);
        ve.mouseDown (down);
        s.mouseDrag (drag);
        ve.mouseDrag (drag);
        s.mouseUp (drag);
        ve.mouseUp (drag);
        check (near (s.getValue(), 25.0), "right-click and drag on a plain Slider leaves the value");
        check (ValueEntry::menusShownForTesting == before + 1, "the House menu was shown once");
    }

    {
        // Fix 1: disabled controls get no menu and no jump.
        juce::Component parent;
        parent.setSize (200, 100);
        juce::Slider s (juce::Slider::LinearHorizontal, juce::Slider::NoTextBox);
        s.setRange (0.0, 1.0, 0.0);
        s.setValue (0.7, juce::dontSendNotification);
        parent.addAndMakeVisible (s);
        s.setBounds (0, 0, 200, 30);
        ValueEntry ve (s);
        s.setEnabled (false);
        const int before = ValueEntry::menusShownForTesting;
        const auto down = rightEvent (s, { 10.0f, 15.0f });
        s.mouseDown (down);
        ve.mouseDown (down);
        ve.mouseUp (down);
        check (near (s.getValue(), 0.7), "disabled plain slider right-clicked keeps 0.7");
        check (ValueEntry::menusShownForTesting == before, "disabled plain slider shows no menu");

        // JUCE skips the drag start when its own popup menu is enabled: no saved value to restore.
        juce::Slider m (juce::Slider::LinearHorizontal, juce::Slider::NoTextBox);
        m.setRange (0.0, 1.0, 0.0);
        m.setPopupMenuEnabled (true);
        m.setValue (0.7, juce::dontSendNotification);
        parent.addAndMakeVisible (m);
        m.setBounds (0, 40, 200, 30);
        ValueEntry vm (m);
        const auto md = rightEvent (m, { 10.0f, 15.0f });
        m.mouseDown (md);
        vm.mouseDown (md);
        vm.mouseUp (md);
        check (near (m.getValue(), 0.7), "no drag start seen: right-click does not restore a stale value");

        const int beforeDial = ValueEntry::menusShownForTesting;
        Dial d;
        d.setRange (0.0, 1.0);
        d.setEnabled (false);
        d.mouseDown (rightEvent (d, { 5.0f, 5.0f }));
        check (ValueEntry::menusShownForTesting == beforeDial, "disabled Dial shows no menu");
    }

    {
        // Fix 2: no-op changes send no gestures. Fix 6: hasParameter.
        Proc proc;
        auto* param = proc.apvts.getParameter ("freq");
        GestureLog log;
        param->addListener (&log);
        Dial dial;
        juce::AudioProcessorValueTreeState::SliderAttachment att (proc.apvts, "freq", dial);
        check (! dial.valueEntry.hasParameter(), "no parameter before setDoubleClickDefault");
        zqsfx::ui::setDoubleClickDefault (dial, proc.apvts, "freq");
        check (dial.valueEntry.hasParameter(), "parameter set by setDoubleClickDefault");
        dial.setValue (1000.0, juce::sendNotificationSync);
        log.begins = log.ends = 0;
        check (dial.valueEntry.applyText ("1 kHz"), "unchanged text is still readable");
        check (log.begins == 0 && log.ends == 0, "applying unchanged text sends 0 gestures");
        dial.valueEntry.resetToDefault();
        check (log.begins == 0 && log.ends == 0, "reset when already at default sends 0 gestures");
        dial.setValue (300.0, juce::sendNotificationSync);
        log.begins = log.ends = 0;
        dial.valueEntry.resetToDefault();
        check (log.begins == 1 && log.ends == 1 && near (dial.getValue(), 1000.0), "reset from another value sends one gesture");
        param->removeListener (&log);
    }

    {
        // 0.7.1 (Worldizer pilot F2): a Product holding its parameter by const reference picks
        // the const overload; it must still give the Dial its parameter, or typed values reach
        // the host with no gesture and touch-mode automation drops them.
        Proc proc;
        const juce::RangedAudioParameter& constParam = *proc.apvts.getParameter ("freq");
        Dial dial;
        juce::AudioProcessorValueTreeState::SliderAttachment att (proc.apvts, "freq", dial);
        zqsfx::ui::setDoubleClickDefault (dial, constParam);
        check (dial.valueEntry.hasParameter(), "the const-reference form also gives the Dial its parameter");
    }

    {
        // Fix 7: under a real AudioProcessorEditor the box goes on the editor.
        Proc proc;
        struct Ed : juce::AudioProcessorEditor
        {
            explicit Ed (juce::AudioProcessor& p) : AudioProcessorEditor (p) { setSize (400, 300); }
        } ed (proc);
        juce::Component wrapper;
        ed.addAndMakeVisible (wrapper);
        wrapper.setBounds (100, 100, 40, 50);
        Dial knob;
        knob.setRange (0.0, 1.0);
        wrapper.addAndMakeVisible (knob);
        knob.setBounds (0, 13, 40, 37);
        knob.valueEntry.openEditor();
        auto* box = knob.valueEntry.getOpenEditor();
        check (box != nullptr && box->getParentComponent() == &ed, "box's parent is the AudioProcessorEditor");
        if (box != nullptr) box->onEscapeKey();
    }

    // Closed text boxes are deleted on the next message-loop turn. Let those run now: left for
    // shutdown, they are destroyed after JUCE's Desktop, which crashes on Linux.
    juce::MessageManager::getInstance()->runDispatchLoopUntil (100);

    std::cout << "ValueEntryTests: " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}

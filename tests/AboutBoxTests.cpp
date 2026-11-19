// AboutBoxTests: the House About box. delayText wording, what the card shows, one box per host,
// closing (Escape, Close, backdrop) with focus handed back, and the accessibility floor.
// Nothing here opens a browser or reaches the network. Exit 0 on pass, 1 on any failure.

#include <cmath>
#include <iostream>
#include <zqsfx_ui/zqsfx_ui.h>
#include <zqsfx_ui/components/AboutBox.h>

using namespace zqsfx::ui;

namespace
{
int failures = 0;
void check (bool ok, const char* what)
{
    std::cout << (ok ? "PASS " : "FAIL ") << what << "\n";
    if (! ok) ++failures;
}

void pump (int ms) { juce::MessageManager::getInstance()->runDispatchLoopUntil (ms); }

// The smallest AudioProcessor that can report a delay and a sample rate.
struct TestProcessor : juce::AudioProcessor
{
    TestProcessor (int latency, double rate)
    {
        setRateAndBufferSizeDetails (rate, 512);
        setLatencySamples (latency);
    }
    const juce::String getName() const override { return "Test"; }
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
};

int countBoxes (juce::Component& host)
{
    int n = 0;
    for (auto* c : host.getChildren())
        if (dynamic_cast<AboutBox*> (c) != nullptr)
            ++n;
    return n;
}

void collect (juce::Component& c, std::vector<juce::Component*>& out)
{
    for (auto* child : c.getChildren())
    {
        out.push_back (child);
        collect (*child, out);
    }
}

std::vector<juce::Component*> descendants (juce::Component& c)
{
    std::vector<juce::Component*> v;
    collect (c, v);
    return v;
}

bool hasLabel (juce::Component& c, const juce::String& text)
{
    for (auto* d : descendants (c))
        if (auto* l = dynamic_cast<juce::Label*> (d))
            if (l->getText() == text)
                return true;
    return false;
}

bool hasLabelStartingWith (juce::Component& c, const juce::String& prefix)
{
    for (auto* d : descendants (c))
        if (auto* l = dynamic_cast<juce::Label*> (d))
            if (l->getText().startsWith (prefix))
                return true;
    return false;
}

juce::Button* findButton (juce::Component& c, const juce::String& text)
{
    for (auto* d : descendants (c))
        if (auto* b = dynamic_cast<juce::Button*> (d))
            if (b->getButtonText() == text)
                return b;
    return nullptr;
}

AboutInfo makeInfo (juce::AudioProcessor* p, const juce::String& productId = {})
{
    AboutInfo info;
    info.productName = "Unravel";
    info.version = "1.4.0";
    info.credits = { "Fonts: Barlow Condensed, IBM Plex Mono (SIL OFL)", "Knob art: CC0" };
    info.sourceUrl = "https://github.com/themightyzq/unravel";
    info.processor = p;
    info.productId = productId;
    return info;
}

void closeAndWait (AboutBox& box)
{
    box.close();
    pump (100);
}
} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI gui;

    // ---- delayText ---------------------------------------------------------------------
    check (AboutBox::delayText (0, 48000.0) == "Delay: none", "delayText: 0 samples is none");
    check (AboutBox::delayText (0, 0.0) == "Delay: none", "delayText: 0 samples, unknown rate, is none");
    check (AboutBox::delayText (512, 48000.0) == "Delay: 512 samples (10.7 ms at 48 kHz)", "delayText: 512 at 48 kHz");
    check (AboutBox::delayText (512, 44100.0) == "Delay: 512 samples (11.6 ms at 44.1 kHz)", "delayText: 512 at 44.1 kHz");
    check (AboutBox::delayText (1000, 96000.0) == "Delay: 1000 samples (10.4 ms at 96 kHz)", "delayText: 1000 at 96 kHz");
    check (AboutBox::delayText (512, 0.0) == "Delay: 512 samples", "delayText: unknown rate (0 Hz) shows samples only");
    check (AboutBox::delayText (1, 48000.0) == "Delay: 1 sample (0.0 ms at 48 kHz)", "delayText: singular sample");

    // ---- a plugin box ------------------------------------------------------------------
    {
        juce::Component host;
        host.setSize (600, 400);
        juce::Component focusTarget;
        host.addAndMakeVisible (focusTarget);
        TestProcessor proc (512, 48000.0);

        auto& box = AboutBox::show (host, makeInfo (&proc), &focusTarget);
        check (countBoxes (host) == 1, "show adds one AboutBox to the host");
        check (box.getBounds() == host.getLocalBounds(), "the box covers the host");
        check (hasLabel (box, "Delay: 512 samples (10.7 ms at 48 kHz)"), "plugin box shows the delay row");
        check (hasLabel (box, "Unravel"), "shows the product name");
        check (hasLabel (box, "Version 1.4.0"), "shows the version");
        check (hasLabel (box, "Knob art: CC0"), "shows credit lines");
        check (hasLabel (box, "GPL-3.0-or-later"), "shows the licence line");
        check (hasLabel (box, "connect@zq-sfx.com"), "shows the contact email");
        check (findButton (box, "Source code") != nullptr, "shows a Source code link");
        check (findButton (box, "zq-sfx.com") != nullptr, "shows the website link");
        check (box.getCardBounds().getWidth() == 420, "card is 420 wide on a roomy host");

        auto& again = AboutBox::show (host, makeInfo (&proc), &focusTarget);
        check (&again == &box && countBoxes (host) == 1, "show twice on one host keeps one box");

        // Every control meets the house floor.
        bool allOk = true;
        bool anyButton = false;
        for (auto* d : descendants (box))
            if (auto* b = dynamic_cast<juce::Button*> (d))
                if (b->isVisible())
                {
                    anyButton = true;
                    allOk = allOk && b->getTitle().isNotEmpty() && b->getTooltip().isNotEmpty()
                            && b->getHeight() >= geom::minHitTargetPx;
                }
        check (anyButton && allOk, "every visible button has a title, a tooltip and is at least 22 px tall");

        const auto* mark = [&]() -> LogoMark* {
            for (auto* d : descendants (box))
                if (auto* m = dynamic_cast<LogoMark*> (d))
                    return m;
            return nullptr;
        }();
        check (mark != nullptr && mark->getHeight() >= LogoMark::minimumHeight(), "the mark is at least its minimum height");
        bool selfClicks = true, childClicks = true;
        if (mark != nullptr)
            mark->getInterceptsMouseClicks (selfClicks, childClicks);
        check (mark != nullptr && ! selfClicks, "the mark in the card is not clickable");

        // The delay row follows the processor (2 Hz timer).
        proc.setLatencySamples (256);
        pump (1100);
        check (hasLabel (box, "Delay: 256 samples (5.3 ms at 48 kHz)"), "delay row refreshes from the processor");

        // Escape closes; focus goes back to the target (when it is showing).
        AboutBox::lastFocusGrabTarget() = nullptr;
        AboutBox::showingForTesting() = true;
        check (box.keyPressed (juce::KeyPress (juce::KeyPress::escapeKey)), "Escape is handled");
        check (countBoxes (host) == 0, "Escape removes the box from the host at once");
        pump (100);
        check (AboutBox::lastFocusGrabTarget() == &focusTarget, "closing grabs focus for the return target when it is showing");
        AboutBox::showingForTesting() = std::nullopt;
    }
    {
        // A target that is not showing is left alone: no grab is attempted.
        juce::Component host, target;
        host.setSize (600, 400);
        host.addAndMakeVisible (target);
        AboutBox::lastFocusGrabTarget() = nullptr;
        AboutBox::showingForTesting() = false;
        auto& box = AboutBox::show (host, makeInfo (nullptr), &target);
        closeAndWait (box);
        check (AboutBox::lastFocusGrabTarget() == nullptr, "a target that is not showing gets no focus grab");
        AboutBox::showingForTesting() = std::nullopt;
    }
    {
        // Escape pressed while Close has focus: JUCE offers the key up the parent chain.
        juce::Component host;
        host.setSize (600, 400);
        auto& box = AboutBox::show (host, makeInfo (nullptr), nullptr);
        auto* close = findButton (box, "Close");
        const juce::KeyPress esc (juce::KeyPress::escapeKey);
        check (close != nullptr && ! static_cast<juce::Component*> (close)->keyPressed (esc), "the Close button itself does not take Escape");
        for (juce::Component* c = close; c != nullptr; c = c->getParentComponent())
            if (c->keyPressed (esc))
                break;
        check (countBoxes (host) == 0, "Escape from the Close button reaches the box through the parent chain");
        pump (100);
    }

    // ---- Tab stays inside the box ---------------------------------------------------------------
    {
        juce::Component host;
        host.setSize (600, 400);
        juce::TextButton outside ("outside");
        host.addAndMakeVisible (outside);
        auto& box = AboutBox::show (host, makeInfo (nullptr), &outside);
        auto* close = findButton (box, "Close");
        auto traverser = close->createKeyboardFocusTraverser();
        bool allInside = true, sawAny = false;
        for (auto* start : { (juce::Component*) close, (juce::Component*) findButton (box, "Source code") })
        {
            auto* next = traverser->getNextComponent (start);
            auto* prev = traverser->getPreviousComponent (start);
            for (auto* c : { next, prev })
            {
                sawAny = sawAny || c != nullptr;
                allInside = allInside && c != &outside && (c == nullptr || box.isParentOf (c));
            }
        }
        check (sawAny && allInside, "Tab and Shift+Tab never leave the box for the editor behind");
        closeAndWait (box);
    }

    // ---- Close button, backdrop click, close from inside a click ----------------------------
    {
        juce::Component host;
        host.setSize (600, 400);
        auto& box = AboutBox::show (host, makeInfo (nullptr), nullptr);
        check (! hasLabelStartingWith (box, "Delay"), "an app box (no processor) has no delay row");

        auto* close = findButton (box, "Close");
        check (close != nullptr, "has a Close button");
        if (close != nullptr)
            close->triggerClick(); // runs inside the button's own click handler
        pump (100);
        check (countBoxes (host) == 0, "Close removes the box (async delete is safe inside the click)");
    }
    {
        juce::Component host;
        host.setSize (600, 400);
        auto& box = AboutBox::show (host, makeInfo (nullptr), nullptr);
        auto src = juce::Desktop::getInstance().getMainMouseSource();
        auto clickAt = [&] (juce::Point<int> p)
        {
            const auto pf = p.toFloat();
            juce::MouseEvent e (src, pf, juce::ModifierKeys(), 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, &box, &box,
                                juce::Time::getCurrentTime(), pf, juce::Time::getCurrentTime(), 1, false);
            box.mouseDown (e);
        };
        clickAt (box.getCardBounds().getCentre());
        check (countBoxes (host) == 1, "a click inside the card does not close it");
        clickAt ({ 2, 2 });
        check (countBoxes (host) == 0, "a click on the backdrop closes it");
        pump (100);
    }

    // ---- narrow host --------------------------------------------------------------------------
    {
        juce::Component host;
        host.setSize (200, 400);
        auto& box = AboutBox::show (host, makeInfo (nullptr), nullptr);
        check (box.getCardBounds().getWidth() >= 184, "a 200 px host gets a card at least 184 wide");
        check (box.getCardBounds().getWidth() <= 200, "the card still fits a 200 px host");
        host.setSize (700, 500);
        check (box.getBounds() == host.getLocalBounds(), "the box follows the host when it resizes");
        check (box.getCardBounds().getWidth() == 420, "and the card re-sizes with it");
        closeAndWait (box);
    }

    // ---- sample rate not known yet (Review Focus 4) --------------------------------------------
    {
        juce::Component host;
        host.setSize (600, 400);
        TestProcessor proc (512, 0.0);
        auto& box = AboutBox::show (host, makeInfo (&proc), nullptr);
        check (hasLabel (box, "Delay: 512 samples"), "0 Hz shows samples with no ms value");
        closeAndWait (box);
    }

    // ---- with an Update notice -------------------------------------------------------------------
    {
        const auto folder = juce::File::getSpecialLocation (juce::File::tempDirectory)
                                .getChildFile ("zqsfx_aboutbox_" + juce::String (juce::Random::getSystemRandom().nextInt64()));
        folder.createDirectory();
        UpdateChecker::testOverride().settingsFolder = folder;
        UpdateChecker::testOverride().url = "file:///zqsfx-about-box-test-no-such-file.json"; // never the network

        {
            juce::Component host;
            host.setSize (600, 400);
            auto& box = AboutBox::show (host, makeInfo (nullptr, "unravel"), nullptr);

            bool hasPanel = false;
            for (auto* d : descendants (box))
                hasPanel = hasPanel || dynamic_cast<UpdateNoticePanel*> (d) != nullptr;
            check (hasPanel, "a productId puts an UpdateNoticePanel in the box");
            closeAndWait (box);
        }
        UpdateChecker::testOverride() = {};
        folder.deleteRecursively();
    }

    // ---- a short host shrinks the card to fit (Fix B) ------------------------------------------
    {
        juce::Component host;
        host.setSize (600, 300);
        auto& box = AboutBox::show (host, makeInfo (nullptr), nullptr);
        const auto card = box.getCardBounds();
        check (host.getLocalBounds().contains (card), "600x300 host: the card lies inside the host");
        check (card.getHeight() <= 300 - 16 + 1, "600x300 host: 16 px total margin left");
        check (box.getBounds() == host.getLocalBounds(), "600x300 host: the backdrop still covers the host");
        auto* close = findButton (box, "Close");
        check (close != nullptr && host.getLocalBounds().contains (box.getLocalArea (close, close->getLocalBounds())),
               "600x300 host: the Close button maps inside the host");
        check (box.getCardTransform().mat00 < 1.0f, "600x300 host: the card is scaled");
        check (std::abs (card.getCentreX() - 300) <= 1 && std::abs (card.getCentreY() - 150) <= 1,
               "600x300 host: the scaled card is centred in the host");

        auto src = juce::Desktop::getInstance().getMainMouseSource();
        const juce::Point<float> pf = card.getCentre().toFloat();
        juce::MouseEvent e (src, pf, juce::ModifierKeys(), 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, &box, &box,
                            juce::Time::getCurrentTime(), pf, juce::Time::getCurrentTime(), 1, false);
        box.mouseDown (e);
        check (countBoxes (host) == 1, "scaled card: a click inside its transformed bounds does not close");
        closeAndWait (box);
    }
    {
        juce::Component host;
        host.setSize (600, 800);
        auto& box = AboutBox::show (host, makeInfo (nullptr), nullptr);
        check (box.getCardTransform().mat00 == 1.0f, "600x800 host: no scaling");
        check (std::abs (box.getCardBounds().getCentreX() - 300) <= 1 && std::abs (box.getCardBounds().getCentreY() - 400) <= 1,
               "600x800 host: the card is centred");
        closeAndWait (box);
    }

    // ---- the host is destroyed while the box is open (Fix A) -----------------------------------------
    {
        const auto folder = juce::File::getSpecialLocation (juce::File::tempDirectory)
                                .getChildFile ("zqsfx_aboutbox_" + juce::String (juce::Random::getSystemRandom().nextInt64()));
        folder.createDirectory();
        UpdateChecker::testOverride().settingsFolder = folder;
        UpdateChecker::testOverride().url = "file:///zqsfx-about-box-test-no-such-file.json";

        auto proc = std::make_unique<TestProcessor> (512, 48000.0);
        auto host = std::make_unique<juce::Component>();
        host->setSize (600, 400);
        AboutBox::show (*host, makeInfo (proc.get(), "unravel"), nullptr);
        check (AboutBox::liveCount() == 1, "one live box before the host goes");
        host.reset();
        check (AboutBox::liveCount() == 0, "destroying the host deletes the open box at once");
        proc.reset(); // the timer must never read this again
        pump (700);
        check (AboutBox::liveCount() == 0, "no box and no crash after the processor is gone");

        UpdateChecker::testOverride() = {};
        folder.deleteRecursively();
    }

    if (failures == 0)
        std::cout << "All AboutBox tests passed\n";
    else
        std::cout << failures << " AboutBox test(s) FAILED\n";
    return failures == 0 ? 0 : 1;
}

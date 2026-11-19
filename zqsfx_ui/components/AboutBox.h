#pragma once
// The House About box: one card every ZQ SFX product opens from its LogoMark, showing the
// mark, name, version, (plugins) the reported delay, credits, licence and source link, the
// company contact, the Update notice panel and Close. A dimmed backdrop covers the host.
//
// Origin: new to this module (replaces each product's own About overlay).

#include <optional>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../tokens/Tokens.h"
#include "../lookandfeel/LookAndFeel.h"
#include "FocusRings.h"
#include "LogoMark.h"
#include "UpdateNotice.h"

namespace zqsfx::ui
{
struct AboutInfo
{
    juce::String productName, version;          // "Unravel", "1.4.0"
    juce::StringArray credits;                  // one line each
    juce::String sourceUrl;                     // "https://github.com/themightyzq/<repo>"
    juce::AudioProcessor* processor = nullptr;  // plugins: delay row; apps: nullptr, no row
    juce::String productId;                     // Update notice id ("unravel"); empty = no update panel
};

class AboutBox : public juce::Component, private juce::Timer, private juce::ComponentListener
{
public:
    // Covers `host` (the editor or main window content) with a dimmed backdrop and a centred card.
    // Focus moves into the card; on close it returns to `returnFocusTo` (normally the LogoMark).
    // The box is owned by the host's child list; the processor and notice must outlive it.
    static AboutBox& show (juce::Component& host, AboutInfo info, juce::Component* returnFocusTo)
    {
        for (auto* c : host.getChildren())
            if (auto* existing = dynamic_cast<AboutBox*> (c))
                return *existing;

        auto* box = new AboutBox (host, std::move (info), returnFocusTo);
        host.addAndMakeVisible (box);
        box->toFront (false);
        box->setBounds (host.getLocalBounds());
        box->closeButton.grabKeyboardFocus();
        return *box;
    }

    // Safe from inside one of its own buttons' click handlers: the box leaves the host at once
    // and is deleted on a later message-loop turn.
    void close()
    {
        if (closing)
            return;
        closing = true;
        stopTimer();

        auto focusTarget = returnTarget;
        if (auto* p = getParentComponent())
        {
            p->removeComponentListener (this);
            p->removeChildComponent (this);
        }
        bool showing = focusTarget != nullptr && focusTarget->isShowing();
       #if ZQSFX_UI_TESTING
        if (showingForTesting().has_value())
            showing = focusTarget != nullptr && *showingForTesting();
       #endif
        if (showing)
        {
           #if ZQSFX_UI_TESTING
            lastFocusGrabTarget() = focusTarget.getComponent();
           #endif
            focusTarget->grabKeyboardFocus();
        }

        std::shared_ptr<AboutBox> owner (this); // freed when the async lambda is destroyed
        juce::MessageManager::callAsync ([owner] {});
    }

    // Plain English, read live. 0 Hz (before prepareToPlay) leaves the ms part out.
    static juce::String delayText (int samples, double sampleRate)
    {
        if (samples <= 0)
            return "Delay: none";

        auto text = "Delay: " + juce::String (samples) + (samples == 1 ? " sample" : " samples");
        if (sampleRate > 0.0)
        {
            auto khz = juce::String (sampleRate / 1000.0, 1);
            if (khz.endsWith (".0"))
                khz = khz.dropLastCharacters (2);
            text += " (" + juce::String (samples * 1000.0 / sampleRate, 1) + " ms at " + khz + " kHz)";
        }
        return text;
    }

    static constexpr int minCardWidth = 260;

    // On-screen bounds of the card in the box (after any shrink-to-fit scaling).
    juce::Rectangle<int> getCardBounds() const { return card.getBoundsInParent(); }
    juce::AffineTransform getCardTransform() const { return card.getTransform(); }

    bool keyPressed (const juce::KeyPress& k) override
    {
        if (k == juce::KeyPress::escapeKey)
        {
            close();
            return true;
        }
        return false;
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (! getCardBounds().contains (e.getPosition()))
            close();
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (colour::panelBorder.withAlpha (0.6f)); // darkest token, dims the host
    }

    void resized() override { layoutCard(); }

   #if ZQSFX_UI_TESTING
    // The component that close() actually called grabKeyboardFocus on (null if it did not): the box
    // is gone by then and a test host is not on the desktop, so real focus cannot be read back.
    // showingForTesting makes the "is the target showing" decision true or false for a test.
    static juce::Component*& lastFocusGrabTarget() { static juce::Component* c = nullptr; return c; }
    static std::optional<bool>& showingForTesting() { static std::optional<bool> v; return v; }
    static int& liveCount() { static int n = 0; return n; }
   #endif

    ~AboutBox() override
    {
        stopTimer();
        if (hostPtr != nullptr)
            hostPtr->removeComponentListener (this);
        setLookAndFeel (nullptr);
       #if ZQSFX_UI_TESTING
        --liveCount();
       #endif
    }

private:
    // The rows live in a child so one transform can shrink the whole card on a short host
    // while the backdrop (this box) keeps covering everything.
    // A Label that draws in the font it was given, wrapped and centred. The module LookAndFeel
    // forces every Label into Barlow (getLabelFont), but the House puts numbers in IBM Plex Mono,
    // and a long credit line must wrap rather than squeeze. Layout measures with the same
    // TextLayout (wrappedHeight), so the row height always matches what is drawn.
    struct TextRow : juce::Label
    {
        void paint (juce::Graphics& g) override
        {
            juce::AttributedString s;
            s.append (getText(), getFont(), findColour (juce::Label::textColourId));
            s.setJustification (juce::Justification::centred);
            juce::TextLayout layout;
            layout.createLayout (s, (float) getWidth());
            const float top = juce::jmax (0.0f, ((float) getHeight() - layout.getHeight()) * 0.5f);
            layout.draw (g, getLocalBounds().toFloat().withTrimmedTop (top));
        }
    };

    struct Card : juce::Component
    {
        void paint (juce::Graphics& g) override
        {
            const auto r = getLocalBounds().toFloat();
            g.setGradientFill (gradients::panel (r));
            g.fillRoundedRectangle (r, 6.0f);
            g.setColour (colour::ruleTitle);
            g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.0f);
        }
    };

    static constexpr int pad = 20, gap = 6, rowH = geom::minHitTargetPx + 4, maxCardWidth = 420;

    AboutBox (juce::Component& host, AboutInfo i, juce::Component* returnFocusTo)
        : hostPtr (&host), info (std::move (i)), returnTarget (returnFocusTo), mark (info.productName)
    {
       #if ZQSFX_UI_TESTING
        ++liveCount();
       #endif
        addAndMakeVisible (card);
        setLookAndFeel (laf.operator->()); // the same look inside any Product's own LookAndFeel
        setTitle ("About " + info.productName);
        // keyboardFocusContainer, not focusContainer (that is only an accessibility group): it is
        // what makes Tab and Shift+Tab cycle inside the box instead of reaching the editor behind.
        setFocusContainerType (FocusContainerType::keyboardFocusContainer);
        setWantsKeyboardFocus (true);
        host.addComponentListener (this);

        mark.setInterceptsMouseClicks (false, false); // the trigger lives in the editor, not here
        mark.setWantsKeyboardFocus (false);
        mark.setAlwaysBright (true);
        card.addAndMakeVisible (mark);

        makeLabel (nameLabel, info.productName, laf->silkFont (22.0f, true), colour::silkTitle);
        makeLabel (versionLabel, "Version " + info.version, laf->stampFont (13.0f), colour::silkLabel);

        if (info.processor != nullptr)
        {
            makeLabel (delayLabel, {}, laf->stampFont (12.0f), colour::silkLabel);
            refreshDelay();
            startTimer (500); // 2 Hz: follows a sample-rate or latency change while open
        }

        for (auto& line : info.credits)
        {
            auto* l = creditLabels.add (new TextRow());
            makeLabel (*l, line, laf->silkFont (14.0f), colour::silkLabel);
        }

        makeLabel (licenceLabel, "GPL-3.0-or-later", laf->stampFont (12.0f), colour::silkLabel);
        if (info.sourceUrl.isNotEmpty())
            makeLink (sourceLink, "Source code", info.sourceUrl, "Open the source code in your browser.");
        makeLink (siteLink, "zq-sfx.com", "https://www.zq-sfx.com", "Open zq-sfx.com in your browser.");
        makeLabel (emailLabel, "connect@zq-sfx.com", laf->stampFont (12.0f), colour::silkLabel);

        if (info.productId.isNotEmpty())
        {
            // Owned here, not by the editor: the editor can be destroyed while this box is open.
            notice = std::make_unique<UpdateNotice> (mark, info.productId, info.version);
            panel = std::make_unique<UpdateNoticePanel> (*notice);
            panel->onIdealHeightChanged = [this] { layoutCard(); };
            card.addAndMakeVisible (*panel);
        }

        closeButton.setButtonText ("Close");
        closeButton.setTitle ("Close");
        closeButton.setTooltip ("Close the About box (Escape).");
        closeButton.setDescription ("Close the About box.");
        closeButton.onClick = [this] { close(); };
        card.addAndMakeVisible (closeButton);

        enableFocusRings (*this);
    }

    void makeLabel (juce::Label& l, const juce::String& text, const juce::Font& f, juce::Colour c)
    {
        l.setText (text, juce::dontSendNotification);
        l.setFont (f);
        l.setColour (juce::Label::textColourId, c);
        l.setJustificationType (juce::Justification::centred);
        l.setInterceptsMouseClicks (false, false);
        card.addAndMakeVisible (l);
    }

    void makeLink (juce::HyperlinkButton& b, const juce::String& text, const juce::String& url, const juce::String& tip)
    {
        b.setButtonText (text);
        b.setURL (juce::URL (url));
        b.setFont (laf->silkFont (14.0f), false, juce::Justification::centred);
        b.setColour (juce::HyperlinkButton::textColourId, colour::silkTitle);
        b.setTitle (text);
        b.setTooltip (tip);
        b.setDescription (tip);
        card.addAndMakeVisible (b);
    }

    void refreshDelay()
    {
        if (info.processor == nullptr)
            return;
        const auto text = delayText (info.processor->getLatencySamples(), info.processor->getSampleRate());
        if (text != delayLabel.getText())
            delayLabel.setText (text, juce::dontSendNotification);
    }

    void timerCallback() override { refreshDelay(); }

    void componentMovedOrResized (juce::Component& c, bool, bool wasResized) override
    {
        if (wasResized && &c == hostPtr.getComponent())
            setBounds (c.getLocalBounds());
    }

    // The host (an editor) is being destroyed with this box still open. Nothing may outlive it:
    // the timer reads the processor and the panel reads the notice. JUCE's ListenerList is safe
    // against a listener removing (or deleting) itself mid-callback, and the host's SafePointers
    // are cleared only after this callback, so removing ourselves here is valid. Clearing
    // hostPtr keeps the destructor from touching the dying host a second time.
    void componentBeingDeleted (juce::Component& c) override
    {
        if (&c != hostPtr.getComponent())
            return;
        stopTimer();
        closing = true;
        c.removeComponentListener (this);
        hostPtr = nullptr;
        // Drop focus first: removing a focused child otherwise hands focus to the dying host.
        giveAwayKeyboardFocus();
        c.removeChildComponent (this);
        delete this;
    }

    int cardWidthFor (int hostWidth) const
    {
        // Never below minCardWidth unless the host itself is narrower (then host minus 16).
        if (hostWidth - 32 >= minCardWidth)
            return juce::jmin (maxCardWidth, hostWidth - 32);
        return juce::jmax (1, juce::jmin (hostWidth - 16, minCardWidth));
    }

    static int wrappedHeight (const juce::String& text, const juce::Font& f, int width)
    {
        juce::AttributedString s;
        s.append (text, f);
        s.setJustification (juce::Justification::centred);
        juce::TextLayout layout;
        layout.createLayout (s, (float) juce::jmax (1, width));
        return juce::jmax (rowH - 6, (int) std::ceil (layout.getHeight()) + 4);
    }

    // One pass lays out and measures: card width comes from the host, height from the rows.
    void layoutCard()
    {
        if (hostPtr == nullptr)
            return;
        const int w = cardWidthFor (getWidth());
        const int inner = juce::jmax (1, w - 2 * pad);
        int y = pad;

        auto row = [&] (juce::Component& c, int h)
        {
            c.setBounds (pad, y, inner, h);
            y += h + gap;
        };

        const int markH = juce::jmax (40, LogoMark::minimumHeight());
        mark.setBounds ((w - markH * 8 / 7) / 2, y, markH * 8 / 7, markH);
        y += markH + gap;
        row (nameLabel, 30);
        row (versionLabel, 20);
        if (info.processor != nullptr)
            row (delayLabel, wrappedHeight (delayLabel.getText(), delayLabel.getFont(), inner));
        for (auto* l : creditLabels)
            row (*l, wrappedHeight (l->getText(), l->getFont(), inner));
        row (licenceLabel, 20);
        if (info.sourceUrl.isNotEmpty())
            row (sourceLink, rowH);
        row (siteLink, rowH);
        row (emailLabel, 20);

        if (panel != nullptr)
        {
            panel->setSize (inner, 1); // the stacked layout depends on the width
            const int h = panel->getIdealHeight();
            row (*panel, h);
        }

        y += gap; // Close stands apart from the text above it
        closeButton.setBounds ((w - 100) / 2, y, 100, rowH);
        y += rowH + pad;

        // Too tall for the host: shrink the whole card uniformly, leaving 16 px total margin.
        const int h = y;
        const float s = juce::jmin (1.0f, (float) juce::jmax (1, getHeight() - 16) / (float) juce::jmax (1, h));
        // A transform acts in parent space, about the parent's origin: bounds stay (0, 0, w, h)
        // and the translate does the centring, or the scale would also move the corner.
        const float cx = ((float) getWidth() - (float) w * s) * 0.5f;
        const float cy = ((float) getHeight() - (float) h * s) * 0.5f;
        card.setBounds (0, 0, w, h);
        card.setTransform (juce::AffineTransform::scale (s).translated (cx, cy));
        repaint();
    }

    juce::SharedResourcePointer<LookAndFeel> laf;
    juce::Component::SafePointer<juce::Component> hostPtr;
    AboutInfo info;
    juce::Component::SafePointer<juce::Component> returnTarget;
    bool closing = false;
    Card card;

    LogoMark mark;
    TextRow nameLabel, versionLabel, delayLabel, licenceLabel, emailLabel;
    juce::OwnedArray<TextRow> creditLabels;
    juce::HyperlinkButton sourceLink, siteLink;
    std::unique_ptr<UpdateNotice> notice;       // before the panel: destroyed after it
    std::unique_ptr<UpdateNoticePanel> panel;
    juce::TextButton closeButton;
};
} // namespace zqsfx::ui

#pragma once
// Update notice, UI half: UpdateNotice wires a product's LogoMark badge to the shared checker
// and carries the user's choices; UpdateNoticePanel is the block a product drops into its About
// box. Logic and storage live in update/UpdateCheck.h. Nothing here downloads or installs.
//
// Origin: new to this module.

#include <juce_gui_basics/juce_gui_basics.h>
#include "../tokens/Tokens.h"
#include "../update/UpdateCheck.h"
#include "FocusRings.h"
#include "LogoMark.h"

namespace zqsfx::ui
{
// Owned by a product's editor, beside the LogoMark it badges; must be destroyed before the
// logo is, or after, either way (the logo is held through a SafePointer).
class UpdateNotice : private juce::ChangeListener,
                     public juce::ChangeBroadcaster // tells the panel to redraw
{
public:
    UpdateNotice (LogoMark& logoMark, juce::String product, juce::String version)
        : logo (&logoMark), productId (std::move (product)), currentVersion (std::move (version))
    {
        // The id is the product's key in the Version file and part of file names: lower-case
        // letters and digits only ("hyperprismchorus", not "HyperPrism-Chorus"). Any other id shows nothing.
        jassert (isValidProductId (productId));
        checker->addChangeListener (this);
        checker->requestCheck();
        refresh();
    }

    ~UpdateNotice() override { checker->removeChangeListener (this); }

    // Re-reads settings and the cached file, then updates the badge. Cheap enough to call
    // whenever the About box opens.
    void refresh()
    {
        offered = checker->evaluate (productId, currentVersion).show;
        if (logo != nullptr)
            logo->setUpdateAvailable (offered.has_value());
        sendChangeMessage();
    }

    std::optional<VersionInfo> available() const { return offered; }

    void getIt()
    {
        if (offered)
            juce::URL (offered->getLink).launchInDefaultBrowser();
    }

    void later()
    {
        checker->getSettings().setLaterUntilMs (productId, juce::Time::currentTimeMillis() + update::dayMs);
        changed();
    }

    void skipThisVersion()
    {
        if (offered)
            checker->getSettings().setSkippedVersion (productId, offered->version);
        changed();
    }

    bool isEnabled() { return checker->getSettings().isEnabled(); }

    void setEnabled (bool on)
    {
        checker->getSettings().setEnabled (on);
        if (on)
            checker->requestCheck();
        else
            checker->cancelFetch(); // a fetch already under way must not write a cache
        changed();
    }

private:
    // Updates this notice now, and tells every other open notice in this binary to do the same.
    void changed()
    {
        refresh();
        checker->sendChangeMessage();
    }

    void changeListenerCallback (juce::ChangeBroadcaster*) override { refresh(); }

    juce::SharedResourcePointer<UpdateChecker> checker;
    juce::Component::SafePointer<LogoMark> logo;
    juce::String productId, currentVersion;
    std::optional<VersionInfo> offered;
};

// A block for the About box: the offer with its three buttons while an update is available, and
// the "Check for updates" switch always. Must not outlive its UpdateNotice.
class UpdateNoticePanel : public juce::Component, private juce::ChangeListener
{
public:
    explicit UpdateNoticePanel (UpdateNotice& n) : notice (n)
    {
        setTitle ("Update notice");

        styleControlLabel (message);
        message.setJustificationType (juce::Justification::centredLeft);
        message.setTitle ("Update available");
        addChildComponent (message);

        setup (getItButton, "Get it", "Open the download page in your browser.", [this] { notice.getIt(); });
        setup (laterButton, "Later", "Hide this for 24 hours.", [this] { notice.later(); });
        setup (skipButton, "Skip this version", "Hide this until a newer version than this one appears.",
               [this] { notice.skipThisVersion(); });

        const juce::String tip = "Once a day, ask the ZQ SFX Audio Tools page on GitHub whether a newer version exists. "
                                 "Turning this off turns it off in every ZQ SFX product.";
        toggle.setButtonText ("Check for updates");
        toggle.setTooltip (tip);
        toggle.setTitle ("Check for updates");
        toggle.setDescription (tip);
        toggle.setHelpText (tip);
        toggle.setColour (juce::ToggleButton::textColourId, colour::silkLabel);
        toggle.setColour (juce::ToggleButton::tickColourId, colour::accent);
        toggle.setColour (juce::ToggleButton::tickDisabledColourId, colour::silkCaption);
        toggle.onClick = [this] { notice.setEnabled (toggle.getToggleState()); };
        addAndMakeVisible (toggle);

        enableFocusRings (*this);
        notice.addChangeListener (this);
        sync();
    }

    ~UpdateNoticePanel() override { notice.removeChangeListener (this); }

    // Tells the host About box to re-lay out when the offer appears or goes away.
    std::function<void()> onIdealHeightChanged;

    // Depends on the current width: under narrowWidth the three buttons stack one per row, so
    // call this after setSize (and again after any resize).
    int getIdealHeight() const
    {
        if (! notice.available())
            return rowH;
        const int buttonRows = isStacked() ? 3 : 1;
        const int rows = 1 + buttonRows + 1; // message, buttons, switch
        return rows * rowH + (rows - 1) * gap;
    }

    static constexpr int narrowWidth = 260;

    void resized() override
    {
        auto r = getLocalBounds();
        if (notice.available())
        {
            message.setBounds (r.removeFromTop (rowH));
            r.removeFromTop (gap);
            if (isStacked())
            {
                for (auto* b : { &getItButton, &laterButton, &skipButton })
                {
                    b->setBounds (r.removeFromTop (rowH));
                    r.removeFromTop (gap);
                }
            }
            else
            {
                auto row = r.removeFromTop (rowH);
                const int w = row.getWidth();
                getItButton.setBounds (row.removeFromLeft (w * 22 / 100));
                row.removeFromLeft (gap);
                laterButton.setBounds (row.removeFromLeft (w * 22 / 100));
                row.removeFromLeft (gap);
                skipButton.setBounds (row);
                r.removeFromTop (gap);
            }
        }
        toggle.setBounds (r.removeFromTop (rowH));
    }

private:
    static constexpr int rowH = geom::minHitTargetPx + 4;
    static constexpr int gap  = 4;

    bool isStacked() const { return getWidth() > 0 && getWidth() < narrowWidth; }

    void setup (juce::TextButton& b, const juce::String& text, const juce::String& tip, std::function<void()> onClick)
    {
        b.setButtonText (text);
        b.setTooltip (tip);
        b.setTitle (text);
        b.setDescription (tip);
        b.setHelpText (tip);
        b.onClick = std::move (onClick);
        addChildComponent (b);
    }

    void changeListenerCallback (juce::ChangeBroadcaster*) override
    {
        const int before = getIdealHeight();
        sync();
        if (before != getIdealHeight() && onIdealHeightChanged)
            onIdealHeightChanged();
    }

    void sync()
    {
        const auto offer = notice.available();
        const bool on = offer.has_value();
        message.setText (on ? offer->name + " " + offer->version + " is available" : juce::String(),
                         juce::dontSendNotification);
        for (auto* c : { (juce::Component*) &message, (juce::Component*) &getItButton,
                         (juce::Component*) &laterButton, (juce::Component*) &skipButton })
            c->setVisible (on);
        toggle.setToggleState (notice.isEnabled(), juce::dontSendNotification);
        resized();
    }

    UpdateNotice& notice;
    juce::Label message;
    juce::TextButton getItButton, laterButton, skipButton;
    juce::ToggleButton toggle;
};
} // namespace zqsfx::ui

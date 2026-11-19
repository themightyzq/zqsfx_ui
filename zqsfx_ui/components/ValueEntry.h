#pragma once
// Typing a value into a knob: the House right-click menu ("Type a value...", "Reset to default"),
// Return-to-type, and the small text box that appears over the knob. Dial owns one of these, so
// every Product that uses Dial or Knob gets typing by moving to this module version; a plain
// juce::Slider gets the same behaviour by constructing a ValueEntry on it, and a Product that
// keeps its own right-click menu adds the two items with addValueEntryItems.
//
// The typed text is read through slider.getValueFromText, which a parameter attachment points at
// the parameter's own text conversion, so "2 kHz", "2000" and "-6 dB" work wherever the
// parameter accepts them. When a parameter is set, the typed value reaches the host as one
// complete change (begin, set, end) so automation recording captures it.
//
// Limit of the plain-slider path: on a plain linear juce::Slider, JUCE's own mouseDown runs
// before any listener and can jump the value to the click point for an instant before ValueEntry
// puts it back. With a host-parameter attachment that blip reaches the host. Products with host
// parameters should use Dial, which never passes a right-click to Slider::mouseDown.
//
// Origin: new to this module.

#include <juce_audio_processors/juce_audio_processors.h>

#include <memory>

namespace zqsfx::ui
{
class ValueEntry : private juce::MouseListener,
                   private juce::KeyListener,
                   private juce::Slider::Listener
{
public:
    // Dial passes this: it routes its own mouse and key events here, so no listeners are added.
    struct DialHandlesInput {};
    static constexpr DialHandlesInput dialHandlesInput {};

    // Plain-slider use: installs mouse and key listeners on the slider. The ValueEntry must not
    // outlive the slider.
    explicit ValueEntry (juce::Slider& s, juce::RangedAudioParameter* param = nullptr)
        : slider (s), parameter (param), ownsListeners (true)
    {
        slider.addMouseListener (this, false);
        slider.addKeyListener (this);
        slider.addListener (this);
    }

    ValueEntry (juce::Slider& s, DialHandlesInput) : slider (s), ownsListeners (false) {}

    ~ValueEntry() override
    {
        closeEditor (false, false);
        if (ownsListeners)
        {
            slider.removeListener (this);
            slider.removeKeyListener (this);
            slider.removeMouseListener (this);
        }
    }

    ValueEntry (const ValueEntry&) = delete;
    ValueEntry& operator= (const ValueEntry&) = delete;

    void setParameter (juce::RangedAudioParameter* p) { parameter = p; }
    bool hasParameter() const { return parameter != nullptr; }

    // False means the text was unreadable and the value is unchanged.
    bool applyText (const juce::String& text)
    {
        const auto trimmed = text.trim();
        if (trimmed.isEmpty() || ! trimmed.containsAnyOf ("0123456789"))
            return false;

        const auto value = slider.getRange().clipValue (slider.getValueFromText (trimmed));

#if JUCE_DEBUG
        // Only inside a plugin editor: an app's plain sliders have no host to tell.
        if (parameter == nullptr && ! warnedNoParameter
            && slider.findParentComponentOfClass<juce::AudioProcessorEditor>() != nullptr)
        {
            warnedNoParameter = true;
            DBG ("ValueEntry: no parameter set, so the host will not record typed values; call setDoubleClickDefault (slider, apvts, id)");
        }
#endif
        // Nothing changes, so the host sees no gesture either.
        if (juce::approximatelyEqual (value, slider.getValue()))
            return true;

        if (parameter != nullptr) parameter->beginChangeGesture();
        slider.setValue (value, juce::sendNotificationSync);
        if (parameter != nullptr) parameter->endChangeGesture();
        return true;
    }

    void resetToDefault()
    {
        if (! slider.isDoubleClickReturnEnabled())
            return;
        const auto value = slider.getDoubleClickReturnValue();
        if (juce::approximatelyEqual (value, slider.getValue()))
            return;
        if (parameter != nullptr) parameter->beginChangeGesture();
        slider.setValue (value, juce::sendNotificationSync);
        if (parameter != nullptr) parameter->endChangeGesture();
    }

    void addItems (juce::PopupMenu& menu)
    {
        // Actions rather than ids, so a Product's own menu ids cannot collide with ours.
        const std::weak_ptr<int> alive = lifeToken;
        menu.addItem (juce::PopupMenu::Item (juce::String (juce::CharPointer_UTF8 ("Type a value\xe2\x80\xa6")))
                          .setID (1)
                          .setAction ([this, alive] { if (! alive.expired()) openEditor(); }));
        menu.addItem (juce::PopupMenu::Item ("Reset to default")
                          .setID (2)
                          .setEnabled (slider.isDoubleClickReturnEnabled())
                          .setAction ([this, alive] { if (! alive.expired()) resetToDefault(); }));
    }

    void showMenu()
    {
        if (! slider.isEnabled())
            return;
#if ZQSFX_UI_TESTING
        if (true)
        {
            ++menusShownForTesting;
            return;
        }
#endif
        juce::PopupMenu menu;
        menu.setLookAndFeel (&slider.getLookAndFeel());
        addItems (menu);
        const auto pos = juce::Desktop::getMousePosition();
        menu.showMenuAsync (juce::PopupMenu::Options()
                                .withTargetScreenArea (juce::Rectangle<int> (pos.x, pos.y, 1, 1))
                                .withParentComponent (slider.findParentComponentOfClass<juce::AudioProcessorEditor>()));
    }

    void openEditor()
    {
        if (editor != nullptr || ! slider.isEnabled())
            return;
        // The box goes on the plugin editor (or the window), not the knob's own parent: Knob and
        // most Products wrap a knob in a component barely wider than the dial, which would clip it.
        juce::Component* parent = slider.findParentComponentOfClass<juce::AudioProcessorEditor>();
        if (parent == nullptr && slider.getParentComponent() != nullptr)
            parent = slider.getTopLevelComponent();
        if (parent == nullptr)
            return;

        editor = std::make_unique<juce::TextEditor>();
        auto& ed = *editor;
        ed.setTitle ("Type a value for " + slider.getTitle());
        ed.setTooltip ("Type a value, then press Return. Escape cancels.");
        ed.setMultiLine (false);
        ed.setReturnKeyStartsNewLine (false);
        ed.setJustification (juce::Justification::centred);
        ed.setText (slider.getTextFromValue (slider.getValue()), false);
        ed.onReturnKey = [this] { closeEditor (true, true); };
        ed.onEscapeKey = [this] { closeEditor (false, true); };
        ed.onFocusLost = [this] { closeEditor (true, false); };

        // At least 64x22 so a tiny knob still gets a usable box; kept inside the parent so a
        // knob at the window edge does not push it off-screen.
        const auto b = parent->getLocalArea (slider.getParentComponent(), slider.getBounds());
        auto box = juce::Rectangle<int> (juce::jmax (64, b.getWidth()), 22).withCentre (b.getCentre());
        box = box.constrainedWithin (parent->getLocalBounds());
        ed.setBounds (box);
        parent->addAndMakeVisible (ed);
        ed.selectAll();
        ed.grabKeyboardFocus();
    }

    juce::TextEditor* getOpenEditor() const { return editor.get(); }

    // Plain-slider use only; public so a test can feed them events without a window.
    void mouseDown (const juce::MouseEvent& e) override
    {
        if (! e.mods.isPopupMenu() || ! slider.isEnabled())
            return;
        swallowingPopupClick = true;
        restoreValueFromClick();
        showMenu();
    }

    void mouseDrag (const juce::MouseEvent&) override
    {
        if (swallowingPopupClick)
            restoreValueFromClick();
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        if (swallowingPopupClick)
            restoreValueFromClick();
        swallowingPopupClick = false;
        dragStartSeen = false;
    }

#if ZQSFX_UI_TESTING
    // Counts menus the House would have shown; tests read it instead of opening a real menu.
    static inline int menusShownForTesting = 0;
#endif

private:
    void closeEditor (bool apply, bool restoreFocus)
    {
        if (editor == nullptr)
            return;

        // Detach first: removing a focused editor fires onFocusLost, which must find nothing open.
        auto closing = std::move (editor);
        const auto text = closing->getText();
        // Move the callbacks into locals: one of them may be running right now, and assigning
        // nullptr would destroy it mid-call. The locals keep it alive until this function ends.
        auto keepFocusLost = std::move (closing->onFocusLost);
        auto keepReturn = std::move (closing->onReturnKey);
        auto keepEscape = std::move (closing->onEscapeKey);
        if (auto* parent = closing->getParentComponent())
            parent->removeChildComponent (closing.get());

        if (apply)
            applyText (text);
        if (restoreFocus)
            slider.grabKeyboardFocus();

        // The editor may be running one of its own callbacks right now, so delete it on the
        // next message instead of under its own feet.
        if (juce::MessageManager::getInstanceWithoutCreating() != nullptr)
            juce::MessageManager::callAsync ([doomed = std::shared_ptr<juce::TextEditor> (std::move (closing))] {});
    }

    // Plain-slider use only. The slider's own mouseDown runs before this listener, and on a
    // linear slider it can already have jumped the value to the click, so the value is taken
    // from sliderDragStarted (called before that jump) and put back here.
    void sliderValueChanged (juce::Slider*) override {}
    void sliderDragStarted (juce::Slider*) override { valueBeforeClick = slider.getValue(); dragStartSeen = true; }

    void restoreValueFromClick()
    {
        // JUCE skips the drag start for a disabled slider, an enabled popup menu or an empty
        // range; then there is no saved value and nothing moved, so nothing to put back.
        if (dragStartSeen && ! juce::approximatelyEqual (slider.getValue(), valueBeforeClick))
            slider.setValue (valueBeforeClick, juce::sendNotificationSync);
    }

    bool keyPressed (const juce::KeyPress& key, juce::Component*) override
    {
        if (key == juce::KeyPress::returnKey)
        {
            openEditor();
            return true;
        }
        return false;
    }

    juce::Slider& slider;
    juce::RangedAudioParameter* parameter = nullptr;
    const bool ownsListeners;
    std::unique_ptr<juce::TextEditor> editor;
    std::shared_ptr<int> lifeToken = std::make_shared<int> (0);
    double valueBeforeClick = 0.0;
    bool swallowingPopupClick = false;
    bool dragStartSeen = false;
#if JUCE_DEBUG
    bool warnedNoParameter = false;
#endif
};

// For a Product that keeps its own right-click menu: adds the two House items to it.
inline void addValueEntryItems (juce::PopupMenu& menu, ValueEntry& entry) { entry.addItems (menu); }
} // namespace zqsfx::ui

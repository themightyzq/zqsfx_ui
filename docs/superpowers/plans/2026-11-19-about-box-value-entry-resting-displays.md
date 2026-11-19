# About box, typing values, resting displays: Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add three House behaviours to `zqsfx_ui` 0.7.0 that every Product picks up the same way: typing a knob value, resting displays, and one About box.

**Architecture:** The module is header-only JUCE. It gets three new headers in `zqsfx_ui/components/`, each with its own console test program in `tests/`. `Dial` gains a `ValueEntry` member, so the ten Products that use `Dial` or `Knob` get typing just by moving to the new module version. Tasks 1 to 3 touch only their own files. Task 4 (top tier) registers them, updates the demo, the snapshot tool and the docs, and adds the Hub uninstaller line.

**Tech Stack:** C++17, JUCE 8 (`juce_gui_basics`, `juce_audio_processors`), CMake, plain console test programs (see `tests/DialTests.cpp` for the pattern: `check (bool, const char*)`, exit 1 on any failure).

**Spec:** `docs/superpowers/specs/2026-11-19-about-box-value-entry-resting-displays-design.md`

## Global Constraints

- Namespace `zqsfx::ui`. Header-only, `#pragma once`. The header comment says what it is and that its origin is "new to this module".
- No colour literals. Colours come from `tokens/Tokens.h` (`colour::…`) or the module LookAndFeel.
- Fonts: Barlow Condensed is `LookAndFeel::silkFont`, IBM Plex Mono is `LookAndFeel::stampFont`.
- Every interactive control has `setTitle`, a tooltip, a visible focus ring (`enableFocusRings` / `markOwnFocusRing` in `FocusRings.h`) and a hit target of at least 22 px. Text contrast must meet WCAG AA (4.5:1).
- Audio-thread code: no allocation, no locks, no I/O, no logging.
- No absolute `/Users/...` paths. Never edit vendored JUCE.
- The module version stays `0.7.0`. Do not tag and do not push.
- Build trees go outside the project: `/Volumes/SD/Builds/zqsfx_ui/<task>/` when `/Volumes/SD` is a mounted volume (check with `mount | grep ' /Volumes/SD '`), otherwise the scratchpad given in the brief.

## Review Focus

1. **A Product's knob that already has its own right-click menu** (b33p's `B33pSlider` overrides `mouseDown`). The subclass override wins, and `Dial` must not open a second menu. Test: a `Dial` subclass that overrides `mouseDown` gets no House menu (Task 1).
2. **A right-click on a plain `juce::Slider` given `ValueEntry`** must not move the value or start a drag. Test in Task 1.
3. **The text box when the knob sits near the window edge, or the knob is tiny (NoTextBox, 30 px).** The box is at least 64×22 and stays inside the parent. Test in Task 1.
4. **The About box with the sample rate still unknown** (0 Hz before `prepareToPlay`) shows "Delay: 512 samples" with no ms value and no division by zero. Test in Task 3.
5. **Destroying a display while it rests, or the flag's owner being destroyed first.** The `RestClock` must hold no dangling callback. `RestClock` takes the flag by pointer, and the brief tells Products that the flag lives in the processor, which outlives the editor. Test: a `RestClock` destroyed while resting fires no callback afterwards (Task 2).

---

### Task 1: Typing a value (`ValueEntry`, wired into `Dial`)

**Files:**
- Create: `zqsfx_ui/components/ValueEntry.h`
- Modify: `zqsfx_ui/components/Dial.h`. Add a `ValueEntry` member. The `setDoubleClickDefault` overloads that take a parameter also call `valueEntry.setParameter` when the slider is a `Dial` (use `dynamic_cast`).
- Test: `tests/ValueEntryTests.cpp` (the CMake target `ZqsfxUiValueEntryTests` and CTest entry `ValueEntryTests` are already registered with `ZQSFX_UI_TESTING=1` and modal loops on; a placeholder file is there to replace; do not edit `CMakeLists.txt`)

**Interfaces (produces):**
```cpp
class ValueEntry  // attaches to one slider; owner keeps it alive no longer than the slider
{
public:
    explicit ValueEntry (juce::Slider&, juce::RangedAudioParameter* param = nullptr); // plain-slider use installs mouse+key listeners
    void setParameter (juce::RangedAudioParameter*);       // gestures go to this param when set
    bool applyText (const juce::String& text);             // false = unreadable, value unchanged
    void resetToDefault();                                 // slider's double-click value; no-op if none
    void addItems (juce::PopupMenu&);                      // "Type a value…" (id 1), "Reset to default" (id 2, disabled when no default)
    void showMenu();                                       // async PopupMenu at the mouse
    void openEditor();                                     // text box over the slider
    juce::TextEditor* getOpenEditor() const;               // nullptr when closed
};
inline void addValueEntryItems (juce::PopupMenu&, ValueEntry&);  // for Products with their own menu
// Dial: public member `ValueEntry valueEntry { *this };`
```

**Behaviour the code must have** (the spec's N1 section is the authority):
- `applyText` converts the text with `slider.getValueFromText`, which the attachment points at the parameter's own text conversion. It limits the value to the slider's range and calls `setValue (…, sendNotificationSync)`. When a parameter is set, it wraps that in `param->beginChangeGesture()` and `endChangeGesture()`. Text is unreadable when it is empty, or when the trimmed text has no digit.
- `Dial::mouseDown`: when `e.mods.isPopupMenu()`, call `valueEntry.showMenu()` and do not call `Slider::mouseDown`; otherwise call the base. `Dial::keyPressed`: Return opens the editor (returns true); everything else stays as it is now.
- The editor: a `juce::TextEditor` added to the slider's parent and centred on the slider, at least 64×22, inside the parent's bounds. It starts with `slider.getTextFromValue (getValue())`, all selected, and gets focus. Its title is "Type a value for " + the slider title. Return applies, Escape cancels, and losing focus applies. On close it is removed, and focus goes back to the slider. Only one editor is open per slider.
- In plain-slider use (constructed on a non-`Dial` slider), `ValueEntry` adds itself as a mouse listener and a key listener. A right-click shows the menu. The value must not change from that click: save the value on the popup-menu mouseDown and restore it on mouseUp if a drag moved it.

- [ ] **Step 1: Write the failing tests** in `tests/ValueEntryTests.cpp`. Build a small `juce::AudioProcessor` with one `AudioParameterFloat` "freq", range 20–20000, default 1000. Its text functions are `"N Hz"` / `"N kHz"`, and the value-from-text function parses an optional "k" multiplier. Attach a `Dial` through `APVTS::SliderAttachment` and call `setDoubleClickDefault (dial, apvts, "freq")`. Record gestures with an `AudioProcessorParameter::Listener`.
  - `applyText ("2 kHz")` gives 2000, with exactly one gesture begin and one end.
  - `applyText ("50000")` gives 20000, limited to the range.
  - `applyText ("banana")` returns false and leaves the value unchanged. `applyText ("")` does the same.
  - `resetToDefault()` gives 1000.
  - `addItems` gives two items, with ids 1 and 2 and the texts "Type a value…" and "Reset to default". On a slider with no double-click default, item 2 is disabled.
  - `dial.keyPressed (KeyPress (KeyPress::returnKey))` returns true, and `getOpenEditor() != nullptr`.
  - The editor's text equals `getTextFromValue`. After setting its text to "5 kHz" and calling its `onReturnKey`, the value is 5000 and the editor is closed. Escape (`onEscapeKey`) leaves the value unchanged and closes it.
  - A knob 30×30 at the parent's right edge in a 200×100 parent gives an editor at least 64×22 that lies inside the parent.
  - Review Focus 1: a `struct OwnMenuDial : Dial { void mouseDown (const MouseEvent&) override { ++hits; } }` sent a popup-menu mouse event counts hits == 1 and opens no House editor or menu. Use a `ZQSFX_UI_TESTING` counter `ValueEntry::menusShownForTesting`.
  - Review Focus 2: a plain `juce::Slider` with `ValueEntry ve (slider)`, sent a right-button mouseDown, drag and mouseUp (build `MouseEvent`s by hand), keeps its value, and `menusShownForTesting` goes up by 1.
- [ ] **Step 2: Run, expect FAIL** (compile errors count). Run `cmake --build <build> --target ZqsfxUiValueEntryTests && <build>/…/ZqsfxUiValueEntryTests`.
- [ ] **Step 3: Implement `ValueEntry.h`** and the `Dial.h` changes. `Dial.h` includes `ValueEntry.h`. `ValueEntry.h` must not include `Dial.h`.
- [ ] **Step 4: Run the tests and expect them all to PASS.** The existing `DialTests` must also still pass.
- [ ] **Step 5: Red proof.** Copy `ValueEntry.h` aside. Break the gesture wrap (remove `beginChangeGesture`), run the tests and see the gesture test FAIL. Restore the file and confirm the copy matches it with `cmp`. Report both outputs.

### Task 2: Resting displays (`ActivityFlag`, `RestClock`, `drawRestingMark`)

**Files:**
- Create: `zqsfx_ui/components/RestingDisplay.h`
- Test: `tests/RestingDisplayTests.cpp` (the CMake target `ZqsfxUiRestingDisplayTests` and CTest entry `RestingDisplayTests` are already registered with `ZQSFX_UI_TESTING=1` and modal loops on; a placeholder file is there to replace; do not edit `CMakeLists.txt`)

**Interfaces (produces):**
```cpp
class ActivityFlag   // lives in the processor; audio thread writes, message thread reads
{
public:
    void noteBlock (const juce::AudioBuffer<float>&) noexcept;  // peak of all channels
    void notePeak (float linearPeak) noexcept;                   // keeps the max since last take
    float takePeak() noexcept;                                   // returns max and resets to 0
};
class RestClock : private juce::Timer
{
public:
    static constexpr float silenceDb = -80.0f;
    static constexpr double holdSeconds = 2.0;
    static constexpr int checkHz = 4;
    RestClock (juce::Component& display, ActivityFlag* flag, std::function<void (bool resting)> onChange);
    // flag == nullptr: rests only when hidden. Starts awake; first evaluation on construction.
    bool isResting() const noexcept;
    void poll();   // evaluate now (the timer calls it at checkHz)
   #if ZQSFX_UI_TESTING
    void setClockForTesting (std::function<double()> nowSeconds);
    void setShowingForTesting (std::optional<bool>);  // overrides display.isShowing()
   #endif
};
void drawRestingMark (juce::Graphics&, juce::Rectangle<float> area);
```

**Behaviour:**
- The display rests when it is not showing (`display.isShowing()`, which is also false when the window is minimised). It also rests when every peak taken since the last loud one has stayed below −80 dBFS for at least `holdSeconds`. It wakes at the first `poll` that takes a peak at or above the threshold while the display is showing.
- `onChange` fires only on a change. When resting, it sets the display's accessible description to "No sound" (when resting for silence) or leaves it (when resting because hidden). On waking it restores the earlier description.
- `ActivityFlag` is one `std::atomic<float>`. `notePeak` uses a compare-exchange max. It does no allocation and takes no lock.
- `drawRestingMark` draws `LogoMark::createPath()` scaled to fit a centred square. The square's side is `min (area.getHeight() * 0.35f, area.getWidth() * 0.35f, 48.0f)`, and the fill is `colour::silkCaption` at alpha 0.5.

- [ ] **Step 1: Write the failing tests:**
  - Hidden display (forced false) → `isResting()` true after `poll()`.
  - Showing, loud peak (0.5) at t=0 → awake. Then silent polls at t=1.0 → awake; at t=2.1 → resting, `onChange (true)` called exactly once, description "No sound".
  - Loud peak at t=2.3 → awake, description restored.
  - A peak of −79 dBFS (`Decibels::decibelsToGain (-79.f)`) counts as sound, and one of −81 dBFS counts as silence.
  - `noteBlock` on a 2×512 buffer with a 0.25 sample in channel 1 → `takePeak() == 0.25f`, then the next `takePeak() == 0`.
  - `noteBlock` makes no allocation. Replace global `operator new`/`new[]` in the test file with counting versions, then call `noteBlock` 1000 times and expect a count of 0.
  - Review Focus 5: build a `RestClock` with a callback that writes to a counter in an outer scope. Let it rest, then destroy it, then pump the message loop for 600 ms. The counter must not change after destruction.
  - `drawRestingMark` into a 200×100 `Image`: some pixel near the centre is non-transparent, and the corner pixels are transparent.
- [ ] **Step 2: Run, expect FAIL.**
- [ ] **Step 3: Implement `RestingDisplay.h`.**
- [ ] **Step 4: Run, expect all PASS.**
- [ ] **Step 5: Red proof.** Change the hold to 0 seconds, see the hold test FAIL, restore, and check with `cmp`. Report both outputs.

### Task 3: The House About box (`AboutBox`)

**Files:**
- Create: `zqsfx_ui/components/AboutBox.h`
- Test: `tests/AboutBoxTests.cpp` (the CMake target `ZqsfxUiAboutBoxTests` and CTest entry `AboutBoxTests` are already registered with `ZQSFX_UI_TESTING=1` and modal loops on; a placeholder file is there to replace; do not edit `CMakeLists.txt`)

**Interfaces (consumes):** `LogoMark` (`components/LogoMark.h`), `UpdateNotice` and `UpdateNoticePanel (UpdateNotice&)` (`components/UpdateNotice.h`), `LookAndFeel::silkFont`/`stampFont`, `enableFocusRings`.

**Interfaces (produces):**
```cpp
struct AboutInfo
{
    juce::String productName, version;          // "Unravel", "1.4.0"
    juce::StringArray credits;                  // one line each
    juce::String sourceUrl;                     // "https://github.com/themightyzq/<repo>"
    juce::AudioProcessor* processor = nullptr;  // plugins: delay row; apps: nullptr, no row
    UpdateNotice* notice = nullptr;             // the Product's own; nullptr = no update panel
};
class AboutBox : public juce::Component
{
public:
    // Covers `host` (the editor or main window content) with a dimmed backdrop and a centred card.
    // Focus moves into the card; on close focus returns to `returnFocusTo` (normally the LogoMark).
    static AboutBox& show (juce::Component& host, AboutInfo, juce::Component* returnFocusTo);
    void close();                                   // removes itself and deletes itself asynchronously
    static juce::String delayText (int samples, double sampleRate);
    static constexpr int minCardWidth = 260;
};
```

**Behaviour and exact text:**
- `delayText`:
  - `(0, any)` gives "Delay: none".
  - `(512, 48000)` gives "Delay: 512 samples (10.7 ms at 48 kHz)".
  - `(512, 44100)` gives "Delay: 512 samples (11.6 ms at 44.1 kHz)".
  - `(1000, 96000)` gives "Delay: 1000 samples (10.4 ms at 96 kHz)".
  - `(512, 0)` gives "Delay: 512 samples".
  - `(1, 48000)` gives "Delay: 1 sample (0.0 ms at 48 kHz)".
  - The ms value has one decimal. The kHz value drops ".0".
- The card's rows, top to bottom:
  1. the mark (a `LogoMark`, not clickable, at least `LogoMark::minimumHeight()` tall);
  2. the product name in `silkFont` (bold, 22 px);
  3. "Version " + version in `stampFont`;
  4. the delay row, in `stampFont`, only when `processor` is set. It refreshes from `processor->getLatencySamples()` and `getSampleRate()` on a 2 Hz timer while open;
  5. each credit line;
  6. "GPL-3.0-or-later" and a `HyperlinkButton` "Source code" (`sourceUrl`, omitted if empty);
  7. a `HyperlinkButton` "zq-sfx.com" (`https://www.zq-sfx.com`) and the text "connect@zq-sfx.com";
  8. the `UpdateNoticePanel`, when `notice` is set;
  9. a `TextButton` "Close".
- Card width: `jlimit (minCardWidth, 420, host.getWidth() - 32)`. It never goes below `minCardWidth`. If the host is narrower, the card is the host width minus 16, and the panel's narrow layout handles it.
- The backdrop is `colour::panelShadow` or the darkest token, at alpha 0.6. Escape, Close, or a click on the backdrop outside the card closes the box. Only one box per host: `show` on a host that already has one returns the existing box.
- The box sets a `SharedResourcePointer<zqsfx::ui::LookAndFeel>` as its own look, so it looks the same inside a Product that uses its own look.

- [ ] **Step 1: Write the failing tests:**
  - All the `delayText` cases above.
  - On a 600×400 host with a `DemoProcessor`-like `AudioProcessor` reporting a delay of 512 at 48 kHz, call `show`. The host then has one `AboutBox` child, and the box contains a `Label` whose text is "Delay: 512 samples (10.7 ms at 48 kHz)".
  - With `processor = nullptr`, no label text starts with "Delay".
  - With `notice` set (use `ZQSFX_UI_TESTING`'s `testOverride()` from `update/UpdateCheck.h`, pointing at a temp settings folder and an `https://127.0.0.1:9/` URL so nothing is fetched), there is a descendant `UpdateNoticePanel`.
  - `show` called twice → still one child.
  - `keyPressed (Escape)` → after pumping the message loop, the host has no `AboutBox` child, and `returnFocusTo` is the pending focus target. Check `Component::getCurrentlyFocusedComponent() == returnFocusTo` when it is on the desktop; otherwise assert through a `ZQSFX_UI_TESTING` field `lastFocusReturnTarget`.
  - Every `Button` descendant has a non-empty title and tooltip and is at least 22 px tall.
  - A 200 px wide host → card width ≥ 184 (200 − 16).
  - Review Focus 4: a sample rate of 0 → "Delay: 512 samples".
- [ ] **Step 2: Run, expect FAIL.**
- [ ] **Step 3: Implement `AboutBox.h`.**
- [ ] **Step 4: Run, expect all PASS.** `UpdateTests` must also still pass.
- [ ] **Step 5: Red proof.** Make `delayText` drop the ms clause, see the test FAIL, restore, and check with `cmp`. Report both outputs.

### Task 4: Integration, demo, docs, Hub uninstaller line (top tier, after 1–3)

**Files:**
- Modify:
  - `zqsfx_ui/zqsfx_ui.h`: include `ValueEntry.h` (before `Dial.h`), `RestingDisplay.h` and `AboutBox.h` (after `UpdateNotice.h`).
  - `demo/DemoComponent.h`: the logo opens the `AboutBox`. `SineScreen` rests through `RestClock` and draws the mark. The `DemoProcessor` reports a delay of 512.
  - `tools/snapshot/Main.cpp`: also write `snapshots/about_plugin.png` and `snapshots/about_app.png`.
  - `CHANGELOG.md`, `README.md` and `docs/ZQSFX_UI_STYLE_GUIDE.md` (mark section: the resting mark, and the About box as a Brand mark).
  - In the Hub: `ZQSFX_AudioTools/installer/userdata.txt` (the Updates folder) and `ZQSFX_AudioTools/tests/test_installer.sh` (accept and refuse lines, each seen red once).
- [ ] **Step 1:** Integrate. Run `tools/check.sh` and expect every test ok.
- [ ] **Step 2:** Run the full tier: a Universal Release build of every target, then `ctest --test-dir <build> --output-on-failure`. Render the snapshots, look at them, and send them to ZQ.
- [ ] **Step 3:** Run an independent `code-reviewer` pass on the whole diff and fix its findings.
- [ ] **Step 4:** Commit by path: `dv commit zqsfx_ui ZQSFX_AudioTools/installer ZQSFX_AudioTools/tests ZQSFX_AudioTools/TASKS.md -m …`, plus a local git commit in `zqsfx_ui` (`connect@zq-sfx.com`, no push). The commit is made only when ZQ asks.

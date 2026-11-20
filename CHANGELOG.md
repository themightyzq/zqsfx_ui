# Changelog

All notable changes to this project are documented here. Versioning is semver; token
values only change in a major version (see README.md "Versioning").

## [Unreleased]

## [0.7.2] - 2026-11-19

Findings from product re-releases that adopted 0.7.1.

### Added
- `Knob` takes a `parse` argument after `fmt`. With a `fmt` and no `parse` it installs a default
  reader, `parseReadoutText` ("50 %", "10.3 kHz", "2k", "1,000 Hz", "10,5", "Tonal 50", "- 5",
  "-6 dB", "-inf").
  It covers readouts in those shapes only; a `fmt` that changes units or names a side ("1.20 s",
  "L 30 %") must pass its own `parse` (README, "Typing a value"). `KnobTests`.
- `RestClock::setDescriptionTarget`: choose which component says "No sound" (`nullptr` for none).
- `ActivityFlag::takePeak()`: the loudest peak since the last call, so a meter shows the real
  peak of a short hit after rest. One reader only.
- `PeakMeter::setFillColour`: a solid fill for a meter whose colour carries meaning.
- README: typing needs `EDITOR_WANTS_KEYBOARD_FOCUS TRUE`; custom readouts need a matching
  parser; Linux builds (`JUCE_USE_CURL=0`) never show an update notice.

### Fixed
- Typing a 0..1 knob's "50 %" gave 1.0, and "10.3 kHz" clamped to the range floor, because the
  typed text was read by the parameter's plain parser, not the knob's readout.
- Typing a word readout such as "Off" or "-inf" was refused for lack of a digit. It is now
  accepted when the slider shows the same word back for the value it read.
- Opening "Type a value" and closing it without typing no longer applies the shown text, so a
  readout that rounds or names a span ("Off") can no longer move the knob on its own, and a
  stale readout cannot drag it back against host automation.
- A word typed without its unit ("-inf" for a readout "-inf dB") is accepted.
- `RestClock` gives a description target that outlives it its own text back when it is destroyed.
- `RestClock`'s description target is a safe pointer: a target deleted first is forgotten, not
  used after it is gone.
- `RestClock::setClockForTesting` and `setShowingForTesting` are always available, so a
  Product's tests (built without `ZQSFX_UI_TESTING`, no message loop) can call `poll()`.

## [0.7.1] - 2026-11-19

Findings from the first product to adopt 0.7 (Worldizer).

### Added
- `ZQSFX_UPDATE_URL=off` and `ZQSFX_UPDATE_SETTINGS_DIR=<absolute folder>` environment
  variables: a product's own tests can keep the update check off the network and out of the real
  settings folder (a headless editor test had stamped the real "last checked" date).
- `PeakMeter`: a tooltip and title of the product's choosing, an optional fall
  (`setFallDbPerSecond`, House value `houseFallDbPerSecond` = 20 dB/s), and `setResting` (an
  empty well). `PeakMeterTests`.

### Changed
- `ActivityFlag` moved to `audio/ActivityFlag.h`, which needs only `juce_audio_basics`, so a
  processor header no longer pulls in GUI code. `RestingDisplay.h` still includes it.

### Fixed
- `setDoubleClickDefault (slider, const RangedAudioParameter&)` now also gives a `Dial` its
  parameter. Before, a product holding its parameter by const reference silently lost host
  recording of typed values.
- Removed the debug warning about a `Dial` with no parameter: it fired on knobs that are
  deliberately not parameters.
- Tests: `ValueEntryTests` crashed on Linux at exit; two `UpdateTests` checks assumed an idle
  machine. CI now builds every test program.

## [0.7.0] - 2026-11-19

### Added
- Update notice. `update/UpdateCheck.h`: version comparison, the Version file reader, the
  once-a-day decision, `UpdateSettings` (the user's answers, shared by every ZQ SFX product: one
  small file per setting in `ZQ SFX/Updates/`, each replaced whole by an atomic rename, no locks)
  and `UpdateChecker` (one per plugin binary; fetches at most once a day when an editor opens, and
  once any product has checked that day the others skip; never downloads or installs). Closing a
  window mid-check cancels the request instead of killing the thread. `ZQSFX_UPDATE_URL` may only
  name an `https://` address. `components/UpdateNotice.h`: `UpdateNotice` wires a product's logo
  to the checker and carries Get it, Later, Skip this version and the "Check for updates" switch;
  every open notice in the binary updates at once. `UpdateNoticePanel` is the About-box block; under
  260 px wide its buttons stack. `LogoMark::setUpdateAvailable` draws a "!" badge and adds
  "(update available)" to the mark's tooltip and accessible text.
- Hardening from review: a Version file nested deeper than 32 levels is refused before parsing (a
  3 KB file of brackets crashed the host's background thread); an id that is not lower-case
  letters and digits shows no offer, since Later and Skip could not hide it; a check claims the day
  before fetching, so plugins opening together fetch once, and an unwritable settings folder still
  fetches only once a day per process; a cancelled check hands the day back; writes retry a
  locked file three times (Windows).
- `UpdateTests` console test, run in CI.
- Typing a value (`components/ValueEntry.h`). Every `Dial`, and so every `Knob`, now has the
  House right-click menu ("Type a value…", "Reset to default"), and Return on a focused knob
  opens a small text box over it. Text is read through the parameter's own text conversion, so
  "2 kHz" works where the parameter accepts it; out-of-range values are limited to the range;
  unreadable text changes nothing. A typed value reaches the host as one complete change, so
  automation recording takes it. `setDoubleClickDefault (slider, apvts, id)` also tells a `Dial`
  its parameter. A plain `juce::Slider` gets the same by owning a `ValueEntry` on it; a Product
  with its own knob menu adds the two items with `addValueEntryItems`. A right-click never moves
  the value. `ValueEntryTests`.
- Resting displays (`components/RestingDisplay.h`). `ActivityFlag` (in the processor; the audio
  thread notes each block's peak, no allocation, no lock) and `RestClock` (owned by a display)
  decide four times a second whether the display should animate: it rests while hidden or
  minimised, or after 2 seconds below -80 dBFS, and wakes at the first loud block.
  `drawRestingMark` draws the dim mark a resting display of live sound shows; a display of
  settings stops moving but keeps drawing them. `RestingDisplayTests`.
- The House About box (`components/AboutBox.h`). One card over a dimmed backdrop inside the
  window: mark, name, version, the reported delay (plugins; read live, "Delay: 512 samples
  (10.7 ms at 48 kHz)"), credits, licence and source link, zq-sfx.com and the address, the
  Update notice panel, Close. Escape, Close or a click outside closes it and focus returns to the
  logo; it scales down to fit a short window; it closes itself if the window is destroyed under
  it. `AboutBoxTests`. `LogoMark::setAlwaysBright` for the mark inside it.
- The gallery's logo opens the About box, and the screens panel shows a resting display. The
  snapshot tool also writes `snapshots/about_plugin.png` and `snapshots/about_app.png`.

### Fixed
- The module declaration's `version:` line read 0.5.0; it now matches the release.

## [0.6.1] - 2026-11-11

### Fixed
- `enableFocusRings` now opts in every component under the root, not only those that want
  keyboard focus at the moment it runs. JUCE reports a disabled control as not wanting focus, so
  in 0.6.0 a button that started disabled (Split, Add Chord) never got its ring once enabled. A
  component that never takes focus never shows the ring, so opting it in changes nothing.
  `FocusRingTests` covers the disabled case.

## [0.6.0] - 2026-11-11

### Added
- `zqsfx::ui::enableFocusRings (root)` (`components/FocusRings.h`): opts every focusable
  component under `root` into the house focus ring. JUCE draws the ring only on components that
  call `setHasFocusOutline (true)`, and its own buttons, combo boxes and toggles never do, so in
  products that used them, tabbing to a button showed nothing. The root itself is skipped (an
  editor takes focus only for shortcuts). Call it at the end of an editor's constructor.
- `zqsfx::ui::markOwnFocusRing (c)`: marks a control that draws its own focus so
  `enableFocusRings` leaves it alone. `Knob` (its ring around the dial face) and `LogoMark` (it
  lights up) now use it.
- `FocusRingTests` console test, run in CI.

## [0.5.0] - 2026-10-02

### Added
- `zqsfx::ui::Dial` (`components/Dial.h`): the house rotary dial as a namespace-level class, a
  drop-in replacement for a `juce::Slider` member. It takes keyboard focus, shows the
  LookAndFeel's focus outline, keeps JUCE's plain-arrow step and adds Shift+arrow as a fine
  step of one tenth. Products can adopt the house knob behaviour without re-laying out their
  editors. `Knob::Dial` remains as an alias, so existing code is unchanged.
  On a stepped control (nonzero interval) Shift+arrow moves one interval, the finest step it
  can take, rather than a tenth that the slider would snap back.
- `zqsfx::ui::setDoubleClickDefault`: double-click returns a slider to its parameter's
  default. Overloads take a `RangedAudioParameter`, an APVTS plus parameter ID, or a plain
  default value for controls not bound to a parameter. `Knob` now uses it.
- `DialTests` console test (key steps, focus settings, double-click default), run in CI.
- `.github/workflows/ci.yml`: builds the `ZqsfxUiTokenTests` target and runs it through
  `ctest` on macOS and Linux for every push to main/master, pull request, and manual run.
  No module code or token values changed.

## [0.4.1] - 2026-09-25

### Fixed
- `LookAndFeel::stripFor` no longer drops straight to `drawVectorKnob` for a size band whose
  own strip failed to load while the other two strips are perfectly fine. It now tries the
  dial-size-matched strip (or the slider's explicit `zqsfxStrip` override) first, then falls
  through to the other two loaded strips, nearest size band first, and only reaches the
  vector fallback when none of the three have loaded at all. Reproduced by invalidating just
  `stripS`: every dial under 42 px used to render as the flat, dark, scalloped vector knob
  next to an untouched `stripM` knob one slot over, a jarring mismatch (colour, cap material,
  pointer weight, ring style) that a single bad or missing embed could trigger for an entire
  size band. `drawVectorKnob` itself is unchanged and stays the true last resort.
- `drawRotarySlider` now explicitly requests `Graphics::highResamplingQuality` before
  blitting a strip frame. Every dial size this module currently defines (66/46/44/40, and
  anything a product asks for below that) downscales a strip frame, where this makes no
  difference (area-averaging either way), but it also covers a dial requested LARGER than a
  strip's native frame (bicubic instead of bilinear upsampling), so a knob is never softer
  than the strip art actually is, at any size.

### Changed
- The two knob design source files are renamed by their KnobGallery number
  (`knob_gallery_2410.knob`, `knob_gallery_2075.knob`); PROVENANCE.md updated. README gains
  the contact line.

## [0.4.0] - 2026-09-23

### Added
- `Knob` is keyboard-operable. Its inner slider is now `Knob::Dial`, a `juce::Slider` that
  takes keyboard focus (JUCE's default is to refuse it, so no house knob could be tabbed to
  or nudged before), keeps the plain arrow step (the parameter interval, or 1 % of the
  range), adds Shift+arrow as a fine step of one tenth of that, and draws an accent focus
  ring around the dial while focused. `knob.slider` keeps its name and type-compatibility
  with `juce::Slider&`, so no call site changes.

## [0.3.0] - 2026-09-22

### Added
- `Knob` now sets `setDoubleClickReturnValue` from the bound parameter's own default, so
  double-clicking any house knob returns it to that parameter's default value. The default is
  read live from the APVTS and converted out of normalised form, so it stays correct if a
  product changes a parameter's default later.

  This was deliberately centralised rather than fixed per product. A portfolio-wide audit
  found only three of ten products implemented the behaviour at all, and the shared `Knob`
  not having it meant every adopting product inherited the gap by default. Inconsistency
  between sibling products is itself the usability problem: a person who uses two of these
  plugins learns that double-click works, then finds it silently does nothing in the next one.

## [0.2.1] - 2026-09-21

### Changed
- `strip_a` (large dials) now carries a painted cream pointer. The design's own pointer is a
  small dark tick that was close to unreadable on a product's main controls. The as-designed
  strip is kept in `assets/knobs/rendered/` and restoring it is a one-file copy.

### Added
- `tools/paint_knob_pointer.py`.

## [0.2.0] - 2026-09-21

### Added
- **House knobs embedded and on by default**: three CC0 filmstrips from the KnobGallery
  (#2638 for dials 56 px and up, #2410 for 42 px and up, #2075 below), 128 frames each, with
  provenance, unmodified `.knob` sources, and render steps in `assets/knobs/`. No per-product
  copy, credit, or licence needed.
- `tools/normalise_knob_strip.py` (crop a strip to a centred frame with a feathered edge).

### Changed
- Visible change for any consumer that did not call `setKnobStrips`: knobs are now the house
  filmstrips instead of the vector knob, which remains as the load-failure fallback.
- #2075 was rendered with a 270 degree sweep (its source animates a full turn, so minimum and
  maximum looked identical) and cropped from 120 to 80 px so it matches the other two in scale.

## [0.1.1] - 2026-09-21

Owner decisions after reviewing the gallery.

### Changed
- `Panel` section titles are drawn in the platform bold face (as Broken's `Block` always
  did), no longer in Barlow Condensed. This is a visible change for anything that used
  `Panel` in 0.1.0; nothing had shipped on it.
- The house knob is the filmstrip art Broken uses. It cannot be redistributed from a public
  library (see README "Knobs"), so products embed their own copy. The vector knob is now
  documented as the fallback, not the default look.

### Added
- CMake `zqsfx_ui_add_knob_strips (DIR <dir> TARGETS <t>...)` embeds a product's strips as
  `ZqsfxKnobStrips.h`.
- `LookAndFeel::setKnobStripsFromMemory (...)`.

## [0.1.0] - 2026-09-21

Initial extraction of the shared ZQ SFX UI module from Broken (Project_TurboSynth),
per `docs/ZQSFX_UI_STYLE_GUIDE.md`. Broken itself is unchanged; this is a new,
product-agnostic module only.

### Added
- `tokens/Tokens.h`: base palette, complementary colour-blind-safe channels
  (`comp::sky/yellow/purple/green/white`), gradients, and geometry constants, ported
  byte-identical from Broken's `Theme.h` (legacy compatibility aliases dropped).
- `lookandfeel/LookAndFeel`: ported from Broken's `TsLookAndFeel`, with a vector-drawn
  rotary knob by default (`drawVectorKnob`), optional filmstrip image knobs via
  `setKnobStrips` for products that keep licensed art, and keyboard focus drawn through
  `createFocusOutlineForComponent` instead of hand-drawn rings.
- `components/`: `Panel`, `Knob`, `Combo`, `LitToggle`, `TextToggle`, `Led`,
  `PhosphorScreen`, `PeakMeter`, `LogoMark`, ported from Broken's `Theme.h` and
  `Controls.h`, generalised to drop product-specific hard-coding, and extended with the
  accessibility floor (title, description, help text on every interactive wrapper).
- Embedded OFL fonts (Barlow Condensed Medium/SemiBold, VT323, IBM Plex Mono) via
  `juce_add_binary_data`.
- `demo/`: an interactive gallery app showing one of every control.
- `tools/snapshot/`: a headless PNG renderer of the gallery, for visual review without
  a display.
- `tests/TokenTests.cpp`: WCAG contrast, Machado 2009 colour-blind simulation with CIE
  Lab deltaE separation, token parity against Broken's `Theme.h`, and logo path checks.
- CMake: FetchContent-friendly `juce_add_module` setup, `zqsfx::zqsfx_ui` (and
  `zqsfx::ui`) as the link target, `ZQSFX_UI_BUILD_DEMO` to keep consumers from also
  building the gallery/snapshot/tests.

### Notes
- No product has migrated onto this module yet. Broken keeps its own filmstrip knob
  art and hand-drawn focus rings until it migrates (see the style guide's rollout
  order).

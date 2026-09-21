# Changelog

All notable changes to this project are documented here. Versioning is semver; token
values only change in a major version (see README.md "Versioning").

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

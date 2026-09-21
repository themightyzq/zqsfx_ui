# Changelog

All notable changes to this project are documented here. Versioning is semver; token
values only change in a major version (see README.md "Versioning").

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

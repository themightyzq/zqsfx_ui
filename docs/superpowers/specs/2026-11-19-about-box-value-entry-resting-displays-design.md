# House wave 1 in the module: About box, typing values, resting displays

Date: 2026-11-19. Status: draft for ZQ's approval. Task: AT-27 (Hub tracker).
Rules this builds: [the House](../../../../docs/HOUSE.md), Behaviour section.

## Why

Someone who has learned one ZQ SFX Product should already know how to work every other one.
Today each Product has its own About box (eleven plain alert boxes and Broken's overlay). Only
b33p lets you type a knob value. Displays keep animating when the window is hidden or no sound
is playing. These three get built once in `zqsfx_ui`, so every Product picks them up the same way
in its pre-launch re-release.

## Goals

- **N1, typing a value.** Every knob takes a typed value. A right-click menu offers "Type a value…"
  and "Reset to default". Return on a focused knob opens the same entry box. Double-click still
  resets.
- **N2, resting displays.** A display that shows live sound stops animating when its window is
  hidden or the sound has been silent for a while, and shows the ZQ SFX mark instead.
- **N3, House About box.** One About box shows the mark, name, version, the delay the plugin
  reports, credits, the licence and the Update notice panel. It is coloured from the module's
  colours, which become the Product's Skin in wave 2.

## Not in this change

- **Fitting it into each Product.** That happens in each Product's own re-release Task (WaveEdit's
  is AT-37). This change ships the module, its tests and the demo.
- The Full / Low / Off animation setting (AT-41). The resting helper leaves room for it.
- Swappable Skins, the preset bar and the scale menu, which are wave 2 (AT-47).
- The host's own right-click items, such as a host's "automate this" entries. They could be added
  later through JUCE's host context menu.

## Approaches (Claude's picks, marked ✓)

**Typing a value**
- ✓ **Build it into `Dial`, plus a helper for a plain `juce::Slider`.** Ten of the thirteen
  Products already use `Dial` or `Knob`, so they get typing just by moving to the new module
  version. Products whose knob already has its own right-click menu (b33p) keep that menu and
  add the two House items with one call.
- An object each Product creates per knob. This would mean around 250 extra lines across the
  Products for the same result.

**About box**
- ✓ **A panel drawn inside the plugin window,** over a dimmed backdrop, the way Broken's overlay
  works now. A separate window can open behind the host's window. A plain alert box cannot hold
  the Update notice's buttons.
- A juce `AlertWindow` with a custom part inside. Its size and look are hard to control, and it
  is a separate window.

**Resting displays**
- ✓ **A small helper each display holds, plus one flag the audio code sets.** Products drive
  their displays in about a dozen different ways (a timer per editor, per display, or on the
  host's screen refresh). A helper fits all of them.
- A shared base class every display inherits from. This would mean rewriting each Product's
  display classes.

## Design

### N1: typing a value (`components/ValueEntry.h`, wired into `Dial`)

- Right-clicking a `Dial` (or Ctrl-click on a Mac) opens a menu with "Type a value…" and "Reset
  to default". "Reset to default" is greyed out when the knob has no default.
- "Type a value…" or Return on a focused knob opens a small text box over the knob. It holds the
  current value as the knob shows it, with all of the text selected.
- Return applies the value and Escape cancels. Clicking elsewhere applies it, the way host text
  fields do.
- **The text is read through the knob's own conversion**, which the parameter attachment installs.
  Entries such as "2 kHz", "2000" and "-6 dB" work wherever the parameter's text conversion
  accepts them.
- A value outside the range is limited to the range. Text the knob cannot read leaves the value
  unchanged.
- **Host automation sees one complete change** (begin, set, end), so an automation recording
  takes the typed value. A test proves this.
- `addValueEntryItems (PopupMenu&, Slider&)` adds the two items to a Product's own menu.
  `enableValueEntry (Slider&)` gives a plain `juce::Slider` the same behaviour.
- Accessibility: the text box is titled "Type a value for <knob name>". It returns focus to the
  knob when it closes.

### N2: resting displays (`components/RestingDisplay.h`)

- **Audio side:** `ActivityFlag`. `processBlock` calls `noteBlock (buffer)`, which keeps the
  block's peak in one atomic number. It does no allocation and takes no lock.
- **Window side:** `RestClock` reads that flag and decides whether the display should animate.
  - It rests when the display is not showing: hidden, behind a tab, a closed editor, or a
    minimised window.
  - It rests when the peak has stayed below −80 dBFS for 2 seconds. The two seconds let tails
    and decays finish.
  - It wakes on the first loud block, within about a quarter of a second.
- While resting, the display's own timer stops. A quiet 4 Hz check stays on to notice the sound
  coming back.
- `drawRestingMark (Graphics&, area)` draws the mark centred, small and dim, in the module's
  caption colour.
- **Claude's call:** displays of live sound (meters, scopes, spectra, waveforms) show the mark
  while resting. Displays of settings (an envelope shape, a filter curve, an LFO shape) stop
  animating but keep drawing the settings, because a person still needs to see their settings
  when nothing is playing.
- A resting display's accessible description says "No sound".

### N3: the House About box (`components/AboutBox.h`)

- **What a Product passes in:**
  - its name and version;
  - its Update notice product id;
  - credit lines (fonts, knob art);
  - the source-code link;
  - the processor, if it is a plugin.
- **What the box shows, top to bottom:**
  - the mark;
  - the name in Barlow Condensed;
  - the version, in IBM Plex Mono;
  - for plugins only, a delay row such as "Delay: 512 samples (10.7 ms at 48 kHz)", or
    "Delay: none". The row is read live, so it is correct after the sample rate changes;
  - the credit lines;
  - the licence line: "GPL-3.0-or-later", with a source-code link;
  - zq-sfx.com and connect@zq-sfx.com;
  - the Update notice panel;
  - Close.
- **Opening and closing:**
  - It opens from the `LogoMark` click that Products already have.
  - Escape, Close and a click on the dimmed backdrop close it.
  - Keyboard focus moves into the box and goes back to the mark when it closes.
- **Apps** use the same box without the delay row.
- **Size:** at least 260 px wide, so the Update notice panel's narrow layout works. It scales
  with the window.

### Also in this change

- Add the Hub item from AT-27: `~/Library/Application Support/ZQ SFX/Updates/` goes in the
  uninstaller's Remove Presets Too list (`installer/userdata.txt`), with accept and refuse test
  lines.
- The demo app shows all three. The snapshot tool renders the About box for one plugin and one
  app, so ZQ can approve the look from a picture.
- Update CHANGELOG, README and the style guide section on the mark. The module version stays
  0.7.0, which is still untagged.

## Tests (each seen failing once before it passes)

- **Typing a value:**
  - "2 kHz" lands on 2000.
  - Out-of-range text is limited to the range.
  - Unreadable text and Escape leave the value unchanged.
  - Reset returns to the default.
  - A typed value reaches the parameter as one begin/set/end change.
  - Return opens the text box.
  - The two menu items exist.
- **Resting displays:**
  - Silence for 2 seconds rests the display, and a loud block wakes it.
  - A hidden display rests.
  - `noteBlock` never allocates. This is checked under an allocation counter.
- **About box:**
  - The delay text is right for 0, 512 and 1000 samples at 44.1, 48 and 96 kHz.
  - Apps have no delay row.
  - The Update notice panel is present.
  - Escape closes the box and focus returns to the mark.
  - Every control has a title and a tooltip.
- **The existing checks:** the token contrast checks stay green, and the Hub's installer tests
  pass with the new line.

## Left to the builder

- Exact sizes and spacing, as long as they follow the style guide's spacing and the 22 px hit
  targets.
- How the text box looks: the module's screen style, as long as it reads at AA contrast.

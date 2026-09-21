# House knob strips: provenance

All three come from the KnobGallery at <https://www.g200kg.com/en/webknobman/gallery.php>,
where each upload carries a licence chosen by its uploader. All three are marked **CC0**
(public domain dedication): no attribution required, redistribution and modification allowed.
Credit is given here anyway. The licence is as declared by the uploader; the gallery does not
verify it. Downloaded 2026-09-21. The gallery's own records at download time:

| Strip | Dials | Gallery id | File | Author (as listed) | Licence | Uploaded |
|---|---|---|---|---|---|---|
| `strip_a.png` | 56 px and up | #2638 | `knob_large_silver.knob` | SolurOathLabs | CC0 | 2026-06-04 |
| `strip_b.png` | 42 px and up | #2410 | `knob_gallery_2410.knob` | dh96 | CC0 | see gallery |
| `strip_c.png` | under 42 px | #2075 | `knob_gallery_2075.knob` | C. Anders | CC0 | see gallery |

The original `.knob` source files are kept unmodified in `source/` so the strips can be
re-rendered or recoloured in KnobMan / WebKnobMan (<https://www.g200kg.com/en/webknobman/>).

## How the strips were rendered

WebKnobMan 0.96, vertical strip, **128 frames**, at each design's **native size** (95, 83, and
120 px square). Do not change the output size when re-rendering: these designs use pixel offsets
and their layers drift apart when scaled (tried at 128 px, it broke #2410).

One change was made at render time, and only to the render, not to the file in `source/`:

- **#2075**: its pointer layer (Layer16) animates the angle from -180 to +180 degrees, a full
  turn, so the minimum and maximum frames looked identical. It was rendered with that layer set
  to **-135 to +135** to match the 270 degree sweep every other control uses. To reproduce: open
  the file, select Layer16, Effects, set Angle from -135 and to 135, then Export.

## Normalising `strip_c`

#2075 leaves a wide transparent margin around the knob for its drop shadow: the body fills only
55 percent of the 120 px frame, where the other two fill about 85 percent, so at the same dial
size it looked much smaller. `strip_c.png` is therefore the 120 px render cropped to a centred
80 px frame with the outer 5 px of alpha feathered (so the shadow fades instead of being cut):

    python3 tools/normalise_knob_strip.py assets/knobs/rendered/strip_c_2075_120px_270deg.png assets/knobs/strip_c.png 80 5

The untouched 120 px render is kept in `rendered/`.

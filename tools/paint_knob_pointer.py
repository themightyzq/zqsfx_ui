#!/usr/bin/env python3
"""Paint a bright pointer line onto every frame of a vertical knob filmstrip.

For designs whose own pointer is too faint to read. The line is drawn at the angle each frame
represents (frame 0 = start angle, last frame = end angle, 0 degrees = straight up, clockwise
positive), supersampled 4x so it is antialiased, and composited over the original frame.

    paint_knob_pointer.py <in.png> <out.png> <r_inner> <r_outer> <width_px> [RRGGBB] [start_deg] [end_deg]

Radii and width are in pixels of the source frame, measured from the frame centre.
Needs Pillow.
"""
import math
import sys
from PIL import Image, ImageDraw

src, dst = sys.argv[1], sys.argv[2]
r0, r1, width = float(sys.argv[3]), float(sys.argv[4]), float(sys.argv[5])
rgb = sys.argv[6] if len(sys.argv) > 6 else "DED6C2"           # zqsfx::ui::colour::pointer
a0 = float(sys.argv[7]) if len(sys.argv) > 7 else -135.0
a1 = float(sys.argv[8]) if len(sys.argv) > 8 else 135.0
colour = tuple(int(rgb[i:i + 2], 16) for i in (0, 2, 4))

im = Image.open(src).convert("RGBA")
w = im.size[0]
frames = im.size[1] // w
assert im.size[1] == frames * w, "not a vertical strip of square frames"

SS = 4
c = w * SS / 2.0
out = Image.new("RGBA", im.size)
for f in range(frames):
    frame = im.crop((0, f * w, w, (f + 1) * w))
    ang = math.radians(a0 + (a1 - a0) * (f / (frames - 1) if frames > 1 else 0.0))
    dx, dy = math.sin(ang), -math.cos(ang)
    layer = Image.new("RGBA", (w * SS, w * SS), (0, 0, 0, 0))
    d = ImageDraw.Draw(layer)
    p0 = (c + dx * r0 * SS, c + dy * r0 * SS)
    p1 = (c + dx * r1 * SS, c + dy * r1 * SS)
    lw = max(1, round(width * SS))
    # soft dark keyline under the pointer so it reads on a bright metal cap
    d.line([p0, p1], fill=(0, 0, 0, 150), width=lw + 2 * SS)
    d.line([p0, p1], fill=colour + (255,), width=lw)
    for p in (p0, p1):                                          # round caps
        d.ellipse([p[0] - lw / 2, p[1] - lw / 2, p[0] + lw / 2, p[1] + lw / 2], fill=colour + (255,))
    layer = layer.resize((w, w), Image.LANCZOS)
    out.paste(Image.alpha_composite(frame, layer), (0, f * w))

out.save(dst, optimize=True)
print(f"{dst}: {w}x{w}, {frames} frames, pointer {a0:g}..{a1:g} deg")

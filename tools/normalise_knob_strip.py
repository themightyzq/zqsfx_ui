#!/usr/bin/env python3
"""Crop a vertical knob filmstrip so the knob body fills its frame like the other house strips.

Some KnobGallery designs leave a wide transparent margin (for a drop shadow), so at the same
dial size the knob looks much smaller than its neighbours. This crops every frame to a centred
square and feathers the outer few pixels of alpha, so a soft shadow fades out instead of being
cut by the new frame edge. The rotation centre is unchanged (the crop is symmetric).

    normalise_knob_strip.py <in.png> <out.png> <new_frame_px> [feather_px]

Needs Pillow and numpy.
"""
import sys
import numpy as np
from PIL import Image

src, dst, new = sys.argv[1], sys.argv[2], int(sys.argv[3])
feather = int(sys.argv[4]) if len(sys.argv) > 4 else 5

im = Image.open(src).convert("RGBA")
w = im.size[0]
frames = im.size[1] // w
assert im.size[1] == frames * w, "not a vertical strip of square frames"
assert new <= w and (w - new) % 2 == 0, "new frame must be smaller than the old one by an even amount"
m = (w - new) // 2

a = np.asarray(im).reshape(frames, w, w, 4)[:, m:m + new, m:m + new, :].copy()

# distance (in px) of each pixel from the nearest frame edge, 0 at the edge
idx = np.arange(new)
edge = np.minimum(idx, new - 1 - idx)
dist = np.minimum.outer(edge, edge).astype(np.float32)
ramp = np.clip(dist / float(feather), 0.0, 1.0)
a[..., 3] = (a[..., 3].astype(np.float32) * ramp).round().astype(np.uint8)

Image.fromarray(a.reshape(frames * new, new, 4), "RGBA").save(dst, optimize=True)
print(f"{dst}: {new}x{new}, {frames} frames")

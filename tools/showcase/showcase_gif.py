#!/usr/bin/env python3
"""SB_SHOWCASE frames -> a looping GIF for the itch page.

usage: showcase_gif.py <frames dir> <out.gif> [width] [step] [first] [last]
Frames are saved at 30 fps; step 2 gives a 15 fps GIF.
"""
import sys
from pathlib import Path
from PIL import Image

src, out = Path(sys.argv[1]), sys.argv[2]
width = int(sys.argv[3]) if len(sys.argv) > 3 else 640
step = int(sys.argv[4]) if len(sys.argv) > 4 else 2
files = sorted(src.glob("f*.png"))
first = int(sys.argv[5]) if len(sys.argv) > 5 else 0
last = int(sys.argv[6]) if len(sys.argv) > 6 else len(files)
files = files[first:last:step]

frames = []
for f in files:
    im = Image.open(f).convert("RGB")
    h = round(im.height * width / im.width)
    frames.append(im.resize((width, h), Image.LANCZOS))
# One palette for every frame: no colour shimmer between frames.
pal = frames[len(frames) // 2].quantize(colors=128, method=Image.MEDIANCUT)
q = [fr.quantize(palette=pal, dither=Image.NONE) for fr in frames]
q[0].save(out, save_all=True, append_images=q[1:], duration=round(1000 * step / 30), loop=0, optimize=True)
print(out, len(q), "frames", Path(out).stat().st_size // 1024, "KB")

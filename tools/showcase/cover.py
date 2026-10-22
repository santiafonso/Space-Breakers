#!/usr/bin/env python3
"""A showcase frame -> the itch cover (630x500) with the title on it.

usage: cover.py <frame.png> <out.png> [font dir] [zoom] [core x]
zoom > 1 cuts closer; core x is where the core sits across the cover (0..1),
so the busy side of the arena can fill the rest. The core sits low, under the title.
"""
import sys
from PIL import Image, ImageDraw, ImageFilter, ImageFont

frame, out = sys.argv[1], sys.argv[2]
fonts = sys.argv[3] if len(sys.argv) > 3 else "assets/fonts"
zoom = float(sys.argv[4]) if len(sys.argv) > 4 else 1.0
core_x = float(sys.argv[5]) if len(sys.argv) > 5 else 0.5
W, H = 630, 500
im = Image.open(frame).convert("RGB")
# The arena without the HUD bands, cut to the cover's shape around the core.
band = 38
ch = round((im.height - 2 * band) / zoom)
cw = round(ch * W / H)
x0 = max(0, min(im.width - cw, round(im.width / 2 - core_x * cw)))
top = max(band, min(im.height - band - ch, round(im.height / 2 - 0.6 * ch)))
im = im.crop((x0, top, x0 + cw, top + ch)).resize((W, H), Image.LANCZOS)

# Darken the top so the title reads.
shade = Image.new("L", (W, H))
d = ImageDraw.Draw(shade)
for y in range(H):
    d.line([(0, y), (W, y)], fill=int(200 * max(0.0, 1 - y / 220)))
im = Image.composite(Image.new("RGB", (W, H), (8, 11, 18)), im, shade)

draw = ImageDraw.Draw(im)
title = ImageFont.truetype(f"{fonts}/Lato-Black.ttf", 64)
sub = ImageFont.truetype(f"{fonts}/Lato-Bold.ttf", 22)
def centered(y, text, font, fill, glow=None):
    w = draw.textlength(text, font=font)
    x = (W - w) / 2
    if glow:
        layer = Image.new("RGBA", (W, H))
        ImageDraw.Draw(layer).text((x, y), text, font=font, fill=glow)
        layer = layer.filter(ImageFilter.GaussianBlur(10))
        im.paste(layer, (0, 0), layer)
    draw.text((x, y), text, font=font, fill=fill)
centered(34, "SPACE BREAKERS", title, (244, 236, 214), glow=(232, 190, 96, 150))
centered(112, "Defend the core. Fling. Break the waves.", sub, (170, 182, 200))
im.save(out)
print(out)

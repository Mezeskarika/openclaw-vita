#!/usr/bin/env python3
"""Generates the placeholder LiveArea images in vita/sce_sys/.

The Vita only accepts 8-bit indexed PNGs here, hence the quantize() step.
Uses the game's console font. Replace the output with real artwork before a release.
"""
import os
import sys

from PIL import Image, ImageDraw, ImageFont

BACKGROUND = (92, 22, 18)
TEXT = (240, 196, 64)

# (path relative to vita/sce_sys, size, font size)
IMAGES = [
    ("icon0.png", (128, 128), 26),
    ("livearea/contents/bg.png", (840, 500), 96),
    ("livearea/contents/startup.png", (280, 158), 40),
]


def make_image(path, size, font_size, font_path):
    image = Image.new("RGB", size, BACKGROUND)
    draw = ImageDraw.Draw(image)
    font = ImageFont.truetype(font_path, font_size)
    draw.text((size[0] / 2, size[1] / 2), "OpenClaw", fill=TEXT, font=font, anchor="mm")
    image.quantize(colors=256).save(path, optimize=True)


def main():
    vita_dir = os.path.dirname(os.path.abspath(__file__))
    sce_sys = os.path.join(vita_dir, "sce_sys")
    font_path = os.path.join(vita_dir, "..", "Build_Release", "clacon.ttf")
    for rel_path, size, font_size in IMAGES:
        path = os.path.join(sce_sys, rel_path)
        os.makedirs(os.path.dirname(path), exist_ok=True)
        make_image(path, size, font_size, font_path)
        print("wrote", path)
    return 0


if __name__ == "__main__":
    sys.exit(main())

#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
"""Rasterize the small Bravura symbol subset used by the FM-1 score renderer."""

import argparse
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


SYMBOLS = {
    "TREBLE": 0xE050,
    "BASS": 0xE062,
    "WHOLE": 0xE0A2,
    "QUARTER": 0xE0A4,
    "TRIPLE_FLAT": 0xE266,
    "DOUBLE_FLAT": 0xE264,
    "FLAT": 0xE260,
    "SHARP": 0xE262,
    "DOUBLE_SHARP": 0xE263,
    "TRIPLE_SHARP": 0xE265,
    "NATURAL": 0xE261,
}


def rasterize(font, codepoint):
    glyph = chr(codepoint)
    left, top, right, bottom = font.getbbox(glyph, anchor="ls")
    mask = Image.new("L", (right - left, bottom - top))
    ImageDraw.Draw(mask).text((-left, -top), glyph, anchor="ls", font=font, fill=255)
    return top, mask


def packed_rows(mask):
    width, height = mask.size
    pixels = mask.load()
    data = []
    for y in range(height):
        for x in range(0, width, 2):
            high = (pixels[x, y] + 8) // 17
            low = (pixels[x + 1, y] + 8) // 17 if x + 1 < width else 0
            data.append((high << 4) | low)
    for i in range(0, len(data), 12):
        yield "        " + ", ".join(f"0x{byte:02x}" for byte in data[i : i + 12]) + ","


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("font", help="Path to the upstream Bravura.otf")
    parser.add_argument("output", type=Path, help="Destination C header")
    args = parser.parse_args()

    regular = ImageFont.truetype(args.font, 24)
    lines = [
        "/* Bravura musical symbols, rasterized by tools/rasterize_staff.py.",
        " * Copyright (c) Steinberg Media Technologies GmbH. See dist/licenses/Bravura-OFL.txt. */",
        "enum StaffSymbol {",
        *(f"    SYMBOL_{name}," for name in SYMBOLS),
        "};",
        "static const StaffGlyph staff_glyphs[] = {",
    ]
    for name, codepoint in SYMBOLS.items():
        top, mask = rasterize(regular, codepoint)
        width, height = mask.size
        lines.append(f"    [SYMBOL_{name}] = {{{top}, {width}, {height}, (const uint8_t[]){{")
        lines.extend(packed_rows(mask))
        lines.append("    }},")
    lines.extend(["};", ""])
    args.output.write_text("\n".join(lines), encoding="ascii")


if __name__ == "__main__":
    main()

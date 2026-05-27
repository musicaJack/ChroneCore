#!/usr/bin/env python3
"""Rasterize icons/alarm.svg (Tabler Icons MIT) to 24x24 1-bit C header."""
from pathlib import Path

from PIL import Image, ImageDraw

SIZE = 24
SCALE = 8
OUT = Path(__file__).resolve().parent.parent / "include" / "chrone_alarm_icon_bitmap.h"


def main() -> None:
    img = Image.new("L", (SIZE * SCALE, SIZE * SCALE), 255)
    draw = ImageDraw.Draw(img)
    w = 2 * SCALE

    def s(v: float) -> float:
        return v * SCALE

    # Tabler outline/alarm.svg paths
    draw.ellipse([s(5), s(6), s(19), s(20)], outline=0, width=w)
    draw.line([s(12), s(10), s(12), s(13), s(14), s(13)], fill=0, width=w)
    draw.line([s(7), s(4), s(4.25), s(6)], fill=0, width=w)
    draw.line([s(17), s(4), s(19.75), s(6)], fill=0, width=w)

    img = img.resize((SIZE, SIZE), Image.Resampling.LANCZOS)
    px = img.load()
    flat: list[int] = []
    for y in range(SIZE):
        for bx in range(3):
            byte = 0
            for bit in range(8):
                x = bx * 8 + bit
                if x < SIZE:
                    if px[x, y] > 128:
                        byte |= 1 << (7 - bit)
            flat.append(byte)

    hexes = ", ".join(f"0x{b:02x}" for b in flat)
    text = f"""#pragma once
/* 24x24 1-bit alarm icon — rasterized from Tabler Icons outline/alarm (MIT)
 * https://github.com/tabler/tabler-icons/blob/main/icons/outline/alarm.svg
 */
#define CHRONE_ALARM_ICON_SZ 24

static const uint8_t chrone_alarm_icon_bitmap_24[{SIZE * 3}] = {{
  {hexes}
}};
"""
    OUT.write_text(text, encoding="utf-8")
    print("wrote", OUT)


if __name__ == "__main__":
    main()

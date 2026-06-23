#!/usr/bin/env python3
"""
png2zx.py -- Convert PNG images to ZX Spectrum binary formats.

Modes:
  scr  (default) : 256x192 PNG -> assets/scr/loading.scr (6912 bytes)
  gfx  --gfx <png>... : 8x8 / 16x16 PNGs -> src/assets_gfx.{c,h}

Generated -- do not edit by hand.
"""

import argparse
import os
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# ZX Spectrum colour palette (index 0-7)
# PAPER/INK bits in attribute byte use these 3-bit values.
# ---------------------------------------------------------------------------
ZX_PALETTE = [
    (0,   0,   0),    # 0 BLACK
    (0,   0,   215),  # 1 BLUE
    (215, 0,   0),    # 2 RED
    (215, 0,   215),  # 3 MAGENTA
    (0,   215, 0),    # 4 GREEN
    (0,   215, 215),  # 5 CYAN
    (215, 215, 0),    # 6 YELLOW
    (215, 215, 215),  # 7 WHITE
]

REPO_ROOT = Path(__file__).resolve().parent.parent


def nearest_zx_colour(r: int, g: int, b: int) -> int:
    """Return the ZX colour index (0-7) nearest to (r, g, b)."""
    best_idx = 0
    best_dist = float("inf")
    for idx, (pr, pg, pb) in enumerate(ZX_PALETTE):
        dist = (r - pr) ** 2 + (g - pg) ** 2 + (b - pb) ** 2
        if dist < best_dist:
            best_dist = dist
            best_idx = idx
    return best_idx


# ---------------------------------------------------------------------------
# .scr BITMAP INTERLEAVE
# offset for pixel (x, y), x in 0..255, y in 0..191
# Formula from zx-arch skill / ZX Spectrum hardware reference:
#   offset = ((y & 0xC0) << 5) | ((y & 0x07) << 8) | ((y & 0x38) << 2) | (x >> 3)
# ---------------------------------------------------------------------------

def scr_bitmap_offset(x: int, y: int) -> int:
    return ((y & 0xC0) << 5) | ((y & 0x07) << 8) | ((y & 0x38) << 2) | (x >> 3)


def _interleave_spot_check() -> None:
    """
    Internal self-test: synthesize a 256x192 all-black image, set one known
    pixel, convert to .scr bitmap bytes, and assert it lands at the expected
    interleaved offset.

    Test pixel: x=8, y=10  (second byte of row 10, bit 7 / leftmost)
    Expected offset:
      y=10 -> 0b00001010
        (y & 0xC0) = 0x00 -> <<5 = 0x0000
        (y & 0x07) = 0x02 -> <<8 = 0x0200
        (y & 0x38) = 0x08 -> <<2 = 0x0020
      x=8  -> x>>3 = 1
      offset = 0x0000 | 0x0200 | 0x0020 | 0x0001 = 0x0221 = 545
    The bit within that byte: bit 7 (0x80), because x&7 = 0.
    """
    from PIL import Image as _Image

    img = _Image.new("RGB", (256, 192), (0, 0, 0))
    px = img.load()

    test_x, test_y = 8, 10
    px[test_x, test_y] = (215, 215, 215)  # WHITE

    bitmap = _png_to_scr_bitmap(img)

    expected_offset = scr_bitmap_offset(test_x, test_y)
    assert expected_offset == 0x0221, (
        f"offset formula broken: got {expected_offset:#x}, expected 0x0221"
    )

    byte_val = bitmap[expected_offset]
    expected_bit = 0x80 >> (test_x & 7)  # x&7=0 -> bit7 -> 0x80
    assert (byte_val & expected_bit) != 0, (
        f"Spot-check FAILED: bitmap[0x{expected_offset:04x}]=0x{byte_val:02x}, "
        f"expected bit {expected_bit:#x} to be set"
    )


def _png_to_scr_bitmap(img) -> bytearray:
    """
    Convert a 256x192 PIL Image to 6144-byte interleaved ZX bitmap.
    Each 8x8 cell is quantized to 2 colours (INK = majority, PAPER = minority /
    background). The INK colour is the one used for the 'on' bits.
    """
    from PIL import Image

    if img.size != (256, 192):
        img = img.resize((256, 192), Image.LANCZOS)
    img = img.convert("RGB")
    px = img.load()

    bitmap = bytearray(6144)

    for row in range(24):       # 24 character rows
        for col in range(32):   # 32 character columns
            # Collect pixels for this 8x8 cell
            cell_pixels = []
            for cy in range(8):
                for cx in range(8):
                    cell_pixels.append(px[col * 8 + cx, row * 8 + cy])

            # Quantize each pixel to ZX palette
            zx_indices = [nearest_zx_colour(r, g, b) for r, g, b in cell_pixels]

            # Determine INK (most frequent non-background colour).
            # PAPER = the colour with most occurrences (background).
            # INK   = the other colour (or PAPER if cell is uniform).
            from collections import Counter
            counts = Counter(zx_indices)
            most_common = counts.most_common()
            paper_idx = most_common[0][0]
            ink_idx = most_common[1][0] if len(most_common) > 1 else paper_idx

            # Build 8 bitmap bytes for this cell
            for cy in range(8):
                y = row * 8 + cy
                byte_val = 0
                for cx in range(8):
                    x = col * 8 + cx
                    pixel_zx = zx_indices[cy * 8 + cx]
                    # Bit is SET when pixel matches INK colour
                    if pixel_zx == ink_idx and ink_idx != paper_idx:
                        byte_val |= (0x80 >> cx)
                    elif ink_idx == paper_idx:
                        # Uniform cell: set all bits (solid ink = paper colour)
                        byte_val |= (0x80 >> cx)
                offset = scr_bitmap_offset(x, y)
                bitmap[offset] = byte_val

    return bitmap


def _png_to_scr_attrs(img) -> bytearray:
    """
    Convert a 256x192 PIL Image to 768-byte linear attribute region.
    Attribute byte: bit7=FLASH bit6=BRIGHT bits5-3=PAPER bits2-0=INK.
    """
    from PIL import Image
    from collections import Counter

    if img.size != (256, 192):
        img = img.resize((256, 192), Image.LANCZOS)
    img = img.convert("RGB")
    px = img.load()

    attrs = bytearray(768)

    for row in range(24):
        for col in range(32):
            cell_pixels = []
            for cy in range(8):
                for cx in range(8):
                    cell_pixels.append(px[col * 8 + cx, row * 8 + cy])

            zx_indices = [nearest_zx_colour(r, g, b) for r, g, b in cell_pixels]

            counts = Counter(zx_indices)
            most_common = counts.most_common()
            paper_idx = most_common[0][0]
            ink_idx = most_common[1][0] if len(most_common) > 1 else paper_idx

            attr_byte = (paper_idx << 3) | ink_idx
            attrs[row * 32 + col] = attr_byte

    return attrs


def cmd_scr(args) -> int:
    """Convert assets/png/loading.png -> assets/scr/loading.scr."""
    try:
        from PIL import Image
    except ImportError:
        print("ERROR: Pillow not installed. Run: pip install Pillow", file=sys.stderr)
        return 1

    src_png = REPO_ROOT / "assets" / "png" / "loading.png"
    dst_scr = REPO_ROOT / "assets" / "scr" / "loading.scr"

    if not src_png.exists():
        print(f"ERROR: {src_png} not found. Run gen_assets.py first.", file=sys.stderr)
        return 1

    dst_scr.parent.mkdir(parents=True, exist_ok=True)

    img = Image.open(src_png)
    bitmap = _png_to_scr_bitmap(img)
    attrs = _png_to_scr_attrs(img)

    scr_data = bytes(bitmap) + bytes(attrs)
    assert len(scr_data) == 6912, f"BUG: scr_data is {len(scr_data)} bytes, expected 6912"

    dst_scr.write_bytes(scr_data)
    print(f"Wrote {len(scr_data)} bytes -> {dst_scr}")

    # Emit src/loading_scr.h and src/loading_scr.c
    src_dir = REPO_ROOT / "src"
    src_dir.mkdir(parents=True, exist_ok=True)

    banner = "/* generated -- do not edit by hand */\n"

    # Header
    h_path = src_dir / "loading_scr.h"
    h_lines = [
        banner,
        "#ifndef LOADING_SCR_H\n",
        "#define LOADING_SCR_H\n",
        "\n",
        "#include <stdint.h>\n",
        "\n",
        "extern const uint8_t loading_scr[6912];\n",
        "\n",
        "#endif /* LOADING_SCR_H */\n",
    ]
    h_path.write_text("".join(h_lines))
    print(f"Wrote {h_path}")

    # Source: emit 16 hex bytes per line for readability
    c_path = src_dir / "loading_scr.c"
    hex_rows = []
    for i in range(0, 6912, 16):
        chunk = scr_data[i:i + 16]
        hex_rows.append("    " + ", ".join(f"0x{b:02x}" for b in chunk))
    c_body = ",\n".join(hex_rows)
    c_lines = [
        banner,
        '#include "loading_scr.h"\n',
        "\n",
        "const uint8_t loading_scr[6912] = {\n",
        c_body,
        "\n};\n",
    ]
    c_path.write_text("".join(c_lines))
    print(f"Wrote {c_path}")

    return 0


# ---------------------------------------------------------------------------
# GFX converter: 8x8 or 16x16 PNGs -> src/assets_gfx.{c,h}
# ---------------------------------------------------------------------------

def _image_to_udg_bytes(img, name: str):
    """
    Convert an 8x8 or 16x16 PIL image to a list of (array_name, bytes) tuples.

    8x8  -> one array of 8 bytes (one per row, MSB = leftmost pixel).
    16x16 -> four arrays of 8 bytes in sub-cell order:
             top-left, top-right, bottom-left, bottom-right.
    The INK colour is the most frequent non-background colour.
    Each bit is SET when the pixel matches INK (foreground).
    """
    from PIL import Image
    from collections import Counter

    img = img.convert("RGB")
    px = img.load()
    w, h = img.size

    if w == 8 and h == 8:
        cells = [("", 0, 0)]
    elif w == 16 and h == 16:
        # sub-cell order: top-left, top-right, bottom-left, bottom-right
        cells = [("_tl", 0, 0), ("_tr", 8, 0), ("_bl", 0, 8), ("_br", 8, 8)]
    else:
        raise ValueError(f"Unsupported sprite size {w}x{h}: must be 8x8 or 16x16")

    result = []
    for suffix, ox, oy in cells:
        cell_pixels = []
        for cy in range(8):
            for cx in range(8):
                cell_pixels.append(px[ox + cx, oy + cy])

        zx_indices = [nearest_zx_colour(r, g, b) for r, g, b in cell_pixels]
        counts = Counter(zx_indices)
        most_common = counts.most_common()
        paper_idx = most_common[0][0]
        ink_idx = most_common[1][0] if len(most_common) > 1 else paper_idx

        row_bytes = []
        for cy in range(8):
            byte_val = 0
            for cx in range(8):
                pixel_zx = zx_indices[cy * 8 + cx]
                if pixel_zx == ink_idx and ink_idx != paper_idx:
                    byte_val |= (0x80 >> cx)
                elif ink_idx == paper_idx:
                    byte_val |= (0x80 >> cx)
            row_bytes.append(byte_val)

        array_name = f"gfx_{name}{suffix}"
        result.append((array_name, bytes(row_bytes)))

    return result


def cmd_gfx(args) -> int:
    """Convert one or more 8x8/16x16 PNGs -> src/assets_gfx.{c,h}."""
    try:
        from PIL import Image
    except ImportError:
        print("ERROR: Pillow not installed. Run: pip install Pillow", file=sys.stderr)
        return 1

    src_dir = REPO_ROOT / "src"
    src_dir.mkdir(parents=True, exist_ok=True)
    out_c = src_dir / "assets_gfx.c"
    out_h = src_dir / "assets_gfx.h"

    all_arrays = []  # list of (array_name, bytes)
    for png_path_str in args.gfx:
        png_path = Path(png_path_str)
        if not png_path.exists():
            print(f"ERROR: {png_path} not found", file=sys.stderr)
            return 1
        name = png_path.stem.replace("-", "_").replace(" ", "_")
        img = Image.open(png_path)
        arrays = _image_to_udg_bytes(img, name)
        all_arrays.extend(arrays)

    guard = "ASSETS_GFX_H"
    header_banner = (
        "/* generated -- do not edit by hand */\n"
        "/* generated by tools/png2zx.py --gfx */\n"
    )

    # Write .h
    h_lines = [
        header_banner,
        f"#ifndef {guard}\n",
        f"#define {guard}\n",
        "\n",
        "#include <stdint.h>\n",
        "\n",
    ]
    for array_name, data in all_arrays:
        h_lines.append(f"extern const uint8_t {array_name}[{len(data)}];\n")
    h_lines.append(f"\n#endif /* {guard} */\n")

    out_h.write_text("".join(h_lines))

    # Write .c
    c_lines = [
        header_banner,
        f'#include "assets_gfx.h"\n',
        "\n",
    ]

    if any(len(arrays) == 4 for _, arrays in [(None, all_arrays)]):
        # Emit sub-cell order comment for 16x16 sprites
        pass

    for array_name, data in all_arrays:
        size = len(data)
        hex_bytes = ", ".join(f"0x{b:02x}" for b in data)
        c_lines.append(f"/* 8-byte UDG row data, MSB = leftmost pixel */\n")
        if "_tl" in array_name:
            c_lines.append(
                "/* 16x16 sprite sub-cell order: top-left, top-right, "
                "bottom-left, bottom-right */\n"
            )
        c_lines.append(
            f"const uint8_t {array_name}[{size}] = {{{hex_bytes}}};\n\n"
        )

    out_c.write_text("".join(c_lines))

    print(f"Wrote {out_h} ({out_h.stat().st_size} bytes)")
    print(f"Wrote {out_c} ({out_c.stat().st_size} bytes)")
    return 0


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main() -> int:
    # Run the interleave spot-check on every invocation (fast, no I/O needed).
    _interleave_spot_check()

    parser = argparse.ArgumentParser(
        description="Convert PNG images to ZX Spectrum binary formats."
    )
    parser.add_argument(
        "--gfx",
        nargs="+",
        metavar="PNG",
        help="GFX mode: convert 8x8/16x16 PNGs to src/assets_gfx.{c,h}",
    )
    args = parser.parse_args()

    if args.gfx:
        return cmd_gfx(args)
    else:
        return cmd_scr(args)


if __name__ == "__main__":
    sys.exit(main())

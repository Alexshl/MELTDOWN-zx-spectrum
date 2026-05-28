---
name: zx-arch
description: Reference for ZX Spectrum 48K/128K hardware architecture — memory map, screen layout (bitmap + attribute file), color attribute encoding, keyboard matrix on port 0xFE, beeper, AY-3-8912 sound (128K), 50 Hz interrupt, and 128K bank switching via port 0x7FFD. Invoke when planning or implementing any low-level Spectrum behaviour, when verifying that a hardware-related claim is correct, or when a z88dk helper abstracts something whose underlying semantics you need to understand. Facts here are from authoritative sources (worldofspectrum.org, sinclair.wiki.zxnet.co.uk, breakintoprogram.co.uk) — when in doubt, follow the linked references rather than inventing.
---

# ZX Spectrum architecture — quick reference

This reference covers **the parts most projects need** on 48K/128K targets. Advanced topics (contended-memory timing, ULA snow, AY envelopes, +3 disk) are behind the links at the end.

## 1. Memory map

### 48K Spectrum
| Address           | Size  | Contents                                  |
|-------------------|-------|-------------------------------------------|
| `0x0000–0x3FFF`   | 16 KB | ROM (BASIC interpreter)                   |
| `0x4000–0x57FF`   | 6 KB  | Screen bitmap (192 lines × 32 bytes)      |
| `0x5800–0x5AFF`   | 768 B | Attribute file (32 × 24)                  |
| `0x5B00–0x5BFF`   | 256 B | Printer buffer / system use               |
| `0x5C00–0x5CBF`   | 192 B | System variables                          |
| `0x5CC0–0xFFFF`   | ~42 KB| BASIC / user RAM                          |

### 128K Spectrum
Same first 32 KB, **plus** banks 0..7 (16 KB each), paged into the `0xC000–0xFFFF` window via port `0x7FFD`:

| 0x0000–0x3FFF | ROM (bit 4 of 0x7FFD selects: 0 = 128 editor, 1 = 48 BASIC) |
| 0x4000–0x7FFF | bank 5 (normal screen)                                       |
| 0x8000–0xBFFF | bank 2                                                       |
| 0xC000–0xFFFF | one of banks 0..7 (bits 0-2 of 0x7FFD)                       |

Bit 3 of 0x7FFD: 0 = normal screen (bank 5), 1 = shadow screen (bank 7).
Bit 5: "lock" — once set to 1, the port ignores writes until the machine is reset.

**Important**: when paging you must disable interrupts (`di`) and keep the stack outside the area that changes. z88dk `+zx` (subtype zx-128) can work with banks via `mem128_push_di()` / `mem128_pop_ei()` — see the z88dk header `<arch/zx/spectrum.h>`.

Many projects fit entirely in base 48K RAM and never need bank switching. The extra banks become useful for AY music, large lookup tables, or double-buffered/shadow-screen tricks.

## 2. Screen bitmap layout (0x4000)

256 × 192 pixels. 6144 bytes. Addressing is **non-linear** — this matters for direct writes.

The Y-address structure (bits 5..12 of the pixel address) for a pixel coordinate `y` (0..191):

```
y = y7 y6 y5 y4 y3 y2 y1 y0   (y7 is always 0, since 192 < 256)
       │  │  │  │  │  │  │
       Y2 Y1 Y0 Y5 Y4 Y3 Y2 Y1 Y0  ← mnemonic for the address bits
```

More precisely, the **full address of the first byte** for a pixel `(x, y)`:

```c
addr = 0x4000
     | ((y & 0xC0) << 5)        // top 2 bits of Y — selects screen third
     | ((y & 0x07) << 8)        // bottom 3 bits of Y — line within the char cell
     | ((y & 0x38) << 2)        // middle 3 bits of Y — char-row within the third
     |  (x >> 3);               // X in bytes (8 px/byte)
```

Bit within the byte: `0x80 >> (x & 7)` (MSB = leftmost pixel).

**Practical consequence**: adjacent pixel rows within one character cell are 256 bytes apart, while adjacent cells horizontally are 1 byte apart. z88dk encapsulates this in:
- `zx_pxy2saddr(x, y)` — pixel coords → bitmap byte address
- `zx_cxy2saddr(cx, cy)` — char-cell coords (0..31, 0..23) → address of the cell's top pixel row
- `zx_cxy2aaddr(cx, cy)` — char-cell → attribute byte address

Source: [breakintoprogram.co.uk — Screen Memory Layout](http://www.breakintoprogram.co.uk/hardware/computers/zx-spectrum/screen-memory-layout), [Wikipedia — ZX Spectrum graphic modes](https://en.wikipedia.org/wiki/ZX_Spectrum_graphic_modes).

## 3. Attribute file (0x5800)

768 bytes, **linear**: `addr = 0x5800 + row * 32 + col`, where `row` ∈ [0..23], `col` ∈ [0..31].

One byte describes the color of an entire 8×8 character cell:

```
bit:  7      6      5    4    3    2    1    0
     FLASH BRIGHT P2   P1   P0   I2   I1   I0
                 └─── PAPER ──┘└──── INK ────┘
```

PAPER and INK are three bits each (8 colors):

| Code | Color    |
|------|----------|
| 0    | BLACK    |
| 1    | BLUE     |
| 2    | RED      |
| 3    | MAGENTA  |
| 4    | GREEN    |
| 5    | CYAN     |
| 6    | YELLOW   |
| 7    | WHITE    |

BRIGHT = 1 → bright variant (but BLACK stays black). FLASH = 1 → INK/PAPER swap ~1.6 times per second (in hardware).

**Attribute clash**: a single cell can hold only 2 colors at once (one INK + one PAPER). This is a defining trait of the Spectrum.

In z88dk the constants are `INK_BLACK`, `INK_BLUE`, ..., `PAPER_*`, `BRIGHT`, `FLASH` from `<arch/zx.h>`. Combine them with `|`.

## 4. Keyboard — port 0xFE

40 keys, organized as **8 half-rows of 5 keys**. To read: write a mask into the **high byte** of the I/O address, with the low byte = `0xFE`. Each bit of the high byte = "scan half-row N"; **0 = scan, 1 = don't scan**. In the response, bits 0..4 = key states (**0 = pressed**).

| High I/O byte | Half-row | Keys (bit 0 → bit 4)                       |
|---------------|----------|---------------------------------------------|
| `0xFE`        | 0        | CAPS SHIFT, Z, X, C, V                      |
| `0xFD`        | 1        | A, S, D, F, G                               |
| `0xFB`        | 2        | Q, W, E, R, T                               |
| `0xF7`        | 3        | 1, 2, 3, 4, 5                               |
| `0xEF`        | 4        | 0, 9, 8, 7, 6                               |
| `0xDF`        | 5        | P, O, I, U, Y                               |
| `0xBF`        | 6        | ENTER, L, K, J, H                           |
| `0x7F`        | 7        | SPACE, SYMBOL SHIFT, M, N, B                |

z88dk `<input.h>`: `in_key_pressed(IN_KEY_SCANCODE_o)` encapsulates this. Scancodes are macros of the form `IN_KEY_SCANCODE_<key>` (lowercase for letters, ENTER/SPACE uppercase).

Source: [breakintoprogram.co.uk — Keyboard](http://www.breakintoprogram.co.uk/hardware/computers/zx-spectrum/keyboard), [sinclair.wiki — ZX Spectrum ULA](https://sinclair.wiki.zxnet.co.uk/wiki/ZX_Spectrum_ULA).

## 5. Border & beeper — port 0xFE (write)

Writing to `0xFE`:

```
bit:  7  6  5    4    3   2   1   0
                EAR MIC  B2  B1  B0
                            └ BORDER ┘
```

- Bits 0-2: border color (0..7, no BRIGHT).
- Bit 3: MIC (tape write).
- Bit 4: **EAR / speaker**. Toggling it makes a click — that's the beeper.

In z88dk: `zx_border(color)` for the border, `bit_beep(pitch, duration)` or `bit_beepfx_di(...)` for sounds (they use the ROM routine at 0x03B5).

## 6. AY-3-8912 — sound chip (128K only)

3 square-wave channels + 1 noise channel + envelope. Controlled via 2 ports:

| Port      | Purpose                          |
|-----------|----------------------------------|
| `0xFFFD`  | Write: select register (0..14). Read: data of the current register. |
| `0xBFFD`  | Write: data into the selected register.                             |

14 registers: 6 for channel A/B/C frequencies, a noise register, the mixer, 3 volume registers, 3 for the envelope.

For beeper-only projects the AY is not used. When you do need it, z88dk has `ay_*` functions and the AYFX library for effects.

## 7. 50 Hz interrupt

Z80 in IM 1, interrupt on vsync = 50 Hz (PAL). By default the ROM ISR at `0x0038` increments the system `FRAMES` counter (`0x5C78` on 48K), handles the keyboard for BASIC, and returns.

`intrinsic_halt()` (z88dk) executes `HALT` — the CPU sleeps until the next interrupt. This is the right way to idle in a 50 Hz game loop: it doesn't burn the CPU and gives a stable timer.

One frame = 20 ms = 70,000 T-states on a 3.5 MHz Z80.

## 8. Z80 CPU (brief)

- 3.5 MHz (standard Spectrum clock).
- Registers: AF, BC, DE, HL, IX, IY, SP, PC + shadow AF', BC', DE', HL'.
- 8-bit data bus, 16-bit address bus (64 KB address space).
- Contended memory: when the ULA is reading the screen (0x4000–0x7FFF), CPU access there is delayed. This affects the timing of fine effects (border tricks), but not ordinary game code.

## 9. z88dk helpers — summary

The most commonly used helpers:

| z88dk function/macro              | What it does                                                    |
|-----------------------------------|-----------------------------------------------------------------|
| `zx_border(color)`                | Border color                                                    |
| `zx_cls(attr)`                    | Clear the screen with a given attribute                         |
| `zx_cxy2saddr(cx, cy)`            | Char cell (0..31, 0..23) → bitmap byte address                  |
| `zx_cxy2aaddr(cx, cy)`            | Char cell → attribute byte address                              |
| `zx_pxy2saddr(x, y)`              | Pixel coords → bitmap byte address                              |
| `intrinsic_halt()`                | Wait for the next vsync (HALT)                                  |
| `in_key_pressed(SCANCODE)`        | Returns 0 or non-zero — whether the key is pressed              |
| `bit_beep(pitch, duration)`       | Beeper: tone of a given pitch and duration (blocks the CPU)     |
| `printf("\x16<row><col>...")`     | ROM AT — positions text in char-cell coordinates               |
| `<arch/zx.h>`                     | Color constants: `INK_*`, `PAPER_*`, `BRIGHT`, `FLASH`         |
| `<input.h>`                       | Keyboard scancodes                                              |
| `<sound.h>`                       | Beeper and sound functions                                     |

**When in doubt** about a signature, open the header in the z88dk install (inside the container):
```bash
find $Z88DK/include -name 'zx.h' -o -name 'input.h' -o -name 'sound.h'
```

## 10. When to use this skill

Agents (especially `planner` and `coder`) should consult this document when a task touches:

- Direct addresses (`0x4000`, `0x5800`, `0xFE`, `0x7FFD`)
- Manual pixel- or attribute-address calculation
- Low-level keyboard reads (when a z88dk wrapper isn't available)
- Sound (beeper vs AY)
- Colors and attribute clash
- Frame timing, vsync, 50 Hz
- 128K bank switching

**If a fact in this document contradicts an external source**, follow the links below and verify via WebFetch. Spectrum hardware facts have been stable since 1982, but this text digest may contain a typo.

## Sources

- [breakintoprogram.co.uk — Screen Memory Layout](http://www.breakintoprogram.co.uk/hardware/computers/zx-spectrum/screen-memory-layout)
- [breakintoprogram.co.uk — Memory Map](http://www.breakintoprogram.co.uk/hardware/computers/zx-spectrum/memory-map)
- [breakintoprogram.co.uk — Keyboard](http://www.breakintoprogram.co.uk/hardware/computers/zx-spectrum/keyboard)
- [World of Spectrum — 128K Technical Information](https://worldofspectrum.org/faq/reference/128kreference.htm)
- [sinclair.wiki — Memory paging](https://sinclair.wiki.zxnet.co.uk/wiki/Memory_paging)
- [sinclair.wiki — ZX Spectrum ULA](https://sinclair.wiki.zxnet.co.uk/wiki/ZX_Spectrum_ULA)
- [Wikipedia — ZX Spectrum graphic modes](https://en.wikipedia.org/wiki/ZX_Spectrum_graphic_modes)
- [z88dk wiki — Sinclair ZX Spectrum platform](https://github.com/z88dk/z88dk/wiki/Platform---Sinclair-ZX-Spectrum)
- [z88dk Getting Started (newlib)](https://github.com/z88dk/z88dk/blob/master/doc/ZXSpectrumZSDCCnewlib_01_GettingStarted.md)

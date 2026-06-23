# MELTDOWN — project vision

A sci-fi **tower defense** for the **ZX Spectrum 128K**, built on `zx-framework`
(C → z88dk → `.tap`, headless tests in ZEsarUX).

> Note: this is the project's original vision. Per `CLAUDE.md`, do not edit it without an explicit
> request once created — subsequent deviations live in task completion notes.

## Pitch

You command the automated defences of a deep-space reactor. Waves of alien/robotic intruders breach the
perimeter and crawl along the corridors toward the core. Place and cycle defensive turrets along the path,
destroy intruders before they reach the core. Every intruder that breaks through raises reactor instability;
when stability hits zero the reactor goes into **MELTDOWN** (you lose). Survive all waves to hold the sector.

## Why this game, why this platform

Pure tower defense did **not** exist in the classic ZX era (the genre grew out of Warcraft 3 / Flash custom
maps, ~2002–2009), so it is genuinely fresh for the platform. Its closest ancestors in the ZX catalogue are
the "static defence" line — *Rampart*, *City Defence*, *Air Defence* — and the proto-RTS *Nether Earth*.
Modern design anchors: **Kingdom Rush**, **Bloons TD**, **Plants vs Zombies**. A recent homebrew,
**Full Spectrum Defence** (Sausageware, 128K, 2026), proves TD runs well on the Spectrum — we treat it as a
proof of concept, not a template (their theme is anti-virus; ours is reactor defence).

TD maps almost 1:1 onto the framework's sweet spot: a grid of character cells, integer-only math, few
on-screen entities (no attribute clash), and tick logic inside the 50 Hz frame loop.

## Core loop

`BUILD phase` (place turrets) → press ENTER to release the wave → `WAVE phase` (real-time: intruders crawl the
path, turrets auto-fire) → wave cleared → back to `BUILD` → … → all waves survived = **WIN**, stability 0 =
**MELTDOWN**. One map, ~6–8 waves for the MVP.

## Playfield

- Character-cell grid, 32×24 cells. Bottom 2–3 rows are the HUD (gold, reactor stability, wave #, selected
  turret). Playable map ≈ 30×18.
- One intruder = one cell (glyph + attribute). One turret = one cell. No moving projectile sprites in the MVP:
  a turret hit is instant (hitscan) shown as a brief flash / attribute change — this sidesteps attribute clash
  and Z80 sprite cost.
- The enemy route is a **fixed, pre-computed waypoint list** (entry → core). Turrets do **not** block the path;
  there is no A\* (out of scope).

## Entities (MVP)

Turrets (3 roles):
- `LASER` — fast, low damage, cheap, single target.
- `MISSILE` — slow, high damage, small splash, expensive.
- `TESLA` — medium, slows the target (control).

Intruders (3 types):
- `DRONE` — baseline HP/speed.
- `RUNNER` — fast, low HP.
- `BRUTE` — slow, armored, high HP.

## Controls (keyboard, `in_key_pressed`)

- Move cursor (highlighted cell): `Q` up, `A` down, `O` left, `P` right (classic Sinclair layout).
- Select turret type: `SPACE` — cycles `LASER → MISSILE → TESLA` (current type shown in HUD).
- Build: `M` — places the selected turret on the current cell if it is buildable and gold suffices; otherwise
  the placement is rejected.
- Release wave: `ENTER` — starts the assault during the BUILD phase.
- Out of scope (MVP): `S` sell, `U` upgrade, Kempston joystick.

## Economy (MVP model: bounty + start + wave-clear bonus)

`gold` (uint16) = starting capital + a fixed kill bounty per intruder (uint8 per type; tougher/heavier = more)
+ a flat bonus for clearing a wave. An intruder that reaches the core gives **no** bounty (double penalty).
Reactor stability (uint8, N units) acts as "lives" — it is not spent on purchases, only lost when intruders
break through. *Interest on savings and income-generating towers are out of scope (post-MVP upgrades).*

## Graphics pipeline (Gemini "Nano Banana" → ZX)

Art is generated with Google's Gemini image model ("Nano Banana") and converted to ZX formats by in-project
tooling. The API key lives in `.env` as `GEMINI_API_KEY` (gitignored); it never appears in code or logs.

- `tools/gen_assets.py` — `google-genai` client: reads `GEMINI_API_KEY`, generates PNGs (model
  `gemini-2.5-flash-image`, optionally `gemini-3-pro-image-preview`) from prompts in `assets/prompts/` into
  `assets/png/` (idempotent, cached by name).
- `tools/png2zx.py` — converter: PNG 256×192 → loading screen `.scr` (6912 bytes, ZX palette quantization,
  dithering); sprite/tile PNGs → C byte arrays (8×8 UDGs and 16×16 = 2×2-cell sprites + per-cell attribute) →
  generated `src/assets_gfx.{h,c}`.

Realistic expectations: the `.scr` loading screen and the title art are a direct win for AI art; in-game glyphs
use AI output as **reference**, downsampled by the converter and hand-tuned at the pixel level.

## Architecture

Recommended framework patterns, all of which fit this game:

- **Single 50 Hz frame loop** via `intrinsic_halt()`. Intruders advance on frame counters; turrets fire on
  cooldown counters. Entity counts are small (≤ `MAX_ENEMIES` 24, ≤ `MAX_TOWERS` 32) → comfortable for Z80.
- **All state in one global struct** named `G` (linker symbol `_G`, which the integration harness reads):
  `struct GameState G;`. Fixed arrays, no heap, no `float`. Gold/HP/timers are `uint8_t`/`uint16_t`; distances
  use integer Chebyshev metric.
- **Character-cell graphics**, dirty-cell redraw (only changed cells repaint).
- **`const` lookup tables** (turret/intruder stats, wave tables, the path, UDG byte arrays) land in the code
  segment via `zpragma.inc`.
- **128K**: AY-3-8912 for sound (SFX only in the MVP). No bank switching needed — one map's data fits the
  standard region (code org `0x6000`, stack `0xF000`).
- **Strict module layering, no cycles**: `main → (input, game, render, sound)`. `render` knows only how to
  draw glyphs/sprites at cells, never game semantics.

### Modules (`src/`)

| File | Responsibility |
| ---- | -------------- |
| `main.c` | 50 Hz loop, init, module orchestration |
| `game.{c,h}` | state machine (BUILD/WAVE/WIN/MELTDOWN), enemy/turret arrays, wave spawner, economy, targeting, combat, reactor stability — owns global `G` |
| `render.{c,h}` | draw map / HUD / cursor / turrets / intruders; dirty-cell redraw; uses `assets_gfx` |
| `input.{c,h}` | keyboard (cursor, cycle turret, build, release wave) |
| `sound.{c,h}` | AY SFX (fire / hit / wave-start / core-hit / win / meltdown) |
| `level.{c,h}` | const map (path/buildable/entry/core), pre-computed path, wave tables, turret/intruder stat tables |
| `assets_gfx.{c,h}` | **generated** by `png2zx.py`: UDG / sprite byte arrays |

## Build / test

```bash
# host-side asset tooling (outside Docker)
pip install -r requirements.txt          # google-genai, Pillow
python tools/gen_assets.py               # GEMINI_API_KEY from .env → assets/png/*.png
python tools/png2zx.py                    # → assets/scr/loading.scr (6912) + src/assets_gfx.{c,h}

# C build + tests (Docker)
docker compose run --rm build            # → build/app.tap (+ app.map for integration)
docker compose run --rm smoke            # PC≠0x0000, smoke.scr=6912
docker compose run --rm integration      # JSON scenarios in tools/integration/scenarios/ over ZRCP
```

Integration scenarios assert on emulator memory via `_G+offset` symbols from `build/app.map`
(ops: `frames`, `key`, `assert_eq`, `assert_nonzero`). New `.c` files must be added to `SRCS` in
`Makefile.inner`.

## MVP definition of done

On 128K: one map of ~6–8 waves is playable; 3 turret types × 3 intruder types; correct economy / stability /
win-meltdown; AI loading screen + basic turret/intruder graphics; AY SFX; stable (smoke + all integration
scenarios PASS).

## Out of scope (future)

- Multiple maps; procedural route generation; maze-TD with path-blocking towers (needs A\*).
- Animated projectile sprites; flying enemies, healers, bosses.
- Turret upgrades beyond level 1; selling with refund; bomb ability.
- Kempston joystick; save/progress; high scores; AY music (SFX only in MVP).
- Switching art to `gemini-3-pro-image-preview` (Nano Banana Pro) as a post-MVP quality upgrade.

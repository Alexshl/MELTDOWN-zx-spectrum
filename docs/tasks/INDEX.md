# Tasks — Project Roadmap

Implementation tasks for **MELTDOWN** (sci-fi tower defense, ZX 128K) built on `zx-framework`.
Vision: [`../PROJECT.md`](../PROJECT.md). Each task is driven through the 5-agent pipeline (`/task <id>`).

## Progress

| #   | Task | Status | Output files |
| --- | ---- | ------ | ------------ |
| 01  | [Scaffold — 128K, 50 Hz loop, modules](01-scaffold-128k-frame-loop.md) | DONE | `src/{main,game,render,input,sound,level}.*`, `Makefile.inner` |
| 02  | [Asset tooling — Gemini gen + PNG→ZX converter](02-asset-tooling.md) | DONE | `tools/gen_assets.py`, `tools/png2zx.py`, `requirements.txt`, `.gitignore` |
| 03  | [Loading screen `.scr` in the `.tap`](03-loading-screen.md) | DONE | `assets/scr/loading.scr`, build glue |
| 04  | [Map model + cell renderer + UDG tiles](04-map-model-render.md) | DONE | `src/level.*`, `src/render.*`, `src/assets_gfx.*` |
| 05  | [Cursor, turret placement, economy](05-cursor-placement-economy.md) | DONE | `src/input.*`, `src/game.*`, scenario `build-place-turret` |
| 06  | [Intruder spawning + path movement](06-enemy-spawn-path.md) | DONE | `src/game.*`, `src/level.*`, scenario `enemy-path-march` |
| 07  | [Combat — targeting + damage](07-combat-targeting.md) | DONE | `src/game.*`, scenario `combat-kill` |
| 08  | [Game flow — waves, win/MELTDOWN, HUD, title](08-game-flow-waves.md) | DONE | `src/game.*`, `src/level.*`, scenarios `flow-meltdown`, `flow-win` |
| 09  | [AY sound + content + balance + polish](09-sound-content-polish.md) | DONE | `src/sound.*`, `src/level.*`, scenario `sfx-events` |
| 10  | [AY background music — underground/dungeon theme](10-ay-music.md) | DONE | `src/sound.*`, `src/game.*`, scenario `music-playing` |
| 11  | [Game-over screens + restart (fix end-state freeze)](11-game-over-restart.md) | DONE | `src/main.c`, `src/game.*`, `src/render.*`, scenario `restart-meltdown` |
| 12  | [Audio fix — transient SFX + ominous music](12-audio-transient-sfx-ominous-music.md) | DONE | `src/sound.*`, `src/game.*`, scenario `sfx-transient` |
| 13  | [HUD — selected turret name + cost](13-hud-turret-name-cost.md) | DONE | `src/render.*`, scenario regression |
| 14  | [Title screen — control setup (Keyboard/Kempston + redefine keys)](14-title-input-config.md) | DONE | `src/input.*`, `src/game.*`, `src/render.*`, scenario `input-config` |
| 15  | [End-screen text (YOU WIN / MELTDOWN + PRESS ENTER)](15-end-screen-text.md) | DONE | `src/render.*`, scenario `flow-win` (restart check) |
| 16  | [Multiple levels — 5-level linear campaign](16-multi-level-campaign.md) | IN PROGRESS | `src/level.*`, `src/game.*`, `src/render.*`, scenario `campaign-advance` |

## Execution order

`01 → 02 → 03 → 04 → 05 → 06 → 07 → 08 → 09 → 10 → 11 → 12 → 13 → 14 → 15 → 16`

Parallelizable where convenient: **02** (host tooling) is independent of **01** (game scaffold); **05** and
**06** both depend only on **04** and can be done in either order before **07**.

## Status legend

- **TODO** — not started
- **IN PROGRESS** — currently being worked on
- **BLOCKED** — waiting for clarification or an external dependency
- **DONE** — implemented and verified

## How to work with this index

1. Run `/task <id>` (or `/task` to pick up the first TODO) — the 5-agent pipeline does planner → coder →
   tester → reviewer → documenter.
2. Verify with `docker compose run --rm build` / `smoke` / `integration`.
3. The documenter sets the task to **DONE** here and in the task header on success.

## Out of scope (for future tasks)

- Multiple maps; procedural routes; maze-TD with path-blocking towers (needs A\*).
- Animated projectile sprites; flying / healer / boss enemies; turret upgrades; selling/refund; bomb ability.
- Kempston joystick; save/progress; high scores; AY music.
- Nano Banana Pro (`gemini-3-pro-image-preview`) art upgrade.

# 01. Scaffold — 128K target, 50 Hz frame loop, module skeleton

**Status**: DONE
**Depends on**: —
**Blocks**: 03, 04

## Goal

Replace the hello-world with a ZX 128K app that runs a stable 50 Hz frame loop over a clean module skeleton,
builds to `build/app.tap`, and passes smoke.

## Context

The framework ships a hello-world `src/main.c`. This task lays the architecture skeleton everything else builds
on (see `docs/PROJECT.md` → Architecture): modules `main → (input, game, render, sound)` plus `level` and the
generated `assets_gfx`, the single global `struct GameState G;` (linker symbol `_G`, read by the integration
harness), and a 50 Hz loop synchronized by `intrinsic_halt()`. Target is 128K (smoke/integration already run
ZEsarUX with `--machine 128k`). No gameplay yet — only the scaffold.

## Acceptance criteria

- [ ] `src/` contains stub modules with headers: `main.c`, `game.{c,h}`, `render.{c,h}`, `input.{c,h}`,
      `sound.{c,h}`, `level.{c,h}`; layering `main → (input, game, render, sound)` with no include cycles.
- [ ] A single global `struct GameState G;` is defined and referenced so `_G` appears in `build/app.map`.
- [ ] `main()` runs an infinite loop with exactly one `intrinsic_halt()` per iteration (frame-locked at 50 Hz,
      no busy spin).
- [ ] `Makefile.inner` `SRCS` lists every compiled `.c` file; `docker compose run --rm build` produces
      `build/app.tap` and `build/app.map`.
- [ ] `docker compose run --rm smoke` passes.

## Test plan

```
smoke-only: build succeeds, smoke.scr=6912 bytes, PC≠0x0000, and symbol _G is present in build/app.map
```

## Out of scope

- Any rendering, gameplay, or sound content (later tasks).
- `assets_gfx.{c,h}` content — generated in task 02/03; a placeholder stub include is fine here.

## Completion note

Implemented 2026-06-23. Clean 128K scaffold with stable 50 Hz frame loop, correct module layering, and linker symbol `_G` verified in build map; smoke test passed.

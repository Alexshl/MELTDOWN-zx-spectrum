# 03. Loading screen `.scr` in the `.tap`

**Status**: DONE
**Depends on**: 01, 02
**Blocks**: 04

## Goal

A generated sci-fi "MELTDOWN" loading screen is embedded in the `.tap` and shown at startup.

## Context

Uses the task-02 tooling: `gen_assets.py` produces a 256×192 splash, `png2zx.py` converts it to
`assets/scr/loading.scr` (6912 bytes). The build must embed and display it on load. The display mechanism
(e.g. `appmake +zx --screen`, or copying the screen to `0x4000` at startup before the loop) must be confirmed
against the z88dk docs by the planner — do not invent the flag.

## Acceptance criteria

- [ ] `assets/scr/loading.scr` exists and is exactly 6912 bytes (generated, not committed by hand).
- [ ] The build embeds the loading screen and displays it at startup (visible before/at the first frame).
- [ ] `docker compose run --rm build` still succeeds and `smoke` still passes.

## Test plan

```
smoke-only: build succeeds; assets/scr/loading.scr=6912 bytes; smoke.scr=6912; PC≠0x0000
```

## Out of scope

- Title/menu interactivity (task 08).
- In-game sprites/tiles (task 04).

## Completion note

Implemented 2026-06-23. png2zx.py extended to emit const uint8_t loading_scr[6912] in src/loading_scr.{c,h}; render_loading_screen() memcpy's to 0x4000 at startup; verified with 0 differing bytes between artifacts/smoke.scr and assets/scr/loading.scr on actual Spectrum screen.

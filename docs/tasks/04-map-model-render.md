# 04. Map model + cell renderer + UDG tiles

**Status**: DONE
**Depends on**: 01, 02
**Blocks**: 05, 06

## Goal

The static map (corridors, buildable cells, entry, core) and the HUD frame render correctly, with a movable
highlighted cursor cell.

## Context

`level.c` defines the const map and the pre-computed enemy path; `render.c` draws map + HUD + cursor using UDG
tiles from `assets_gfx` (placeholder UDGs are acceptable until art is final). Dirty-cell redraw per
`docs/PROJECT.md` → Architecture. No entities yet. Input that moves the cursor is wired in task 05; here render
just exposes the draw/move-cursor functions.

## Acceptance criteria

- [ ] `level.{c,h}` defines the const map grid (≈30×18) with cell types (path / buildable / wall / entry /
      core) and a const pre-computed entry→core path list; both in the code segment.
- [ ] `render.c` draws the full map + HUD frame on startup; cell glyphs come from `assets_gfx` UDGs
      (placeholders allowed).
- [ ] A highlighted cursor cell is drawn; `render` exposes a function to redraw the cursor at a given cell.
- [ ] Build + smoke pass; rendering is stable across frames (PC≠0x0000 after ~200 frames).

## Test plan

```
smoke-only: build succeeds; smoke.scr=6912; PC≠0x0000 after the smoke frame budget (render does not crash)
```

## Out of scope

- Cursor input handling and turret placement (task 05).
- Intruders, turrets, combat (tasks 06–07).

## Completion note

Implemented 2026-06-23. level.c defines const level_map[21][32] with 1 ENTRY, 1 CORE, and a contiguous path; render.c draws map + HUD + cursor from 11 UDG tiles. Fixed coordinate-transpose bug: z88dk's zx_cxy2saddr/aaddr are column-first (x,y), not (row,col) as initially planned—all call sites corrected. Visual confirmation: wall border, cyan corridor, green pads, magenta entry, bright yellow core, blue HUD, white cursor all render correctly; clean rebuild 12348 bytes; smoke PASS.

# 16. Multiple levels — 5-level linear campaign

**Status**: TODO
**Depends on**: 15
**Blocks**: —

## Goal

Replace the single hardcoded map with a **5-level linear campaign**. Clearing a level's final wave
**auto-advances** to the next (harder) level; clearing the **last** level is WIN; MELTDOWN (stability 0) ends
the run at any level. Each level has its own map, ENTRY→CORE path, and wave table, with difficulty rising
across the campaign.

## Context

`src/level.c` currently exposes single globals consumed across the codebase:
- `level_cell(col,row)` reads the one `level_map[MAP_ROWS][MAP_COLS]` — used by `render.c` (`render_draw_map`,
  `draw_cell_or_tower`, cursor).
- `level_path[]` / `level_path_len` — used by `game.c` (spawn at `level_path[0]`, path movement, combat range,
  leak at `path_idx >= level_path_len-1`).
- `level_waves[WAVE_COUNT]` — used by `game.c` (`game_start_wave`, spawner) and `WAVE_COUNT` gates the
  win/advance check.

Refactor to support `NUM_LEVELS` levels by storing per-level data and **repointing** the active level so
existing consumers keep working with minimal change:
- Per-level `const` data: `level_maps[NUM_LEVELS][MAP_ROWS][MAP_COLS]`, `level_paths[NUM_LEVELS][...]` +
  `level_path_lens[NUM_LEVELS]`, `level_wave_tables[NUM_LEVELS][WAVE_COUNT]`.
- A `level_load(uint8_t n)` that sets the active pointers: `level_cell` indexes `level_maps[n]`; expose
  `level_path` as a `const struct PathPoint *` (repointed to `level_paths[n]`), `level_path_len` as a `uint8_t`
  set to `level_path_lens[n]`, and `level_waves` as a `const struct WaveDef *` (repointed to
  `level_wave_tables[n]`). All existing `level_path[idx]` / `level_path_len` / `level_waves[idx]` reads keep
  compiling and working unchanged.
- Add `uint8_t current_level` to `G` (new struct TAIL at `_G+245`, after `control_mode@244`), observable over
  ZRCP. `game_init` sets `current_level = 0` and calls `level_load(0)`.

`game.c` progression: the existing wave-clear branch (`wave_index >= WAVE_COUNT-1 && enemies_count==0`) becomes:
if `current_level < NUM_LEVELS-1` → **advance** (`current_level++`, `level_load(current_level)`, clear towers +
enemies, `wave_index=0`, `phase=BUILD`, reset stability/gold to level defaults, request a map redraw); else →
`state = STATE_WIN`. MELTDOWN (stability 0) still ends the run at any level. `main.c`/`render.c` must redraw the
**new** map (and HUD) on a level change, mirroring the TITLE→PLAY map-draw edge.

## CRITICAL regression constraint — level 0 == today's content

**Level 0 must be byte-for-byte the current `level_map` + `level_path` + `level_waves`** (the 15-cell path,
the 6-wave 21-enemy table). Every existing scenario operates on level 0; keeping it identical means
`enemy-path-march`, `combat-kill`, `flow-meltdown`, `build-place-turret`, `sfx-*`, `music-playing`,
`input-config`, `restart-meltdown` keep passing unchanged. Levels 1–4 are **additions**. The only behavioural
change for level 0 is that clearing its 6th wave now **advances to level 1 instead of WIN** — so the `flow-win`
scenario must be updated (see Test plan).

## Acceptance criteria

- [ ] `NUM_LEVELS == 5`. Each level has a distinct `const` map, a **valid** contiguous ENTRY→CORE path whose
      cells are FLOOR in that map, and its own wave table; difficulty increases across levels (more/tougher
      enemies, and/or longer paths).
- [ ] Level 0 is identical to the pre-task map/path/wave table (regression guarantee).
- [ ] Clearing the final wave of a non-last level advances to the next: `current_level++`, towers + enemies
      cleared, `wave_index→0`, `phase→BUILD`, the **new** map drawn, gameplay continues (`state` stays PLAY),
      stability/gold reset to level defaults.
- [ ] Clearing the final wave of the **last** level (`current_level == NUM_LEVELS-1`) sets `state == STATE_WIN`.
- [ ] MELTDOWN (stability 0) ends the run at any level (unchanged).
- [ ] `G.current_level` (uint8_t) is appended after `control_mode@244` (at `_G+245`), observable over ZRCP; no
      existing `_G` offset shifts. `game_init` sets it to 0 and loads level 0.
- [ ] The active level renders correctly (the current level's map is drawn on entry/advance; path movement and
      combat use the current level's path).
- [ ] Build + smoke + all integration scenarios PASS (existing scenarios updated only where multi-level changes
      semantics — `flow-win`; everything else unchanged and green).

## Test plan

```
scenarios: [campaign-advance, flow-meltdown, build-place-turret, enemy-path-march, combat-kill, restart-meltdown, input-config, music-playing]
```

`campaign-advance` (NEW): on level 0, build an adequate defence (scripted keys, like `flow-win` does) and clear
all 6 waves of level 0; assert `_G+245` (current_level) == 1, `_G+239` (state) == 1 (still PLAY — advanced, not
WIN), and `_G+240` (wave_index) == 0 (reset for the new level). This proves the campaign auto-advance.

`flow-win` is **updated/retired**: clearing level 0's 6 waves now advances to level 1 (not WIN), so its old
`state==WIN` assertion is invalid. Replace `flow-win` with `campaign-advance` (or repoint `flow-win` to assert
the level-0→level-1 advance and drop the task-15 WIN→restart block, which no longer triggers at level 0). The
**full-campaign WIN** (clearing level 4) is impractical to drive in one ZRCP scenario (5 levels × 6 waves), so it
is verified by **code inspection**: the single branch `current_level >= NUM_LEVELS-1 ? STATE_WIN : advance`. The
`restart-meltdown` scenario still exercises MELTDOWN + the end-state restart on level 0.

All other scenarios run on level 0 (unchanged content) and must stay green — this is the regression guarantee.

## Out of scope

- Per-level art generation (reuse the existing UDG tiles; new maps are new cell layouts, not new tilesets).
- Procedural map generation; maze-TD / path-blocking towers (needs A*).
- A level-select menu (the campaign is linear; selection was the rejected option).
- Carrying towers across levels (towers are cleared on advance — the map changes); per-level scoreboard.
- Saving campaign progress to tape.

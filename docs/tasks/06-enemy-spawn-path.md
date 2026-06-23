# 06. Intruder spawning + path movement

**Status**: TODO
**Depends on**: 04
**Blocks**: 07

## Goal

Releasing a wave spawns intruders that crawl the fixed path; an intruder reaching the core lowers reactor
stability.

## Context

`game.c` wave spawner: ENTER transitions BUILD→WAVE and spawns the wave's intruders at the entry cell at
intervals. Each intruder advances one path cell every K frames (K per type) along the pre-computed path from
`level.c`. `enemies[]` array (`MAX_ENEMIES`). On reaching the core an intruder is removed and stability
decrements. `render` draws/erases enemy glyphs (dirty-cell). Intruder stats are a const table in `level.c`.

## Acceptance criteria

- [ ] ENTER transitions BUILD→WAVE and begins spawning the wave's intruders at the entry cell.
- [ ] Intruders advance along the pre-computed path one cell per K frames; multiple intruders are tracked
      independently.
- [ ] An intruder reaching the core is removed and decrements reactor stability; its glyph is erased from the
      previous cell.
- [ ] Intruder stat table (HP / speed K / bounty per type) is `const` in `level.c`.

## Test plan

```
scenarios: [enemy-path-march]
```

`enemy-path-march`: press ENTER; advance frames and assert the spawned intruder's path index has advanced
(assert_eq on `enemies[0]` path index after M frames, or assert_nonzero on `enemies_count`); continue until an
intruder reaches the core and assert reactor stability has decremented (assert_eq on the stability field).

## Out of scope

- Turrets firing at intruders (task 07).

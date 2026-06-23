# 05. Cursor, turret placement, economy

**Status**: DONE
**Depends on**: 04
**Blocks**: 07

## Goal

Move the cursor with QAOP, cycle the turret type with SPACE, build with M (spending gold), and reject invalid
placements.

## Context

`input.c` reads QAOP/SPACE/M (letters are sent literally by the integration harness; SPACE is mapped).
`game.c` holds cursor position, selected turret type, gold, and the `towers[]` array (`MAX_TOWERS`). Placing a
turret on a buildable empty cell deducts its cost; non-buildable / occupied / insufficient-gold is rejected.
`render` shows the turret glyph and the selected type in the HUD. Turret stats are a const table in `level.c`.
Economy model per `docs/PROJECT.md` (bounty + start + wave bonus); this task covers starting gold and spend.

## Acceptance criteria

- [ ] QAOP moves the cursor within map bounds; SPACE cycles selected turret `LASER → MISSILE → TESLA`; the HUD
      shows the current selection.
- [ ] M places the selected turret on a buildable empty cell: `towers_count` +1, `gold` -= type cost, the cell
      shows the turret glyph.
- [ ] Invalid placement (non-buildable / occupied / `gold` < cost) is rejected: no turret added, `gold`
      unchanged.
- [ ] Turret stat table (cost / range / damage / cooldown per type) is `const` in `level.c`.

## Test plan

```
scenarios: [build-place-turret]
```

`build-place-turret` (asserts via `_G+offset`): record starting gold; press SPACE to select a type; QAOP to a
known buildable cell; M → assert `towers_count == 1` and `gold == start - cost`; press M again on the now-
occupied cell → assert `towers_count == 1` (rejected, unchanged).

## Out of scope

- Combat / firing (task 07); selling / upgrading.

## Completion note

Implemented 2026-06-23. QAOP + SPACE + M implemented with edge-detected hardware-correct scancodes in input.c; game.c enforces placement rules (buildable, not-occupied, gold sufficient); turret stats const in level.c; all 18 test asserts pass. A test-only input mailbox `g_test_cmd` (non-static global, poked by tools/integration/run.py via ZRCP write-memory to bypass ZEsarUX keyboard port limitations) was added and ships in the binary — candidate for future `#ifdef ZX_TEST_HOOKS` cleanup in tasks 06/08.

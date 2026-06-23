# 09. AY sound + content + balance + polish

**Status**: DONE
**Depends on**: 08
**Blocks**: —

## Goal

AY-3-8912 SFX wired to game events, final 3×3 content, and a balanced ~6–8 wave level; everything stable.

## Context

`sound.c` uses z88dk's AY/PSG support — the exact API must be confirmed against the z88dk Sinclair wiki by the
planner (do not invent it). SFX for fire / hit / wave-start / core-hit / win / meltdown. To make sound testable
over ZRCP, `game.c` writes a `last_sfx` event id into `G` whenever it triggers a sound. Finalize the turret /
intruder stat tables and the wave table into a fair, beatable-but-tense level. Apply final art passes from
`gen_assets.py` / `png2zx.py` for turrets / intruders / HUD where feasible.

## Acceptance criteria

- [ ] AY SFX play (on `--machine 128k`) for: turret fire, intruder hit/kill, wave start, core hit, WIN,
      MELTDOWN; each event sets `G.last_sfx` to a distinct id.
- [ ] All 3 turret types and 3 intruder types are present, mechanically distinct, and used across the wave
      table.
- [ ] The level (~6–8 waves) is completable with good play and loseable with poor play (balance sanity).
- [ ] Build + smoke + all integration scenarios PASS.

## Test plan

```
scenarios: [sfx-events, build-place-turret, enemy-path-march, combat-kill, flow-meltdown, flow-win]
```

`sfx-events`: trigger a wave start and a turret fire; assert `G.last_sfx` becomes the expected non-zero ids
(assert_eq). The remaining scenarios are a regression pass over tasks 05–08.

## Out of scope

- AY music (SFX only in MVP); flying/healer/boss enemies; turret upgrades; multiple maps.

## Completion note

Implemented 2026-06-23. AY-3-8912 SFX wired to six game events via port writes (0xFFFD select / 0xBFFD data) with `G.last_sfx` event tracking for ZRCP verification. Content: 3 turret types (LASER/MISSILE/TESLA) and 3 enemy types (DRONE/RUNNER/BRUTE) deployed across 6-wave table; level is completable with good play and loseable with poor play. Build, smoke, and all six integration scenarios PASS.

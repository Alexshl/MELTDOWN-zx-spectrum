# 07. Combat — targeting + damage

**Status**: TODO
**Depends on**: 05, 06
**Blocks**: 08

## Goal

Turrets auto-target intruders in range, deal damage; kills award bounty and remove the intruder.

## Context

`game.c` combat: when a turret's cooldown is ready it scans `enemies[]` for the nearest / first-in-path target
within range (integer Chebyshev distance), applies damage, then resets cooldown. HP≤0 → intruder removed and
`gold` += its bounty. `MISSILE` applies splash to adjacent cells; `TESLA` applies a slow (raises the target's
K). Hits are shown as a brief flash by `render` (no projectile sprites). Scanning is throttled to cooldown-ready
turrets.

## Acceptance criteria

- [ ] A turret with an in-range target fires on cooldown and reduces that target's HP; out-of-range intruders
      are untouched.
- [ ] An intruder whose HP reaches 0 is removed and `gold` += its bounty; `enemies_count` decreases.
- [ ] `MISSILE` splash damages adjacent cells; `TESLA` slows its target — both observable in `G` state.
- [ ] Targeting is throttled: a turret on cooldown does not perform a full enemy scan that frame.

## Test plan

```
scenarios: [combat-kill]
```

`combat-kill`: build a turret adjacent to the path (QAOP + SPACE + M); press ENTER; advance frames; assert
`enemies_count` drops below the spawned count AND `gold` increased by at least one bounty (assert_eq /
assert_nonzero on the relevant `_G` fields).

## Out of scope

- Full balance pass (task 09); win/lose flow (task 08).

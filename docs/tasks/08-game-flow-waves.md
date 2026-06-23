# 08. Game flow — waves, win/MELTDOWN, HUD, title

**Status**: TODO
**Depends on**: 07
**Blocks**: 09

## Goal

BUILD↔WAVE progression across all waves, WIN on clearing the last wave, MELTDOWN on stability 0, with a complete
HUD and a title screen.

## Context

`game.c` state machine ties everything together: BUILD (place) → WAVE (combat) → wave cleared (all intruders
dead or leaked) → next BUILD → … → last wave cleared = WIN; stability reaching 0 at any point = MELTDOWN. The
flat wave-clear bonus is applied to `gold` on each clear. The HUD shows gold / stability / wave # / selected
turret. A title screen (generated art) precedes the first BUILD. The wave table (count / composition / spacing
per wave) is `const` in `level.c`.

## Acceptance criteria

- [ ] Clearing all intruders of a wave returns to BUILD, applies the flat wave-clear bonus to `gold`, and
      increments the wave number.
- [ ] Clearing the final wave sets `state == WIN`; stability reaching 0 sets `state == MELTDOWN`; each shows its
      end screen.
- [ ] The HUD continuously reflects gold, stability, wave #, and selected turret.
- [ ] A title screen (generated art) is shown before gameplay; a key starts the first BUILD phase.
- [ ] The wave table (≥6 waves) is `const` in `level.c`.

## Test plan

```
scenarios: [flow-meltdown, flow-win]
```

`flow-meltdown`: start; release waves without building (or with too little defence); advance frames →
assert `state == MELTDOWN` and stability == 0.
`flow-win`: build an adequate defence (scripted key sequence) and clear every wave → assert `state == WIN`.

## Out of scope

- Sound (task 09); final balance tuning (task 09).

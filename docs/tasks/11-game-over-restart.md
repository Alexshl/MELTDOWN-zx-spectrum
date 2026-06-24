# 11. Game-over screens + restart (fix end-state freeze)

**Status**: DONE
**Depends on**: 10
**Blocks**: —

## Goal

Fix the perceived "freeze": reaching WIN or MELTDOWN currently washes the screen and then idle-halts forever
with no message and no way to continue. Give each end state a **clear, legible** screen and a **restart** on a
key press, so the game is playable end-to-end and never dead-ends.

## Context

`main.c` WIN/MELTDOWN branches call `render_win_screen()` / `render_meltdown_screen()` once on the
`PLAY → end` edge, then loop calling `intrinsic_halt()` with no input handling. The end-screen renderers only
wash the **attribute** file (leaving the previous gameplay **pixels** visible), so the player sees a coloured
screen full of leftover map/HUD pixels and no "YOU WIN" / "MELTDOWN" text — indistinguishable from a hang.

This task: (1) the end-screen renderers must clear the playfield pixels and present a clear, distinct end state
with a visible restart prompt; (2) the main loop must keep polling input in WIN/MELTDOWN and, on the restart
key (ENTER), call a full game reset (`game_init` semantics) and return to `STATE_TITLE` so a new game can begin.

## Acceptance criteria

- [ ] WIN and MELTDOWN each present a **clear, distinct** end screen: playfield pixels are cleared (no leftover
      gameplay glyphs) and a visible restart prompt is shown (a colour/banner scheme is acceptable; text is a
      plus). The two states are visually distinguishable from each other.
- [ ] Pressing **ENTER** on a WIN or MELTDOWN screen performs a full reset (gold→100, stability→20,
      `wave_index`→0, `towers_count`→0, all enemies cleared, `phase`→BUILD) and returns to `STATE_TITLE`.
- [ ] The main loop never becomes input-dead in WIN/MELTDOWN — `input_poll` runs and the restart key is acted on.
- [ ] Existing offsets are unchanged (no new `G` field is required; if any is added it is appended after the
      current struct tail). Tasks 05–10 scenarios still PASS.
- [ ] Build + smoke + all integration scenarios PASS.

## Test plan

```
scenarios: [restart-meltdown, sfx-events, build-place-turret, enemy-path-march, combat-kill, flow-meltdown, flow-win]
```

`restart-meltdown`: dismiss title, release waves with no turrets until `state == MELTDOWN` (mirror
`flow-meltdown`'s sequence/budgets); assert `_G+239 == 3` (MELTDOWN) and `_G+137 == 0` (stability). Then press
ENTER; advance a few frames; assert `_G+239 == 0` (back to TITLE) and the reset values: `_G+5 == 100` (gold,
low byte; gold is `uint16` — assert the low byte == 100 and high byte 0), `_G+137 == 20` (stability),
`_G+240 == 0` (wave_index), `_G+8 == 0` (towers_count). The remaining scenarios are a regression pass.

## Out of scope

- Scoreboard / high scores; multiple lives; difficulty select (those are separate features).
- Animated end screens or end-screen music (keep the task-09 one-shot WIN/MELTDOWN SFX cue).

## Completion note

Implemented 2026-06-23. End-state freeze resolved: game_restart() wraps full state reset, input.c now handles ENTER in WIN/MELTDOWN states to restart, render functions clear pixel bitmap and present distinct colour schemes (green for WIN, red for MELTDOWN) with blue prompt band. All acceptance criteria verified; restart-meltdown scenario confirms full reset (gold 100, stability 20, wave_index 0, towers_count 0).

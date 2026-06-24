# 15. End-screen text (YOU WIN / MELTDOWN + PRESS ENTER)

**Status**: DONE
**Depends on**: 11, 14
**Blocks**: —

## Goal

Make the WIN and MELTDOWN screens **say what happened and how to continue**. Right now they clear the screen
and wash it green (WIN) / red (MELTDOWN) with a blue band but render **no text** — so the player sees a static
coloured screen with no message and reads it as a freeze. Add legible end text + a restart prompt, and verify
the **WIN** restart path (ENTER → title) works, not just MELTDOWN.

## Context

`render_win_screen` / `render_meltdown_screen` (`src/render.c`, from task 11) currently: `memset` the pixel
bitmap, wash all attributes (WIN green / MELTDOWN red), and paint a blue band on rows 22–23 — but draw no
glyphs. Task 14 added a reusable uppercase text renderer `render_text(col,row,const char *s,attr)` backed by
`font_az[26][8]` + `digit_font` (A–Z, 0–9, space). Use it to label the screens.

The restart wiring already exists and is correct: `input_poll` polls the hardware ENTER edge in **all** states
(`src/input.c:264-269`) and `process_start_wave` restarts when `state == STATE_WIN || STATE_MELTDOWN`
(`src/input.c:109-110`, calling `game_restart` → `game_init`). The task-11 `restart-meltdown` scenario verifies
the MELTDOWN restart via the test mailbox; this task additionally verifies the **WIN** restart (same code path,
but exercised explicitly so we don't infer it).

## Acceptance criteria

- [ ] The WIN screen renders legible text identifying the win (e.g. `YOU WIN`) **and** a restart prompt
      (e.g. `PRESS ENTER`), drawn via `render_text` / `font_az`, positioned so it is clearly readable on the
      green wash (e.g. message centred mid-screen, prompt on the blue band row).
- [ ] The MELTDOWN screen renders legible text identifying the loss (e.g. `MELTDOWN`) and the same restart
      prompt, on the red wash. WIN and MELTDOWN remain visually distinct (green vs red) and their messages differ.
- [ ] Text uses only the existing `render_text` charset (A–Z, 0–9, space) — no new font dependency, no `float`,
      no dynamic memory. The end-screen pixel-clear + colour scheme from task 11 is preserved.
- [ ] Pressing ENTER on the WIN screen restarts the game (full reset → `STATE_TITLE`) — verified, not just for
      MELTDOWN.
- [ ] Build + smoke + all integration scenarios PASS (no `_G` offset / struct change — rendering + a scenario
      assertion only).

## Test plan

```
scenarios: [flow-win, restart-meltdown, build-place-turret, combat-kill, input-config, music-playing]
```

`flow-win` is **extended**: after it reaches WIN (`_G+239 == 2`, `_G+240 == 5`, stability nonzero), append a
restart check — press ENTER, advance a few frames, and assert `_G+239 == 0` (back to TITLE) plus the reset
values (`_G+5 == 100` gold low byte, `_G+6 == 0`, `_G+137 == 20` stability, `_G+240 == 0` wave_index,
`_G+8 == 0` towers_count). This proves the WIN→ENTER→restart path (via the mailbox) end-to-end. `restart-meltdown`
remains the MELTDOWN restart check. The remaining scenarios are a regression pass. The end-screen **text pixels**
are not asserted over ZRCP (visual) — verified by reviewer code inspection that `render_text` draws the WIN/
MELTDOWN messages + prompt on the correct rows with the correct charset.

## Out of scope

- Score / time / wave-count summary on the end screen (a later polish).
- Animated or multi-line end screens; localised text.
- Persisting anything across the restart (session reset is fine, per task 11).

## Completion note

Implemented 2026-06-24. WIN/MELTDOWN screens now render "YOU WIN" and "MELTDOWN" text respectively using the task-14 font_az charset, positioned on green and red washes with "PRESS ENTER" prompt on the blue band. WIN restart path verified end-to-end via flow-win scenario: ENTER returns to TITLE with full reset. All 10 integration scenarios pass.

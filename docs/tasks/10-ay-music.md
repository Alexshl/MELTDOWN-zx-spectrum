# 10. AY background music — looping underground/dungeon theme

**Status**: DONE
**Depends on**: 09
**Blocks**: —

## Goal

A frame-driven AY-3-8912 music player that loops an **original underground/dungeon-style chiptune**
(minor-key, walking bassline — evoking a classic platformer "underworld" vibe) during gameplay, coexisting
with the task-09 SFX. Music plays on AY channels **B/C**; SFX keep channel **A**, so an SFX never silences the
music and vice versa.

## Context

`sound.c` already drives the AY directly via `z80_outp(0xFFFD, reg)` / `z80_outp(0xBFFD, val)` and exposes
`sound_init` / `sound_tick` (called every frame from `main.c`). Task 09 uses **channel A** for SFX
(`last_sfx@241`, sticky). This task adds a non-blocking tune sequencer that advances one step per fixed number
of 50 Hz frames, programming **channel B (melody)** and optionally **channel C (bass)**, looping a `const`
note table. The melody is composed fresh (an original arrangement in the underground/dungeon style) — **not** a
verbatim transcription of any copyrighted composition.

Music runs only while `state == STATE_PLAY`. On entering `STATE_WIN` / `STATE_MELTDOWN` (and while
`STATE_TITLE`), the music channels are silenced. SFX continue to work during `STATE_PLAY`.

To make the player testable over ZRCP, `G` exposes a free-running **`music_cursor`** byte that the sequencer
bumps each time it steps to a new note — appended **after** `last_sfx@241` (i.e. at `_G+242`) so no existing
field offset shifts.

## Acceptance criteria

- [ ] A frame-driven AY tune sequencer plays a **looping** melody (≥16 notes) during `STATE_PLAY`, using AY
      channel(s) **other than channel A**, so SFX (channel A) and music coexist — neither silences the other.
- [ ] The tune is an **original** underground/dungeon-style chiptune stored as a `const` note table (in the code
      segment), looping seamlessly; it is **not** a note-for-note copy of a copyrighted theme.
- [ ] Music starts on the `TITLE → PLAY` transition and the music channels are **silenced** on `WIN` / `MELTDOWN`
      and while on the title screen. SFX still fire during `PLAY`.
- [ ] `G.music_cursor` (`uint8_t`, appended after `last_sfx`, at `_G+242`) advances as notes play, observable over
      ZRCP. No existing `_G` offset shifts.
- [ ] `sound_tick` stays **non-blocking** (only `z80_outp` port writes per frame — no `bit_beep`, no busy-wait,
      no `halt`) so the 50 Hz loop and integration frame budgets are unaffected.
- [ ] Build + smoke + all integration scenarios PASS.

## Test plan

```
scenarios: [music-playing, sfx-events, build-place-turret, enemy-path-march, combat-kill, flow-meltdown, flow-win]
```

`music-playing`: dismiss the title (ENTER), confirm `state == PLAY`; advance enough frames for the sequencer to
step several notes; assert `G.music_cursor` (`_G+242`) is **nonzero** (music advanced). Then build a turret and
start a wave and confirm a SFX still registers (`last_sfx` becomes non-zero) **while** `music_cursor` keeps
advancing — proving SFX and music coexist. The remaining scenarios are a regression pass over tasks 05–09
(they must stay green; the new `music_cursor` field must be the struct tail at `_G+242`).

## Out of scope

- Multi-pattern songs / song structure beyond a single looping phrase; per-channel envelopes; a tracker format.
- Faithful reproduction of any specific copyrighted melody (an original dungeon-style arrangement is used).
- Music for the title / win / meltdown screens (those stay silent or keep their task-09 one-shot SFX cue).

## Completion note

Implemented 2026-06-23. Frame-driven AY music sequencer plays a 20-note original A-natural-minor dungeon theme on channel B (looping, 50 Hz rate-controlled), coexisting with task-09 SFX on channel A via independent R7 mixer bits. G.music_cursor (uint8_t at _G+242) appended after last_sfx, advancing per note, observable over ZRCP. All acceptance criteria verified; build, smoke, and all 7 integration scenarios PASS.

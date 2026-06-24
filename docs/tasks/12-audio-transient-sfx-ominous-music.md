# 12. Audio fix — transient SFX + ominous music

**Status**: DONE
**Depends on**: 10
**Blocks**: —

## Goal

Fix the "sounds pile up / drone" problem and make the music genuinely tense. There must be **one** continuous
music track during gameplay with **short one-shot** sound effects on top — never a sustained SFX tone layering
over the music. Replace the current arpeggiated tune with an **original ominous / tension-building** theme.

## Context

In `sound.c`, `G.last_sfx` is **sticky** (set on an event, never cleared). `sound_tick` infers `sfx_active`
from `last_sfx != SFX_NONE`, so once any SFX fires, channel A holds that tone **indefinitely** — it only
changes when the next event id arrives. The result is a continuous drone clashing with the channel-B music:
exactly the "звук один на другой накладывается" the user reports.

Fix: make each SFX a brief one-shot. When `last_sfx` **changes** (edge), program channel A and start a short
countdown; while the countdown is > 0 channel A is audible; when it expires, silence channel A (volume 0 / mixer
tone-A bit set) regardless of `last_sfx`'s value. Keep `last_sfx` itself sticky so the task-09 `sfx-events`
asserts (`assert_eq` on the event id) keep passing — the audio gating is governed by the new countdown, not by
clearing `last_sfx`. Expose the countdown as an observable `G` field so the transient behaviour is testable.

Also retune the music: an **original** ominous, slow, low-register minor theme (a brooding/foreboding feel —
e.g. low pedal + sparse minor motif, slower tempo) replacing the current ascending/descending arpeggio. It must
remain an original composition (not a transcription of any copyrighted theme), loop during `STATE_PLAY`, and be
silenced outside `STATE_PLAY`. The shared-R7 composition and channel split (music on B/C, SFX on A) from task 10
must be preserved.

## Acceptance criteria

- [ ] Each SFX is a **short one-shot**: channel A is audible only for a brief fixed window after the event, then
      silenced — SFX never sustain or stack. Between SFX, channel A is silent while the music keeps playing.
- [ ] Exactly one music track plays during `STATE_PLAY` (channel B, optional bass on C); it does not compete
      with a droning SFX. SFX still fire correctly during play.
- [ ] The music is an **original** ominous/tension-building theme (darker, slower, low-register, minor),
      replacing the previous tune; it loops during PLAY and is silenced on TITLE/WIN/MELTDOWN.
- [ ] A new observable `G` field (e.g. `sfx_timer`, `uint8_t`, appended **after** `music_cursor@242` at `_G+243`)
      reflects the SFX one-shot countdown: nonzero just after an SFX event, returning to 0 after the window. No
      existing offset shifts.
- [ ] `sound_tick` stays non-blocking (only `z80_outp` writes; no busy-wait/halt).
- [ ] Build + smoke + all integration scenarios PASS (including the task-09 `sfx-events`, which keeps asserting
      `last_sfx` ids — `last_sfx` stays sticky).

## Test plan

```
scenarios: [sfx-transient, sfx-events, music-playing, combat-kill, flow-win]
```

`sfx-transient`: dismiss title, start a wave (no turret) to fire `SFX_WAVE_START`; within a couple of frames
assert `_G+243` (sfx_timer) is **nonzero** (SFX one-shot active); advance more frames than the SFX window;
assert `_G+243 == 0` (SFX auto-silenced) **while** `_G+242` (music_cursor) keeps advancing (assert_nonzero) —
proving SFX is transient and music continues. `sfx-events` / `music-playing` confirm no regression in the
sticky-`last_sfx` ids and music advance; `combat-kill` / `flow-win` are a gameplay regression pass.

## Out of scope

- AY envelopes (R11–R13), multi-pattern songs, a tracker format.
- Faithful reproduction of any specific copyrighted melody (an original ominous arrangement is used).
- Per-SFX distinct decay envelopes (a single fixed one-shot window for all SFX is sufficient).

## Completion note

Implemented 2026-06-23. Fixed overlapping/droning sound by introducing sfx_timer countdown (armed on last_sfx edge, decrements each frame, forces channel A volume to 0 on expiry); last_sfx remains sticky for task-09 compatibility. New ominous A-minor theme added; all nine integration scenarios pass.

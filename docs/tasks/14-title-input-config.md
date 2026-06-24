# 14. Title screen — control setup (Keyboard / Kempston + redefine keys)

**Status**: DONE
**Depends on**: 11
**Blocks**: —

## Goal

On the title screen, let the player choose a control method (**Keyboard** or **Kempston** joystick) and
**redefine all six game keys**. The chosen configuration takes effect in gameplay.

## Context

Today the title screen only waits for ENTER → BUILD. This task adds a small **menu** on the title: select
control method and a **Redefine Keys** flow. Per the agreed scope:

- **Redefine keys** covers all six actions: **Up, Down, Left, Right, Cycle-turret, Build**. ENTER stays fixed
  as confirm / start-wave.
- **Kempston** mode: the joystick moves the cursor and **fire = build**; turret cycle and wave start remain on
  the keyboard. Read Kempston via the verified z88dk API (the planner MUST confirm the exact function/header —
  e.g. `in_stick_kempston()` — or the documented port `0x1F` read, bits `000FUDLR`; do not invent it).

`input.c` currently hardcodes `IN_KEY_SCANCODE_q/a/o/p`, SPACE, M. This task makes those six bindings
**configurable** (stored scancodes), adds a control-method flag, and adds the title menu UI + a redefine flow
(prompt for each action, capture the next pressed key). Gameplay input then dispatches via the configured
bindings / joystick instead of the hardcoded constants.

CRITICAL: the integration test mailbox (`g_test_cmd`, written by `tools/integration/run.py` `send_key`) must
keep working so existing scenarios pass — the mailbox path injects the *logical* action keys (ENTER/M/SPACE/QAOP)
and must continue to drive the corresponding actions regardless of the new binding layer (e.g. the mailbox can
map to default bindings / logical actions, not raw rebindable scancodes). The planner must specify how the
mailbox path stays valid.

## Acceptance criteria

- [ ] The title screen presents a menu to choose **Keyboard** or **Kempston**, and a **Redefine Keys** option
      that lets the player set scancodes for all six actions (Up, Down, Left, Right, Cycle, Build). The menu is
      navigable and a selection/ENTER starts the game (first BUILD phase) as before.
- [ ] In **Keyboard** mode, the (possibly redefined) bindings drive cursor move / cycle / build in gameplay; the
      defaults match the current QAOP + SPACE + M when not redefined.
- [ ] In **Kempston** mode, the joystick moves the cursor and **fire builds**; cycle-turret and start-wave stay
      on the keyboard. Kempston is read via the planner-verified z88dk API / port `0x1F`.
- [ ] Control method and bindings persist for the session (stored in `G` and/or module statics) and the control
      method is observable over ZRCP (a `control_mode` field; any new `G` fields appended **after** the current
      struct tail so no existing offset shifts).
- [ ] The integration mailbox path still drives gameplay actions — all existing scenarios keep passing.
- [ ] Build + smoke + all integration scenarios PASS.

## Test plan

```
scenarios: [input-config, build-place-turret, enemy-path-march, combat-kill, flow-meltdown, flow-win, music-playing]
```

`input-config`: on the title, navigate the menu and select the **Kempston** option (via the mailbox action
keys); assert the `control_mode` field (`_G+<offset>`, the new tail field — planner to fix the exact offset)
reflects Kempston. Then confirm gameplay still functions through the mailbox: dismiss into BUILD, build a turret
(`_G+8 == 1`), start a wave (`_G+2 == 1`). The remaining scenarios are a full regression pass proving the new
binding/menu layer did not break the existing mailbox-driven input (they exercise QAOP/SPACE/M/ENTER).

Kempston joystick reads (port `0x1F`) cannot be injected via ZRCP (same limitation as the keyboard matrix), so
the **joystick-movement** path is verified by code inspection (the reviewer confirms the verified API is used
and wired to cursor/build); the scenario verifies the **menu/selection** and that mailbox gameplay still works.

## Out of scope

- Sinclair/Cursor joystick protocols; redefinable ENTER/confirm key; in-game (mid-play) remapping.
- Saving the config to tape/persistent storage (session-only is fine).
- On-screen joystick calibration.

## Completion note

Implemented 2026-06-23. Title menu (number keys 1=Keyboard, 2=Kempston, 3=Redefine) with ENTER preserved as start-game. Control mode stored at G.control_mode (CTRL_KEYBOARD 0 / CTRL_KEMPSTON 1), rebindable keys as input.c module statics binding[6] with defaults matching old hardcodes. Kempston reads via in_stick_kempston() verified API; mailbox path fully decoupled (maps ASCII→logical actions) so all existing scenarios pass unchanged. Redefine flow captures keys via in_inkey() + in_key_scancode(). New render functions: render_title_menu, render_title_redefine, render_text using A-Z font_az. Build + smoke + all 10 integration scenarios PASS; no offset shifts, no regression.

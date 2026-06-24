# 13. HUD — selected turret name + cost

**Status**: DONE
**Depends on**: 10
**Blocks**: —

## Goal

Show, in the HUD, **which turret is currently selected for placement and how much it costs**, updating as the
player cycles turrets (SPACE).

## Context

The HUD (`render_hud_stats` + `render_hud_selection` in `render.c`) currently shows gold / stability / wave and
a single selected-turret **glyph** at HUD col 1 — but no name and no price. The player can't tell which turret
is selected or whether they can afford it. `turret_stats[]` (`level.c`) holds the per-type cost; the three types
are LASER / MISSILE / TESLA (`G.sel_turret`).

Add a HUD readout of the selected turret's **name** (a short label that fits — e.g. `LASER` / `MISSL` / `TESLA`)
and its **cost** (from `turret_stats[sel_turret].cost`), laid out so it does not overlap the existing
gold/stability/wave fields. Render the small uppercase letters needed via a glyph table in `render.c` (the HUD
already uses a digit font + label glyphs — extend that approach; do not pull in a console/`printf` library).
Update the readout whenever the turret selection changes (the SPACE/cycle path already calls a HUD update).

## Acceptance criteria

- [ ] The HUD shows the currently selected turret's **name** and its **cost** (gold), both reflecting
      `G.sel_turret` and `turret_stats[sel_turret].cost`.
- [ ] Cycling the turret (SPACE) updates the displayed name and cost immediately.
- [ ] The new readout does not overlap or corrupt the existing gold / stability / wave HUD fields or the
      selection glyph; it fits within the HUD rows (21–23).
- [ ] Letter glyphs are rendered from a `const` table in `render.c` (no new library dependency); no `float`,
      no dynamic memory.
- [ ] Build + smoke + all integration scenarios PASS (no `_G` offset changes; HUD-only rendering change).

## Test plan

```
scenarios: [build-place-turret, combat-kill, flow-win]
```

This is a rendering change with no game-state/offset impact, so it is verified by **code inspection** (the
reviewer confirms the name + cost are drawn from `G.sel_turret` / `turret_stats[]` at non-overlapping HUD cells)
plus a **regression** pass: `build-place-turret` (exercises turret cycle + selection HUD), `combat-kill`, and
`flow-win` must stay green, and smoke must show PC ≠ 0 (the extra HUD draw must not hang or corrupt state).

## Out of scope

- Range/damage/cooldown stat display; affordability colour-coding (could be a later polish task — a simple
  name+cost is the MVP here).
- Scrollable/iconographic turret picker UI.

## Completion note

Implemented 2026-06-23. Added 9 const glyph bitmaps for letters (L,A,S,E,R,M,I,T) and $ symbol to render.c with a turret_name_glyphs pointer table; render_hud_turret_info() draws the selected turret name and cost on HUD row 22 (5 letters + separator + $ + 3-digit cost), called on TITLE→PLAY edge and during turret cycle (SPACE). All acceptance criteria verified; no struct/offset changes, rendering only. Build + smoke + all 9 integration scenarios PASS.

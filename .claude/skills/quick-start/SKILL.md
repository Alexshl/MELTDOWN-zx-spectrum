---
name: quick-start
description: Onboarding workflow for a new ZX Spectrum project built on zx-framework. Runs a discovery interview (project type, target hardware, graphics/sound/input choices), writes docs/PROJECT.md as the project vision, generates the first 3–7 implementation tasks in docs/tasks/, verifies the Docker toolchain builds and passes smoke, and hands off to the /task pipeline. Invoke when the user runs /quick-start or asks to start a new project on top of this framework.
---

# /quick-start — new project onboarding

This skill turns a fresh `zx-framework` checkout into a planned, buildable project. It interviews the user, captures the vision in `docs/PROJECT.md`, generates the first tasks, and verifies the toolchain. After it finishes, the user drives implementation with `/task <id>`.

The workflow below is a procedure **you (the main session) follow directly**. Use `AskUserQuestion` for the interview. Write only to `docs/PROJECT.md` and `docs/tasks/` — never touch `src/`, `Makefile.inner`, `.claude/`, the agents, or any other framework file.

---

## Step 1 — Sanity checks

1. **Docker availability.** Run `docker --version` and `docker compose version`. If either is missing, tell the user to install Docker Desktop (https://www.docker.com/products/docker-desktop/) and stop — nothing else works without it.
2. **Fresh skeleton check.** Read `src/main.c`. The stock skeleton is a single `printf("ZX Framework ready\n")` followed by an `intrinsic_halt()` loop. If it contains anything else, a project may already be underway — ask the user via `AskUserQuestion` whether to proceed (overwriting the onboarding) or abort.
3. **Empty roadmap check.** Read `docs/tasks/INDEX.md`. If the progress table already lists real tasks, ask the user whether to proceed or abort.

If the user aborts at either check, stop cleanly.

---

## Step 2 — Discovery interview

Run the interview in **3–4 rounds** of `AskUserQuestion` (do not cram everything into one call). For each question give 2–4 concrete options; the implicit "Other" lets the user free-type. Suggested topic groups:

**Round 1 — Project shape**
- Project type: `Game` / `Utility` / `Demo` / `Port of an existing program`.
- Ask (free-form, via the "Other" path or a follow-up) for a one-line description of the idea.

**Round 2 — Target hardware**
- Target: `48K only` / `128K only` / `Dual-target (48K + 128K)`.
- Does it need AY-3-8912 sound? (`Yes — 128K AY music` / `No — beeper only` / `No sound`). AY implies a 128K target.

**Round 3 — Presentation & input**
- Visual style: `Text mode (ROM font)` / `Character-cell graphics (8×8 attribute cells)` / `Pixel-precise drawing` / `Sprites with software masking`.
- Input: `Keyboard — QAOP` / `Keyboard — cursor/Sinclair` / `Kempston joystick` / `Interface 2`.

**Round 4 — External references (optional)**
- "Do you want to study an existing ZX binary before designing?" `Yes` / `No`.
- If **Yes**: tell the user to drop the file into `imports/` and run, separately:
  ```
  FILE=imports/<file>.tap Q="<your question>" docker compose run --rm investigate
  ```
  Make clear the skill does **not** run the investigator itself — the user does it in another command, then the interview/onboarding can resume.

Collect all answers before moving on.

---

## Step 3 — Write `docs/PROJECT.md`

Create `docs/PROJECT.md` with these sections, filled from the interview. Keep it short and factual:

```markdown
# <Project name>

## Vision
<1–2 sentences: what the program is and who it's for>

## Target
<48K / 128K / dual; note AY usage if any>

## MVP scope
- <3–7 bullet points of what the first shippable version does>

## Out of scope
- <features deliberately deferred>

## Key architectural decisions
- Graphics: <chosen style>
- Audio: <beeper / AY / none>
- Input: <chosen scheme>
- <any other decision captured in the interview>
```

---

## Step 4 — Generate the first tasks

Create between **3 and 7** task files in `docs/tasks/`, using `docs/tasks/_template.md` as the structure. Numbering starts at `01`; slugs are short kebab-case. Each task **must** include a `## Test plan` block using one of the three modes (`skip`, `smoke-only`, `scenarios: [...]`) and start with `**Status**: TODO`.

- **Task 01 is always** "Verify toolchain skeleton builds and runs" — it confirms `docker compose run --rm build` produces `build/app.tap` and `smoke` passes with the stock `src/main.c`. Test plan: `smoke-only`.
- **Subsequent tasks** follow a sensible bootstrap order for the chosen project type. Pick a relevant subset, e.g.:
  - "Set up the screen rendering primitive" (visual projects) — usually `smoke-only`.
  - "Implement input polling" (keyboard / Kempston) — `smoke-only` or `scenarios`.
  - "Implement the first interactive screen / menu" — `scenarios`.
  - "Add the core game state structure" (games) or "Implement the main utility routine" (utilities).
  - "Hook up sound" (only if audio is in scope).
- Keep each task's scope tight — one coherent slice of work with checkable acceptance criteria. Reference the relevant `zx-arch` topics in the Context section where they apply (screen layout, keyboard matrix, beeper, etc.).

---

## Step 5 — Update `docs/tasks/INDEX.md`

Replace the empty placeholder row in the `## Progress` table with one row per generated task: id, link to the file, status `TODO`, and expected output files. Update the "Out of scope" section from `docs/PROJECT.md` if useful.

---

## Step 6 — Build verification

Run these in order and report each result:

1. `docker compose build` (only if the image isn't built yet). If it fails, stop and surface the error — do not proceed.
2. `docker compose run --rm build` to compile the skeleton. Confirm `build/app.tap` is produced.
3. `docker compose run --rm smoke`. Summarize `artifacts/smoke.txt` (exit code, PC value, screenshot size).

If any step fails, **stop and report the toolchain problem first** — a broken toolchain makes the generated tasks unusable until fixed.

---

## Step 7 — Handoff

- Show the user `docs/PROJECT.md` and the list of generated tasks (id + title + status).
- Confirm the build + smoke result.
- Tell the user the next step: run `/task 01` to start the pipeline.

---

## Constraints

- Write ONLY to `docs/PROJECT.md` and `docs/tasks/`. Do not edit `src/`, `Makefile.inner`, `compose.yaml`, `.claude/`, the agents, or any other framework file.
- Do not run the investigator yourself — only instruct the user how to (Step 2, Round 4).
- All generated content (PROJECT.md, task files) is written in **English**.
- Do not start implementing tasks — that's the `/task` pipeline's job. This skill only plans and verifies.

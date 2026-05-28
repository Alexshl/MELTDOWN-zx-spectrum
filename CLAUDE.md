# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working on a project built on top of **zx-framework**.

## Project

`zx-framework` is a starter framework for **ZX Spectrum 48K / 128K** programs (games, utilities, demos) written in C, compiled with **z88dk** (C → Z80 asm → `.tap`) inside Docker, and executed in headless **ZEsarUX** for automated tests. A concrete project lives in this repository on top of the framework: its sources sit in `src/`, its roadmap in `docs/tasks/`, and its vision (when created via `/quick-start`) in `docs/PROJECT.md`.

A fresh checkout ships with a hello-world `src/main.c` so the toolchain is ready out of the box.

## Quick start (fresh project)

```
/quick-start
```

Runs a discovery interview (project type, target hardware, graphics / sound / input choices), writes `docs/PROJECT.md`, generates the first 3–7 implementation tasks in `docs/tasks/`, then verifies the Docker toolchain by running a build and a smoke test. After it returns, drive each task with `/task <id>`.

Full skill: [.claude/skills/quick-start/SKILL.md](.claude/skills/quick-start/SKILL.md).

## Source of truth

Implementation proceeds **strictly through tasks** in `docs/tasks/`:

- `docs/tasks/INDEX.md` — progress index with statuses (TODO / IN PROGRESS / BLOCKED / DONE) and execution order.
- `docs/tasks/NN-<slug>.md` — one task per file with its own acceptance criteria and `## Test plan` block.
- `docs/PROJECT.md` — short project vision (created by `/quick-start` or manually).

Before any code change, read the relevant task. Do not deviate from its acceptance criteria and do not add features outside the MVP scope.

## Build / run

```bash
docker compose build                   # build/update the image (one-time, ~5–10 minutes)
docker compose run --rm build          # produce build/app.tap
docker compose run --rm shell          # interactive shell inside the container
docker compose run --rm smoke          # smoke test: loads the .tap, checks PC ≠ 0x0000
docker compose run --rm integration    # JSON integration scenarios over ZRCP
```

Compose configuration: `compose.yaml`. Environment overrides: copy `.env.example` to `.env`. Only **Docker Desktop** is required on the host — no host-side z88dk or emulator. Details: `docker/README.md`.

## Architecture (recommended patterns)

The framework does not impose a project architecture, but the following patterns are battle-tested for Spectrum-class targets and are what most projects end up with:

- **Single-threaded frame loop at 50 Hz** synchronised by `intrinsic_halt()` on the vsync interrupt.
- **All state in a single global struct** — on Z80 this is both faster and easier to read than threading pointers through every call. No heap allocations.
- **Strict module layering with no cycles** — typically `main → (input, game, render, sound)`, where `render` knows nothing about `game` semantics, only how to draw cells / glyphs at given coordinates.
- **Character-cell graphics** when possible — one logical cell = one 8×8 attribute cell makes Spectrum's attribute clash work for you instead of against you.
- **`const` lookup tables** must end up in the code segment (handled automatically by z88dk with the right pragmas in `zpragma.inc`) so they don't eat RAM.
- **No `float`** — Z80 emulates it expensively. Use integer arithmetic (`uint8_t` / `uint16_t` / `uint32_t`).

These are recommendations, not requirements — the concrete architecture is the project's call and is documented in `docs/PROJECT.md`.

## Workflow: 5-agent pipeline

Every task in `docs/tasks/` flows through **five agents in strict order**:

1. **planner** — reads the task, cross-checks the current code and official z88dk documentation, returns a refined implementation plan. If the task does not specify a needed API or formula, the planner searches the web — it does **not** invent.
2. **coder** — implements the plan. Rebuilds after each change, records errors.
3. **tester** — parses the task's `## Test plan` section, runs the corresponding compose services, reads artifacts (`artifacts/smoke.txt`, `artifacts/integration-*.json`), and returns **PASS / FAIL / INFRA_ERROR**. Does not edit code.
4. **reviewer** — verifies each acceptance criterion against the diff and tester artifacts. May return **REWORK** with a concrete list of problems.
5. **documenter** — sets the task status to **DONE** in `docs/tasks/INDEX.md` and the task header, appends a short completion note.

Transitions:
- **tester** returns **FAIL** → back to **coder** (or **planner** if `suggested_next_agent = planner`).
- **tester** returns **INFRA_ERROR** → pipeline halts without REWORK; the user is told the infrastructure (Docker, ZEsarUX) is unresponsive.
- **reviewer** returns **REWORK** → back to **coder** with a specific list of issues.
- **coder** discovers the plan is wrong → back to **planner**.

The main session **does not write code directly** for tasks in `docs/tasks/`. It only orchestrates the pipeline.

### Required: Test plan in every task

Every `docs/tasks/NN-*.md` file **must** contain a `## Test plan` section in one of three modes:
- `skip: <reason>` — for tasks with no runtime behaviour (documentation, infrastructure).
- `smoke-only: <expectation>` — for tasks where a single `make smoke` run is sufficient.
- `scenarios: [name1, ...]` — for tasks with specific integration scenarios.

Without this section the tester returns FAIL with `suggested_next_agent = planner`.

New task template: `docs/tasks/_template.md`.

### Running the pipeline

The slash command `/task <id>` runs the pipeline for one task:

```
/task <id>
/task             # picks up the first TODO in INDEX.md
```

Detailed instructions per role: `.claude/agents/{planner,coder,tester,reviewer,documenter}.md`. Pipeline itself: `.claude/skills/task/SKILL.md`.

## Hardware reference

The **`zx-arch`** skill (`.claude/skills/zx-arch/SKILL.md`) is a reference for ZX Spectrum 48K/128K architecture: memory map, screen layout (with the pixel-address formula), attribute file, keyboard matrix (port 0xFE), beeper, AY-3-8912, 50 Hz interrupt, 128K bank switching via port 0x7FFD. Facts are sourced from breakintoprogram.co.uk, worldofspectrum.org, and sinclair.wiki.zxnet.co.uk.

Agents `planner`, `coder`, and `reviewer` consult it before any work involving direct addresses, ports, or manual screen-address calculations.

## Investigator subsystem

Ad-hoc research agent (**not** part of the 5-agent pipeline). Takes an arbitrary Z80 binary and a free-form user question.

```bash
FILE=imports/some_game.tap Q="how is the main loop structured" docker compose run --rm investigate
```

`tools/investigate.sh` first runs recon (`file`, `hexdump`, `strings`, `z88dk-dis`, `z80dasm`) into `artifacts/investigations/<timestamp-slug>/`, then the main session invokes the `investigator` agent against that directory.

Drop foreign binaries into `imports/` (gitignored — payload doesn't ship with the framework). See [imports/README.md](imports/README.md).

Constraints:
- Model: opus (declared in the agent frontmatter).
- Read-only outside `artifacts/investigations/<dir>/`. Does not edit `src/`, `docs/`, `tools/`, `docker/`, `.claude/`, `Makefile.inner`, `CLAUDE.md`, or `zpragma.inc`.
- Tools: Read, Grep, Glob, Bash (whitelist: z88dk-dis, z80dasm, hexdump, xxd, file, strings, `docker compose run --rm disasm`/`disasm-alt`/`trace`/`integration`, python3), WebSearch, WebFetch.

Full spec: `.claude/agents/investigator.md`.

## Prohibitions

- **Do not invent z88dk APIs.** If a function is not mentioned in the task, look it up in the [z88dk wiki](https://github.com/z88dk/z88dk/wiki/Platform---Sinclair-ZX-Spectrum) or the [getting-started doc](https://github.com/z88dk/z88dk/blob/master/doc/ZXSpectrumZSDCCnewlib_01_GettingStarted.md) via WebFetch / WebSearch before using it.
- **Do not exceed the task scope.** If a useful feature surfaces that is not in the acceptance criteria, leave a note in the task file's "Out of scope" section — but do not implement it.
- **Do not change the framework architecture without justification.** If a task seems to require restructuring the framework (agents, pipeline, Docker layer), mark it **BLOCKED** and create a new task to align with the user first.
- **Do not edit `docs/PROJECT.md`** without explicit user request once it has been created — it captures the original vision; subsequent deviations live in task completion notes.

## Code style

- C89-compatible baseline (both sccz80 and zsdcc support it), but `stdint.h` types and `//` comments are allowed (z88dk accepts them).
- No dynamic memory: static arrays and global state only.
- No `float` — Z80 emulates it expensively; use `uint32_t` and friends.
- Function naming: `module_action` (`game_tick`, `render_cell`, `input_poll`).
- Comments only when the *why* is non-obvious — not the *what*.

# zx-framework

A starter framework for building ZX Spectrum 48K / 128K programs (games, utilities, demos) in C with [z88dk](https://github.com/z88dk/z88dk), driven end-to-end from [Claude Code](https://claude.ai/code).

Everything you need to compile, run, smoke-test, integration-test, trace, disassemble, and reverse-engineer foreign binaries — fully containerised. No host toolchain required other than Docker.

---

## What's in the box

- **Dockerised toolchain** — `z88dk` for C → Z80 → `.tap`, `ZEsarUX` headless for ZRCP control, `z80dasm` for disassembly, `python3` for scripted integration scenarios.
- **5-agent task pipeline** — every task in `docs/tasks/` flows through `planner → coder → tester → reviewer → documenter` via `/task <id>`. Each step is constrained, documented, and audited against the task's acceptance criteria.
- **Investigator subsystem** — drop a foreign `.tap` / `.sna` / `.rom` into `imports/`, ask a free-form question, and an Opus-grade research agent produces a recon report rooted in real disassembly evidence.
- **Hardware reference skill** — `.claude/skills/zx-arch` ships authoritative facts about the Spectrum's memory map, screen layout, attribute file, keyboard matrix, beeper, AY-3-8912, 50 Hz interrupt, and 128K bank switching, with links to the original sources.
- **Quick-start onboarding** — `/quick-start` interviews you about the project, drafts `docs/PROJECT.md`, generates the first few tasks, and verifies the toolchain — all in one go.

---

## Quick start

```bash
git clone https://github.com/Alexshl/zx-framework.git my-project
cd my-project
cp .env.example .env                      # adjust UID/GID if needed
docker compose build                      # one-time, ~5–10 min
```

Then, in Claude Code from this directory:

```
/quick-start
```

The skill interviews you, generates `docs/PROJECT.md` and the first tasks, and runs a build + smoke test. After that, drive each task with:

```
/task <id>
```

If you prefer to skip the interview, just edit `src/main.c`, create your tasks manually from `docs/tasks/_template.md`, and run `docker compose run --rm build`.

---

## Docker services

All commands are run as `docker compose run --rm <service>`.

| Service        | Purpose                                                                  |
| -------------- | ------------------------------------------------------------------------ |
| `build`        | Compile `src/*.c` → `build/app.tap` via z88dk.                           |
| `shell`        | Interactive bash inside the container with the full toolchain.           |
| `smoke`        | Load the `.tap` into headless ZEsarUX, verify PC ≠ 0x0000, snap screen.  |
| `integration`  | Run JSON scenarios from `tools/integration/scenarios/` over ZRCP.        |
| `trace`        | `z88dk-ticks` cycle-accurate trace of a binary (set `BIN=...` `CYCLES=...`). |
| `disasm`       | `z88dk-dis` disassembly of the current build (uses the `.map`).          |
| `disasm-alt`   | `z80dasm` alternative disassembler.                                      |
| `investigate`  | Recon + Opus-grade analysis of a foreign binary (set `FILE=...` `Q=...`).|

Override env vars on the command line, e.g.:

```bash
FILE=imports/some_game.tap Q="how is the main loop structured" \
  docker compose run --rm investigate

BIN=build/app_CODE.bin CYCLES=2000000 \
  docker compose run --rm trace
```

**Linux ownership note.** By default the container runs as `UID:GID = 1000:1000`. If your host UID differs, files in `build/` will be owned by that fixed UID. Fix it by editing `.env`:

```bash
echo "UID=$(id -u)"  > .env
echo "GID=$(id -g)" >> .env
```

macOS and Windows Docker Desktop handle bind-mount ownership transparently — no `.env` adjustment needed.

---

## Repository layout

```
.
├── src/                 your C sources (starts with a hello-world skeleton)
├── docs/
│   ├── PROJECT.md       project vision (created by /quick-start)
│   └── tasks/           per-task specs + INDEX.md roadmap
├── tools/               smoke/trace/disasm/investigate scripts + integration harness
├── docker/              Dockerfile + entrypoint dispatcher
├── imports/             drop foreign binaries here (gitignored)
├── artifacts/           build outputs, smoke screenshots, investigation reports
├── build/               compiled .tap / .map / .bin (gitignored)
├── .claude/
│   ├── agents/          planner, coder, tester, reviewer, documenter, investigator
│   ├── skills/          zx-arch, task, quick-start
│   └── commands/        slash commands (quick-start, ...)
├── compose.yaml         all Docker services in one place
├── Makefile.inner       inner build (runs inside the container)
├── zpragma.inc          z88dk pragmas (STACKPTR, CRT_ORG_CODE for 128K)
└── CLAUDE.md            project guidance for Claude Code
```

---

## Task workflow

Tasks live in `docs/tasks/NN-<slug>.md`. Each one carries acceptance criteria and a `## Test plan` block (`skip`, `smoke-only`, or `scenarios: [...]`). Run `/task <id>` and the pipeline takes over:

1. **planner** reads the task, cross-checks the current code and z88dk docs, produces a refined plan.
2. **coder** implements the plan, rebuilds after each change.
3. **tester** runs the test plan against `make smoke` / integration scenarios, returns PASS / FAIL / INFRA_ERROR.
4. **reviewer** audits each acceptance criterion against the diff and tester artifacts. Returns APPROVED or REWORK.
5. **documenter** marks the task DONE in `docs/tasks/INDEX.md` and the task header, adds a completion note.

FAIL or REWORK loops back to coder (or planner if the plan itself was wrong). INFRA_ERROR halts the pipeline. The full orchestrator lives in [.claude/skills/task/SKILL.md](.claude/skills/task/SKILL.md).

---

## Investigating foreign binaries

The investigator is a separate, ad-hoc agent (not part of the 5-agent pipeline). Use it to study existing ZX software before designing something inspired by it:

```bash
cp ~/Downloads/some_game.tap imports/
FILE=imports/some_game.tap Q="how does it access screen memory" \
  docker compose run --rm investigate
```

The wrapper runs `file`, `hexdump`, `strings`, `z88dk-dis`, and `z80dasm` into `artifacts/investigations/<timestamp-slug>/`, then hands the directory to an Opus-class agent that produces a markdown report separating **Verified** from **Hypothesis** findings. The agent is read-only outside its investigation directory.

Drop the conclusions into the planner as inspiration when shaping a new task.

---

## References

- [z88dk wiki — ZX Spectrum](https://github.com/z88dk/z88dk/wiki/Platform---Sinclair-ZX-Spectrum)
- [z88dk getting started — ZSDCC newlib](https://github.com/z88dk/z88dk/blob/master/doc/ZXSpectrumZSDCCnewlib_01_GettingStarted.md)
- [Sinclair ZX Spectrum wiki](https://sinclair.wiki.zxnet.co.uk/)
- [World of Spectrum — hardware notes](https://worldofspectrum.org/faq/reference/48kreference.htm)
- [breakintoprogram.co.uk — ZX architecture](https://www.breakintoprogram.co.uk/computers/zx-spectrum/)
- [ZEsarUX emulator](https://github.com/chernandezba/zesarux)

---

## License

MIT. See [LICENSE](LICENSE) if shipped with one in the downstream project.

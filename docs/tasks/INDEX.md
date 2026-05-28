# Tasks — Project Roadmap

This file lists the implementation tasks for the current project built on `zx-framework`.
Each task lives in its own `NN-<slug>.md` file in this directory and is driven through the 5-agent pipeline (`/task <id>`).

For a fresh project, run [`/quick-start`](../../.claude/commands/quick-start.md) — it generates `docs/PROJECT.md` and the initial set of tasks here.

## Progress

| #   | Task | Status | Output files |
| --- | ---- | ------ | ------------ |
| _no tasks yet — run `/quick-start` or create the first one from [`_template.md`](_template.md)._ | | | |

## Status legend

- **TODO** — not started
- **IN PROGRESS** — currently being worked on
- **BLOCKED** — waiting for clarification or an external dependency
- **DONE** — implemented and verified

## How to work with this index

1. Open the current task by its link in the table above.
2. Follow the steps, check the acceptance criteria.
3. Use `docker compose run --rm build` (and `smoke` / `integration` once the relevant tasks land) to verify.
4. Once everything is clean, change the status to **DONE** here and in the task header.
5. Move to the next task.

## Out of scope (for future tasks)

- _Track ideas that are not in the current MVP scope but should not be forgotten._

---
name: task
description: Run a task from docs/tasks/ through the 5-agent pipeline (planner → coder → tester → reviewer → documenter). Usage — /task <id> for a specific task, or /task without args to pick up the next TODO. The skill orchestrates the pipeline, handles REWORK loops, and stops only when the task is DONE or genuinely blocked.
---

# /task — task pipeline

Runs a task from `docs/tasks/` through the strict pipeline **planner → coder → tester → reviewer → documenter**. The main session **does not write code itself** — it only orchestrates the agents.

## Argument

- `/task <id>` — run the task with this ID.
- `/task` (no argument) — pick the first `TODO` in `docs/tasks/INDEX.md`.

## Steps

### 0. Determine the task

1. Read `docs/tasks/INDEX.md`.
2. If an argument is given, take `docs/tasks/<id>-*.md` (find by prefix via Glob).
3. If no argument, find the first row with status `TODO` and take its ID.
4. If all tasks are `DONE`, say "all tasks complete" and exit.

### 1. Set status to IN PROGRESS

Via **Edit** in `docs/tasks/INDEX.md` and the task file header: `TODO → IN PROGRESS`. This is done by the main session, not an agent — it's just a marker.

### 2. Planner phase

Invoke the `planner` agent via the **Agent** tool:

```
Agent({
  subagent_type: "planner",
  description: "Plan task NN",
  prompt: "Task: docs/tasks/NN-*.md. Read it and produce a refined plan using the template in your system prompt. This is the first run — no feedback."
})
```

Save the output as `plan_v1`.

### 3. Coder phase

Invoke `coder` with `plan_v1`:

```
Agent({
  subagent_type: "coder",
  description: "Implement task NN",
  prompt: "Refined plan below. Implement it per your system prompt.\n\n<insert plan_v1>"
})
```

Save the output as `code_report_v1`.

If `code_report_v1.status == HALT_NEED_PLAN_FIX`:
- Go back to step **2** with the additional feedback from `code_report_v1.feedback_to_planner` in the planner's prompt.
- Maximum 3 planner ↔ coder iterations. If it HALTs a third time, set status to **BLOCKED** and tell the user.

### 3.5. Tester phase

Invoke `tester`:

```
Agent({
  subagent_type: "tester",
  description: "Test task NN",
  prompt: "Task: docs/tasks/NN-*.md\nCoder report: <code_report_vN>\n\nRun the test plan, return PASS/FAIL/INFRA_ERROR per your template."
})
```

Save the output as `test_report_v1`.

If `test_report_v1.status == INFRA_ERROR`:
- Set the task status to **BLOCKED** in INDEX.md and the task header.
- Tell the user: "infrastructure: Docker unresponsive — start Docker Desktop and retry".
- **Do NOT trigger REWORK.**

If `test_report_v1.status == FAIL`:
- If `suggested_next_agent == "planner"` → step **2** (planner) with feedback from failures.
- Otherwise → step **3** (coder) again with failures as feedback.
- coder ↔ tester limit = **3 iterations**. If it FAILs on the third, set status to **BLOCKED** and tell the user.

If `test_report_v1.status == PASS` → step **4**.

### 4. Reviewer phase

Invoke `reviewer`:

```
Agent({
  subagent_type: "reviewer",
  description: "Review task NN",
  prompt: "Task: docs/tasks/NN-*.md\nRefined plan: <plan_vN>\nCoder report: <code_report_vN>\nTester report: <test_report_vN>\n\nReturn a verdict per your template."
})
```

Save the output as `review_v1`.

If `review_v1.status == REWORK`:
- If `review_v1.suggested_next_agent == "planner"` → step **2** with feedback from the issues.
- Otherwise → step **3** again with the issues as feedback for the coder.
- Maximum 3 coder ↔ reviewer iterations. If REWORK a third time → **BLOCKED**.

If `APPROVED` → step **5**.

### 5. Documenter phase

Invoke `documenter`:

```
Agent({
  subagent_type: "documenter",
  description: "Mark task NN done",
  prompt: "Task NN is complete. Reviewer note: <short excerpt>. Today's date: <YYYY-MM-DD from system context>. Update INDEX.md and the task file header. Act per your system prompt."
})
```

### 6. Final report to the user

One short block:

```
Task NN — DONE
Planner iterations: X
Coder iterations: Y
Reviewer verdict: APPROVED
Files changed: ...
Next pending task: task MM — <title>

Manual check: docker compose run --rm smoke (or load build/app.tap in an emulator) → walk through the task's acceptance criteria.
```

## Orchestration rules

- **The main session does not write code directly.** Only Edit to mark IN PROGRESS and the final user report. Everything else goes through Agent.
- **Each agent receives only its own inputs** (see their system prompts). Don't hand the task directly to the coder — it works from the planner's plan.
- **Iteration limit** — 3 for each pair (planner↔coder, coder↔tester, coder↔reviewer). After the limit → BLOCKED.
- **INFRA_ERROR from tester** — immediate stop, no REWORK. Don't re-run the coder. Wait for the user to bring infrastructure up.
- **BLOCKED** means: set status to **BLOCKED** in INDEX.md and the task header, and tell the user exactly what blocked it (the last feedback / last issues list).

## What /task does NOT do

- Doesn't pick a task for the user without an explicit request (but `/task` with no argument does take the first TODO).
- Doesn't reorganize `docs/tasks/`.
- Doesn't edit `CLAUDE.md` or the agents.
- Doesn't touch other tasks, even if one looks wrong — that's a separate conversation with the user.

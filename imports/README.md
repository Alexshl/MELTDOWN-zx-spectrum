# imports/

Drop foreign ZX Spectrum binaries here (`.tap`, `.sna`, `.z80`, `.rom`, raw `.bin`) for analysis with the [investigator](../.claude/agents/investigator.md) subsystem.

Files in this directory are gitignored (the directory itself is kept via `.gitkeep`). Nothing here ships with the framework.

## Typical workflow

1. Copy the binary you want to study into `imports/`:
   ```
   cp ~/Downloads/some_game.tap imports/
   ```
2. Run the investigator with a free-form question:
   ```
   FILE=imports/some_game.tap Q="how is the main loop structured" docker compose run --rm investigate
   ```
3. Recon output and the investigator's report land in `artifacts/investigations/<timestamp-slug>/`.
4. Use the findings as input for the `planner` agent when designing a new feature inspired by what you discovered.

## Tips

- For `.tap` files the investigator extracts CODE blocks and disassembles them at their load address.
- For raw binaries set `ORG=0x<addr>` to give the disassembler the right origin.
- For full memory snapshots (`.sna`, `.z80`) you can also load them in `docker compose run --rm shell` and explore with `z80dasm` directly.

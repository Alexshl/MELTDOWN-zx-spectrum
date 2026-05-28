# Docker environment for zx-framework

## Requirements

- Docker Desktop (macOS, Linux or Windows WSL2)
- No host z88dk or ZEsarUX installation needed

## Quick start

```sh
# Build & dev
docker compose build                              # build / update the image
docker compose run --rm build                     # compile build/app.tap
docker compose run --rm shell                     # interactive bash inside the container

# Test
docker compose run --rm smoke                     # smoke test via ZRCP
docker compose run --rm integration               # run all integration scenarios

# Debug
BIN=build/app_CODE.bin docker compose run --rm trace
CYCLES=200000 docker compose run --rm trace

# Disassembly
docker compose run --rm disasm                    # z88dk-dis with symbol map
docker compose run --rm disasm-alt                # z80dasm

# Research
FILE=imports/sample.tap Q="main loop structure" docker compose run --rm investigate
```

## Environment variables

Copy `.env.example` to `.env` and adjust as needed:

```bash
cp .env.example .env
```

Key variables:

| Variable | Default | Description |
|----------|---------|-------------|
| `UID` | `1000` | Host user ID for artifact ownership (Linux/macOS) |
| `GID` | `1000` | Host group ID for artifact ownership (Linux/macOS) |
| `IMAGE_TAG` | `zx-framework:latest` | Docker image tag |
| `BIN` | `build/app_CODE.bin` | Binary for `trace` service |
| `CYCLES` | _(empty)_ | CPU cycle limit for `trace` |
| `FILE` | _(required)_ | Input file for `investigate` service |
| `Q` | _(required)_ | Question for `investigate` service |
| `ORG` | `0x6000` | Origin address for `investigate` disassembly |

## Image contents

| Tool | Source |
|------|--------|
| `zcc`, `z88dk-ticks`, `z88dk-dis` | Official `z88dk/z88dk:latest` image |
| `zesarux` | Built from source tag `ZEsarUX-12.1` with SDL2 |
| `z80dasm` | Ubuntu 24.04 apt package |
| `make`, `gcc` | Ubuntu 24.04 apt packages |

## Headless ZEsarUX

ZEsarUX runs headless via `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy`.
The ZRCP (remote protocol) is enabled at runtime with `--enable-remoteprotocol`
and listens on port 10000.

## Apple Silicon (arm64)

`z88dk/z88dk:latest` is an amd64-only image. Docker Desktop runs it via
Rosetta 2 transparently. Build times may be slightly longer.

## entrypoint subcommands

| Subcommand | Description |
|------------|-------------|
| `build` (default) | Runs `make -f Makefile.inner` to produce `build/app.tap` |
| `shell` | Drops into an interactive bash shell |
| `smoke` | Loads `.tap` into ZEsarUX headless, verifies PC does not halt at 0x0000 |
| `integration` | Runs ZRCP integration scenarios from `tools/integration/scenarios/` |
| `trace` | Execution tracing via z88dk-ticks |
| `disasm` | Disassembly via z88dk-dis |
| `disasm-alt` | Disassembly via z80dasm |
| `investigate` | Ad-hoc binary analysis with recon + AI investigation |

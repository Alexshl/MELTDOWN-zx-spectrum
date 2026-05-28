#!/usr/bin/env bash
# disasm-alt.sh — disassemble app_CODE.bin using z80dasm.
# Produces /work/artifacts/app.z80dasm.asm
set -euo pipefail

BIN="${1:-/work/build/app_CODE.bin}"
OUT="/work/artifacts/app.z80dasm.asm"

mkdir -p /work/artifacts

if [[ ! -s "$BIN" ]]; then
    echo "disasm-alt: binary not found or empty: $BIN" >&2
    exit 2
fi

# --labels: generate labels; --origin: load address (ORG = 0x6000)
z80dasm --labels --origin=0x6000 --output="$OUT" "$BIN"

echo "disasm-alt: wrote $OUT ($(wc -l < "$OUT") lines)"

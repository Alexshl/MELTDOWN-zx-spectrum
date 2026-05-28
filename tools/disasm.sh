#!/usr/bin/env bash
# disasm.sh — disassemble app_CODE.bin using z88dk-dis with symbol map.
# Produces /work/artifacts/app.dis.asm
set -euo pipefail

BIN="${1:-/work/build/app_CODE.bin}"
MAP="${2:-/work/build/app.map}"
OUT="/work/artifacts/app.dis.asm"

mkdir -p /work/artifacts

if [[ ! -s "$BIN" ]]; then
    echo "disasm: binary not found or empty: $BIN" >&2
    exit 2
fi

# -mz80: Z80 target; -o: load address (ORG = 0x6000); -x: symbol map for labels
z88dk-dis -mz80 -o 6000 -x "$MAP" "$BIN" > "$OUT"

echo "disasm: wrote $OUT ($(wc -l < "$OUT") lines)"

#!/usr/bin/env bash
# trace.sh — run z88dk-ticks to trace execution of a .bin file.
# Usage: trace.sh [bin_path]
# Produces /work/artifacts/trace-<basename>.log
# Exit codes:
#   0  — success (output > 1 KB)
#   2  — binary not found or empty
#   3  — trace output is empty or too small (< 1 KB)
set -euo pipefail

BIN="${1:-/work/build/app_CODE.bin}"
MAP="${2:-/work/build/app.map}"
CYCLES="${CYCLES:-2000000}"

BASENAME=$(basename "$BIN")
OUT="/work/artifacts/trace-${BASENAME%.bin}.log"

mkdir -p /work/artifacts

if [[ ! -s "$BIN" ]]; then
    echo "trace: binary not found or empty: $BIN" >&2
    exit 2
fi

MAP_ARGS=()
if [[ -s "$MAP" ]]; then
    MAP_ARGS=(-x "$MAP")
fi

# Run tracer; z88dk-ticks exits non-zero when counter limit is reached — that's OK.
# Note: -b zx flag is documented but not supported in this version; omitted.
z88dk-ticks -mz80 -pc 6000 -trace -counter "$CYCLES" \
    "${MAP_ARGS[@]}" "$BIN" > "$OUT" 2>&1 || true

if [[ ! -s "$OUT" ]]; then
    echo "trace: output file is empty" >&2
    exit 3
fi

SIZE=$(stat -c%s "$OUT" 2>/dev/null || stat -f%z "$OUT")
if [[ "$SIZE" -lt 1024 ]]; then
    echo "trace: output too small ($SIZE bytes), expected > 1 KB" >&2
    exit 3
fi

echo "trace: wrote $OUT ($SIZE bytes)"

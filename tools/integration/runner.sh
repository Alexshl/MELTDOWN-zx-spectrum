#!/usr/bin/env bash
# runner.sh — launches ZEsarUX with ZRCP, runs integration scenario, kills ZEsarUX.
# Usage: runner.sh <scenario.json> [tap_path] [map_path]
# Exit codes mirror run.py: 0=pass, 1=fail, 2=missing, 3=zrcp error
set -euo pipefail

SCENARIO="${1:-}"
TAP="${2:-/work/build/app.tap}"
MAP="${3:-/work/build/app.map}"
ZRCP_PORT=10000
ARTIFACTS=/work/artifacts
LOG="$ARTIFACTS/zesarux-integration.log"

ZEMU_PID=""

cleanup() {
    if [[ -n "$ZEMU_PID" ]]; then
        kill -TERM "$ZEMU_PID" 2>/dev/null || true
        sleep 1
        kill -KILL "$ZEMU_PID" 2>/dev/null || true
    fi
}
trap cleanup EXIT
trap '' PIPE

if [[ -z "$SCENARIO" ]]; then
    echo "runner.sh: no scenario given" >&2
    exit 2
fi

if [[ ! -s "$TAP" ]]; then
    echo "runner.sh: tap missing or empty: $TAP" >&2
    exit 2
fi

mkdir -p "$ARTIFACTS"

# ── launch ZEsarUX ───────────────────────────────────────────────────────────
SDL_VIDEODRIVER=dummy \
    zesarux \
        --machine 128k \
        --tape "$TAP" \
        --enable-remoteprotocol \
        --remoteprotocol-port "$ZRCP_PORT" \
        --quickexit \
        --exit-after 120 \
    >"$LOG" 2>&1 &
ZEMU_PID=$!

# ── wait for ZRCP TCP port ───────────────────────────────────────────────────
DEADLINE=$((SECONDS + 15))
CONNECTED=0
while [[ $SECONDS -lt $DEADLINE ]]; do
    if (exec 3<>/dev/tcp/127.0.0.1/$ZRCP_PORT) 2>/dev/null; then
        CONNECTED=1
        exec 3>&-
        break
    fi
    sleep 0.5
done

if [[ $CONNECTED -eq 0 ]]; then
    echo "runner.sh: ZRCP did not become ready within 15 seconds" >&2
    cat "$LOG" >&2
    exit 3
fi

# Wait for the 128K ROM to boot and auto-load the tape (takes ~4 seconds in smoke).
# The game runs BASIC loader which auto-runs via the tap header.
sleep 5

# ── run Python harness ───────────────────────────────────────────────────────
python3 /src/tools/integration/run.py "$SCENARIO" "$MAP"

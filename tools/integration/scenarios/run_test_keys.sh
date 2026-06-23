#!/usr/bin/env bash
# Quick script to run ZEsarUX + test_keys.py inside the Docker container
set -euo pipefail

TAP="/work/build/app.tap"
MAP="/work/build/app.map"
ZRCP_PORT=10000
LOG="/work/artifacts/zesarux-testkeys.log"

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

mkdir -p /work/artifacts

SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
    zesarux \
        --machine 128k \
        --tape "$TAP" \
        --enable-remoteprotocol \
        --remoteprotocol-port "$ZRCP_PORT" \
        --quickexit \
        --exit-after 120 \
    >"$LOG" 2>&1 &
ZEMU_PID=$!

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
    echo "ZRCP did not become ready" >&2
    cat "$LOG" >&2
    exit 3
fi

echo "ZRCP ready, waiting 5s for tape load..."
sleep 5

echo "Running test_keys.py..."
python3 /work/tools/integration/scenarios/test_keys.py

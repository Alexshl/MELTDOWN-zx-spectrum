#!/usr/bin/env bash
# smoke.sh — starts ZEsarUX with ZRCP, captures screenshot and registers.
# Usage: smoke.sh [tap_path [frames]]
# Exit codes:
#   0  — success
#   2  — tap missing/empty
#   3  — ZRCP not ready within 10 seconds
#   4  — smoke.scr not 6912 bytes
#   5  — PC == 0x0000
set -euo pipefail

TAP="${1:-/work/build/app.tap}"
FRAMES="${2:-200}"
ZRCP_PORT=10000
ARTIFACTS=/work/artifacts
LOG="$ARTIFACTS/zesarux.log"
SCR="$ARTIFACTS/smoke.scr"
TXT="$ARTIFACTS/smoke.txt"

ZEMU_PID=""

cleanup() {
    if [[ -n "$ZEMU_PID" ]]; then
        kill -TERM "$ZEMU_PID" 2>/dev/null || true
        sleep 2
        kill -KILL "$ZEMU_PID" 2>/dev/null || true
    fi
}
trap cleanup EXIT
# Prevent SIGPIPE from killing the script when ZEsarUX closes the ZRCP socket
trap '' PIPE

# ── sanity check ────────────────────────────────────────────────────────────
if [[ ! -s "$TAP" ]]; then
    echo "tap load failed: empty or missing: $TAP" >&2
    exit 2
fi

mkdir -p "$ARTIFACTS"

# ── launch ZEsarUX in background ─────────────────────────────────────────────
# SDL_VIDEODRIVER=dummy suppresses any display requirement inside the container.
SDL_VIDEODRIVER=dummy \
    zesarux \
        --machine 128k \
        --tape "$TAP" \
        --enable-remoteprotocol \
        --remoteprotocol-port "$ZRCP_PORT" \
        --quickexit \
        --exit-after $((FRAMES / 50 + 20)) \
    >"$LOG" 2>&1 &
ZEMU_PID=$!

# ── wait for ZRCP TCP port ────────────────────────────────────────────────────
DEADLINE=$((SECONDS + 10))
CONNECTED=0
while [[ $SECONDS -lt $DEADLINE ]]; do
    # bash /dev/tcp trick (requires bash 4+); suppress "Connection refused" noise
    if (exec 3<>/dev/tcp/127.0.0.1/$ZRCP_PORT) 2>/dev/null; then
        CONNECTED=1
        break
    fi
    sleep 0.5
done

if [[ $CONNECTED -eq 0 ]]; then
    echo "ZRCP did not become ready within 10 seconds" >&2
    cat "$LOG" >&2
    exit 3
fi

# ── ZRCP helper ──────────────────────────────────────────────────────────────
# The protocol sends a prompt after each response; we read until we see "command> "
# or a 2-second quiet timeout (implemented by reading byte-by-byte with a timeout).
# fd 3 is already open (reopen to ensure both directions).
exec 3<>/dev/tcp/127.0.0.1/$ZRCP_PORT

zrcp_read_response() {
    local resp="" line
    # Wait up to 5 seconds for the first byte to arrive.
    local deadline=$((SECONDS + 5))
    while [[ $SECONDS -lt $deadline ]]; do
        if IFS= read -r -t 0.5 -u 3 line; then
            resp+="$line"$'\n'
            break
        fi
    done
    # Drain any remaining lines; stop after 1s of silence (prompt has no \n).
    # This handles both short replies and large dumps (read-memory 768 bytes).
    while IFS= read -r -t 1 -u 3 line; do
        resp+="$line"$'\n'
    done
    printf '%s' "$resp"
}

zrcp_cmd() {
    local cmd="$1"
    printf '%s\n' "$cmd" >&3
    zrcp_read_response
}

# Discard initial banner/prompt
zrcp_read_response >/dev/null 2>&1 || true

# ── run commands ─────────────────────────────────────────────────────────────
VERSION=$(zrcp_cmd "get-version")

# ZRCP "run-for" is absent in ZEsarUX 12.1 — use host-side sleep instead.
SLEEP_SECS=$(( FRAMES / 50 + 1 ))
sleep "$SLEEP_SECS"

# Save screen
zrcp_cmd "save-screen $SCR" >/dev/null

REGS=$(zrcp_cmd "get-registers")

# 0x5800 = 22528 decimal; ZRCP ignores 0x prefix and would read from addr 0 (ROM).
ATTRS=$(zrcp_cmd "read-memory 22528 768")

# Graceful exit — ignore SIGPIPE if ZEsarUX already exited
{ zrcp_cmd "exit" >/dev/null || true; } 2>/dev/null || true
exec 3>&- 2>/dev/null || true

# ── write smoke.txt ──────────────────────────────────────────────────────────
{
    echo "=== ZEsarUX version ==="
    echo "$VERSION"
    echo "=== Registers ==="
    echo "$REGS"
    echo "=== Attribute file (first+last lines) ==="
    # read-memory returns one long hex line; fold into 32-byte rows (64 hex chars each)
    ATTRS_CHUNKED=$(echo "$ATTRS" | tr -d ' \n\r' | fold -w64)
    echo "$ATTRS_CHUNKED" | head -1
    echo "$ATTRS_CHUNKED" | tail -1
} >"$TXT"

# ── verify smoke.scr size ─────────────────────────────────────────────────────
if [[ ! -f "$SCR" ]]; then
    echo "smoke.scr not created" >&2
    exit 4
fi

SCR_SIZE=$(stat -c%s "$SCR" 2>/dev/null || stat -f%z "$SCR")
if [[ "$SCR_SIZE" -ne 6912 ]]; then
    echo "smoke.scr size is $SCR_SIZE, expected 6912" >&2
    exit 4
fi

# ── verify PC ≠ 0x0000 ────────────────────────────────────────────────────────
# get-registers output contains "PC=XXXX" (hex, uppercase, 4 digits)
PC_VAL=$(echo "$REGS" | grep -oE 'PC=([0-9A-Fa-f]+)' | head -1 | cut -d= -f2)
if [[ -z "$PC_VAL" || "$PC_VAL" == "0000" ]]; then
    echo "PC is 0x0000 — emulator likely crashed or did not start" >&2
    exit 5
fi

echo "smoke PASSED: PC=$PC_VAL, screen=$SCR_SIZE bytes"
echo "Artifacts: $SCR  $TXT"

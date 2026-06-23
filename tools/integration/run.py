#!/usr/bin/env python3
"""
integration/run.py — ZRCP-based integration test runner for zx-framework.

Connects to ZEsarUX Remote Control Protocol (already running on localhost:10000),
executes scenario steps, writes result JSON to /work/artifacts/.

Usage: run.py <scenario.json> [map_file]
Exit codes:
  0  success
  1  assertion failed
  2  scenario file not found
  3  ZRCP connection error
"""

import json
import os
import re
import socket
import sys
import time

ZRCP_HOST = "127.0.0.1"
ZRCP_PORT = 10000
ARTIFACTS_DIR = "/work/artifacts"

# ── Key mapping for ZX Spectrum games using in_key_pressed (hardware port reads) ─
# ZEsarUX ZRCP does not support injecting keys into the Z80 hardware port (0xFE)
# via send-keys-string or set-ui-io-ports. Instead, input.c exposes a one-shot
# command mailbox variable g_test_cmd. The runner writes the ASCII character to
# that address; input_poll reads it, acts, and clears it.
# KEY_MAP: scenario "key" name → ASCII character written to g_test_cmd.
KEY_MAP = {
    "LEFT":  "O",   # O = move left (in_key_pressed O)
    "RIGHT": "P",   # P = move right
    "DOWN":  "A",   # A = move down
    "UP":    "Q",   # Q = move up
    "SPACE": " ",   # SPACE = cycle turret
    "ENTER": "\r",  # ENTER (no-op this task)
    "P": "P",
    "Q": "Q",
    "A": "A",
    "O": "O",
    "M": "M",
}


class ZrcpError(Exception):
    pass


class Zrcp:
    """Thin wrapper around the ZEsarUX Remote Control Protocol socket."""

    def __init__(self, host=ZRCP_HOST, port=ZRCP_PORT):
        self._sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._sock.settimeout(10.0)
        try:
            self._sock.connect((host, port))
        except OSError as e:
            raise ZrcpError(f"Cannot connect to {host}:{port}: {e}") from e
        self._buf = b""
        # Drain the initial banner/prompt
        self._read_until_prompt()

    def _read_until_prompt(self, timeout=5.0):
        """Read bytes from socket until 'command> ' prompt is seen."""
        self._sock.settimeout(0.5)
        deadline = time.monotonic() + timeout
        data = b""
        while time.monotonic() < deadline:
            try:
                chunk = self._sock.recv(4096)
                if not chunk:
                    break
                data += chunk
                if b"command> " in data:
                    break
            except socket.timeout:
                if data:
                    break
        self._sock.settimeout(10.0)
        return data.decode("latin-1", errors="replace")

    def cmd(self, line: str) -> str:
        """Send command, return response text (without the prompt)."""
        self._sock.sendall((line + "\n").encode("latin-1"))
        resp = self._read_until_prompt(timeout=10.0)
        # Strip trailing prompt
        if resp.endswith("command> "):
            resp = resp[: -len("command> ")]
        return resp.strip()

    def close(self):
        try:
            self._sock.sendall(b"exit\n")
        except OSError:
            pass
        self._sock.close()


def parse_map(path: str) -> dict:
    """
    Parse z88dk map file.
    Format: SYMBOL = $HEXADDR ; tags...
    Returns dict of symbol_name -> decimal address.
    """
    syms = {}
    pattern = re.compile(r"^(\S+)\s+=\s+\$([0-9A-Fa-f]+)")
    try:
        with open(path) as f:
            for line in f:
                m = pattern.match(line)
                if m:
                    syms[m.group(1)] = int(m.group(2), 16)
    except FileNotFoundError:
        pass
    return syms


def resolve(expr: str, syms: dict) -> int:
    """
    Resolve an address expression like '_G+200' or '33680' to a decimal int.
    Supports: plain decimal, plain hex (0x...), symbol+offset, symbol.
    """
    expr = expr.strip()
    # Pure decimal
    if re.match(r"^\d+$", expr):
        return int(expr)
    # Pure hex
    if re.match(r"^0x[0-9A-Fa-f]+$", expr, re.I):
        return int(expr, 16)
    # Symbol+offset or Symbol-offset
    m = re.match(r"^(\w+)\s*([+-])\s*(\d+)$", expr)
    if m:
        sym, op, off = m.group(1), m.group(2), int(m.group(3))
        base = syms.get(sym)
        if base is None:
            raise ValueError(f"Symbol not found in map: {sym!r}")
        return base + off if op == "+" else base - off
    # Bare symbol
    if expr in syms:
        return syms[expr]
    raise ValueError(f"Cannot resolve address expression: {expr!r}")


def read_bytes(z: Zrcp, addr: int, count: int) -> list:
    """
    Read `count` bytes from emulator memory via ZRCP.
    ZRCP returns bytes as a single hex string (no spaces), e.g. "C3FF00".
    Returns list of ints.
    """
    resp = z.cmd(f"read-memory {addr} {count}")
    # Strip any whitespace/newlines
    hexstr = resp.strip()
    if not hexstr:
        raise ZrcpError(f"read-memory {addr} {count} returned empty response")
    # Parse as consecutive pairs of hex digits
    result = []
    for i in range(0, len(hexstr), 2):
        chunk = hexstr[i:i+2]
        if len(chunk) == 2:
            try:
                result.append(int(chunk, 16))
            except ValueError:
                break
    if not result:
        raise ZrcpError(f"read-memory {addr} {count}: could not parse response: {hexstr[:40]!r}")
    return result


def read_byte(z: Zrcp, addr: int) -> int:
    """Read a single byte from emulator memory via ZRCP."""
    vals = read_bytes(z, addr, 1)
    return vals[0]


def advance_frames(z: Zrcp, n: int):
    """Advance emulation by n frames using host-side sleep (50 Hz = 0.02s/frame)."""
    # ZEsarUX 12.1 does not have 'run-for' — use sleep instead.
    # Emulator is already running (started by runner.sh), no need to send 'run'.
    secs = n / 50.0
    time.sleep(secs)


def send_key(z: Zrcp, key: str, duration_ms: int = 100, syms: dict = None):
    """
    Inject a key action via the g_test_cmd mailbox in input.c.

    ZEsarUX ZRCP does not support injecting into the Z80 keyboard hardware port
    (0xFE), so send-keys-string / set-ui-io-ports do not work with in_key_pressed.
    Instead, input.c exposes g_test_cmd: writing an ASCII byte there causes
    input_poll to process one action on the next call, then clears it.

    duration_ms is used as a settle delay (in ms) after injection so the game
    loop has time to process the command before the caller checks state.
    """
    if syms is None:
        syms = {}
    ascii_char = KEY_MAP.get(key.upper(), KEY_MAP.get(key, key))
    if isinstance(ascii_char, str):
        ascii_char = ascii_char[0] if ascii_char else key[0]
    ascii_val = ord(ascii_char)

    cmd_addr = syms.get("_g_test_cmd")
    if cmd_addr is None:
        # Fallback: try legacy send-keys-string (may not work with in_key_pressed)
        z.cmd(f"send-keys-string {duration_ms} {ascii_char}")
        return

    # Write ASCII value to the command mailbox; input_poll clears it after acting.
    z.cmd(f"write-memory {cmd_addr} {ascii_val:02x}")
    # Wait for input_poll to run (at 50 Hz, ~20 ms per frame; use duration_ms as settle)
    time.sleep(max(duration_ms / 1000.0, 0.1))


def dump_board(z: Zrcp, syms: dict, out_name: str):
    """
    Read G.board[20][10] from emulator memory and write a text artifact.
    G.board starts at _G+0 (200 bytes of uint8_t, piece_id 0=empty, 1-7=piece).
    """
    g_addr = syms.get("_G")
    if g_addr is None:
        print("[dump_board] WARNING: _G symbol not in map, skipping dump", file=sys.stderr)
        return
    # board[20][10] = 200 bytes
    board_bytes = read_bytes(z, g_addr, 200)
    os.makedirs(ARTIFACTS_DIR, exist_ok=True)
    out_path = os.path.join(ARTIFACTS_DIR, f"{out_name}.txt")
    with open(out_path, "w") as f:
        f.write(f"board dump (G=0x{g_addr:04X})\n")
        for row in range(20):
            row_vals = [board_bytes[row * 10 + col] if (row * 10 + col) < len(board_bytes) else 0
                        for col in range(10)]
            f.write("".join(str(v) if v else "." for v in row_vals) + "\n")
    print(f"[dump_board] wrote {out_path}")


def run_scenario(z: Zrcp, syms: dict, scenario: dict) -> list:
    """
    Execute scenario steps. Returns list of result dicts.
    Each step: {"op": ..., ...}
    """
    results = []
    for i, step in enumerate(scenario.get("steps", [])):
        op = step["op"]
        try:
            if op == "frames":
                n = int(step["n"])
                advance_frames(z, n)
                results.append({"step": i, "op": op, "n": n, "status": "ok"})

            elif op == "key":
                key = step["key"]
                dur = int(step.get("duration_ms", 100))
                send_key(z, key, dur, syms)
                results.append({"step": i, "op": op, "key": key, "status": "ok"})

            elif op == "dump_board":
                out_name = step.get("out", f"board-step{i}")
                dump_board(z, syms, out_name)
                results.append({"step": i, "op": op, "out": out_name, "status": "ok"})

            elif op == "assert_eq":
                addr = resolve(step["mem"], syms)
                expected = int(step["val"])
                actual = read_byte(z, addr)
                ok = actual == expected
                ctx = step.get("ctx", "")
                if not ok:
                    msg = f"FAIL assert_eq: mem[0x{addr:04X}]={actual:#04x} != {expected:#04x}  ctx={ctx!r}"
                    results.append({"step": i, "op": op, "status": "FAIL", "msg": msg})
                    print(f"[FAIL] {msg}", file=sys.stderr)
                else:
                    results.append({"step": i, "op": op, "status": "ok",
                                    "addr": addr, "val": actual})

            elif op == "assert_nonzero":
                addr = resolve(step["mem"], syms)
                actual = read_byte(z, addr)
                ok = actual != 0
                ctx = step.get("ctx", "")
                if not ok:
                    msg = f"FAIL assert_nonzero: mem[0x{addr:04X}]=0x00  ctx={ctx!r}"
                    results.append({"step": i, "op": op, "status": "FAIL", "msg": msg})
                    print(f"[FAIL] {msg}", file=sys.stderr)
                else:
                    results.append({"step": i, "op": op, "status": "ok",
                                    "addr": addr, "val": actual})

            else:
                results.append({"step": i, "op": op, "status": "SKIP",
                                "msg": f"unknown op: {op!r}"})

        except Exception as e:
            results.append({"step": i, "op": op, "status": "ERROR", "msg": str(e)})
            print(f"[ERROR] step {i} ({op}): {e}", file=sys.stderr)

    return results


def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <scenario.json> [map_file]", file=sys.stderr)
        sys.exit(2)

    scenario_path = sys.argv[1]
    map_path = sys.argv[2] if len(sys.argv) > 2 else "/work/build/app.map"

    # Load scenario
    if not os.path.exists(scenario_path):
        print(f"Scenario not found: {scenario_path}", file=sys.stderr)
        sys.exit(2)
    with open(scenario_path) as f:
        scenario = json.load(f)

    # Load symbol map
    syms = parse_map(map_path)
    print(f"[map] loaded {len(syms)} symbols from {map_path}")
    if "_G" in syms:
        print(f"[map] _G = 0x{syms['_G']:04X} ({syms['_G']})")

    # Connect to ZRCP
    print(f"[zrcp] connecting to {ZRCP_HOST}:{ZRCP_PORT} ...")
    try:
        z = Zrcp(ZRCP_HOST, ZRCP_PORT)
    except ZrcpError as e:
        print(f"[zrcp] {e}", file=sys.stderr)
        sys.exit(3)

    print(f"[zrcp] connected; running scenario: {scenario.get('name', '?')}")
    print(f"[zrcp] description: {scenario.get('description', '')}")
    # No reset-cpu — the game is already running (ZEsarUX auto-boots 128K with tape).
    # runner.sh waits 1s after ZRCP becomes ready before calling us.

    try:
        results = run_scenario(z, syms, scenario)
    finally:
        z.close()

    # Write result artifact
    os.makedirs(ARTIFACTS_DIR, exist_ok=True)
    result_name = scenario.get("name", "scenario")
    out_path = os.path.join(ARTIFACTS_DIR, f"integration-{result_name}.json")
    failed = [r for r in results if r["status"] not in ("ok", "SKIP")]
    report = {
        "scenario": scenario.get("name"),
        "description": scenario.get("description", ""),
        "results": results,
        "passed": (len(failed) == 0),
    }
    with open(out_path, "w") as f:
        json.dump(report, f, indent=2)
    print(f"[result] wrote {out_path}")

    if failed:
        print(f"[FAIL] {len(failed)} step(s) failed or errored", file=sys.stderr)
        sys.exit(1)

    print(f"[OK] scenario {scenario.get('name')!r} passed")
    sys.exit(0)


if __name__ == "__main__":
    main()

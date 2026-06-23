import socket, time, re

def readall(sock, timeout=3.0):
    data = b""
    sock.settimeout(timeout)
    try:
        while True:
            chunk = sock.recv(4096)
            if not chunk:
                break
            data += chunk
            if b"command> " in data:
                break
    except socket.timeout:
        pass
    sock.settimeout(10)
    return data.decode("latin-1", errors="replace")

s = socket.socket()
s.settimeout(10)
s.connect(("127.0.0.1", 10000))
readall(s)

def cmd(line, wait=3.0):
    s.sendall((line + "\n").encode("latin-1"))
    return readall(s, wait)

def read_byte(addr):
    r = cmd(f"read-memory {addr} 1")
    h = r.strip()
    return int(h[:2], 16) if len(h) >= 2 else -1

def write_byte(addr, val):
    return cmd(f"write-memory {addr} {val:02x}")

# Load map
import sys
with open("/work/build/app.map") as f:
    content = f.read()
pat = re.compile(r"^(\S+)\s+=\s+\$([0-9A-Fa-f]+)", re.M)
syms = {m.group(1): int(m.group(2), 16) for m in pat.finditer(content)}

g_addr = syms["_G"]
g_test_cmd_addr = syms["_g_test_cmd"]
cursor_col_addr = g_addr + 3
cursor_row_addr = g_addr + 4
game_move_cursor_addr = syms.get("_game_move_cursor", 0)

print(f"_G=0x{g_addr:04X} g_test_cmd=0x{g_test_cmd_addr:04X}")
print(f"game_move_cursor=0x{game_move_cursor_addr:04X}")

# Read initial state
cc = read_byte(cursor_col_addr)
cr = read_byte(cursor_row_addr)
tc = read_byte(g_test_cmd_addr)
frame_lo = read_byte(g_addr)
frame_hi = read_byte(g_addr + 1)
print(f"Initial: frame={frame_lo|(frame_hi<<8)} cursor_col={cc} cursor_row={cr} g_test_cmd={tc}")

# Use set-membreakpoint to trap writes to cursor_col
print(f"\nSetting memory write breakpoint at cursor_col (0x{cursor_col_addr:04X})...")
# ZEsarUX set-membreakpoint syntax: set-membreakpoint address type
# type: 0=none, 1=read, 2=write, 3=read+write, 4=execute
bp_resp = cmd(f"set-membreakpoint {cursor_col_addr} 2")
print(f"membreakpoint response: {bp_resp.strip()[:200]}")

# Check breakpoints
print("\nCurrent memory breakpoints:")
bps = cmd("get-membreakpoints")
print(bps[:500])

# Write P to g_test_cmd
print(f"\nWriting 'P' (0x50) to g_test_cmd at 0x{g_test_cmd_addr:04X}")
write_byte(g_test_cmd_addr, 0x50)

# Wait to see if breakpoint triggers
time.sleep(1.0)

# Check if paused
regs = cmd("get-registers")
print(f"\nRegisters after 1s: {regs[:200]}")

# Read cursor state
cc2 = read_byte(cursor_col_addr)
cr2 = read_byte(cursor_row_addr)
tc2 = read_byte(g_test_cmd_addr)
print(f"After 1s: cursor_col={cc2} cursor_row={cr2} g_test_cmd={tc2}")

# Try running if paused
run_resp = cmd("run")
print(f"run response: {run_resp[:100]}")

time.sleep(0.5)
cc3 = read_byte(cursor_col_addr)
cr3 = read_byte(cursor_row_addr)
tc3 = read_byte(g_test_cmd_addr)
print(f"After run: cursor_col={cc3} cursor_row={cr3} g_test_cmd={tc3}")

# Clear breakpoint
print("\nClearing memory breakpoint...")
cmd(f"set-membreakpoint {cursor_col_addr} 0")

s.close()

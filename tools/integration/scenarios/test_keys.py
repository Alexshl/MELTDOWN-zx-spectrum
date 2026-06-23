import socket, time, re

def readall(sock, timeout=2.0):
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

def cmd(line, wait=2.0):
    s.sendall((line + "\n").encode("latin-1"))
    return readall(s, wait)

with open("/work/build/app.map") as f:
    content = f.read()
pat = re.compile(r"^(\S+)\s+=\s+\$([0-9A-Fa-f]+)", re.M)
syms = {m.group(1): int(m.group(2), 16) for m in pat.finditer(content)}
g_addr = syms["_G"]
g_test_cmd_addr = syms["_g_test_cmd"]
cursor_col_addr = g_addr + 3
cursor_row_addr = g_addr + 4

# Also look up game_move_cursor and game_init addresses for sanity check
game_move_cursor_addr = syms.get("_game_move_cursor", 0)
game_init_addr = syms.get("_game_init", 0)
input_poll_addr = syms.get("_input_poll", 0)
print(f"_G=0x{g_addr:04X} g_test_cmd=0x{g_test_cmd_addr:04X}")
print(f"cursor_col=0x{cursor_col_addr:04X} cursor_row=0x{cursor_row_addr:04X}")
print(f"game_move_cursor=0x{game_move_cursor_addr:04X} game_init=0x{game_init_addr:04X} input_poll=0x{input_poll_addr:04X}")

def read_byte(addr):
    r = cmd(f"read-memory {addr} 1")
    h = r.strip()
    return int(h[:2], 16) if len(h) >= 2 else -1

def write_byte(addr, val):
    return cmd(f"write-memory {addr} {val:02x}")

# Read initial state - dump full G struct
def read_range(addr, n):
    results = []
    for i in range(n):
        results.append(read_byte(addr + i))
    return results

print("Initial G struct (first 15 bytes):")
g_bytes = read_range(g_addr, 15)
print(f"  _G bytes: {' '.join(f'{b:02X}' for b in g_bytes)}")
if len(g_bytes) >= 9:
    print(f"  frame={g_bytes[0] | (g_bytes[1]<<8)} phase={g_bytes[2]} cursor_col={g_bytes[3]} cursor_row={g_bytes[4]}")
    print(f"  gold={g_bytes[5] | (g_bytes[6]<<8)} sel_turret={g_bytes[7]} towers_count={g_bytes[8]}")

# Verify code bytes at game_move_cursor write-back (0x6DFA)
write_back_bytes = read_range(0x6DFA, 10)
print(f"Code at 0x6DFA (write-back): {' '.join(f'{b:02X}' for b in write_back_bytes)}")
# Expected: 21 33 97 EB CD xx xx 7D 12 E1 (LD HL,$9733; EX DE,HL; CALL ...; LD A,L; LD (DE),A; POP HL)

# Verify code bytes at game_move_cursor entry (0x6D96)
entry_bytes = read_range(game_move_cursor_addr, 10)
print(f"Code at game_move_cursor ({game_move_cursor_addr:#x}): {' '.join(f'{b:02X}' for b in entry_bytes)}")
# Expected: 2A 33 97 26 00 EB CD D3 6B 19

cc = read_byte(cursor_col_addr)
cr = read_byte(cursor_row_addr)
tc = read_byte(g_test_cmd_addr)
print(f"Initial: cursor_col={cc} cursor_row={cr} g_test_cmd={tc}")

# Set up a ZRCP breakpoint at the write-back instruction in game_move_cursor
# 0x6E02 is LD (DE),A which writes nc to cursor_col
write_back_addr = 0x6E02
print(f"\nSetting breakpoint at write-back (0x{write_back_addr:04X})...")
bp_resp = cmd(f"enable-breakpoint {write_back_addr:04X}")
print(f"  breakpoint response: {bp_resp.strip()[:100]}")

# Write 'P' to g_test_cmd, then poll cursor_col AND cursor_row
write_byte(g_test_cmd_addr, 0x50)  # 'P'
print("Wrote 0x50 to g_test_cmd (P=move right), polling cursor_col and cursor_row...")

samples = []
for i in range(30):
    tc = read_byte(g_test_cmd_addr)
    cc = read_byte(cursor_col_addr)
    cr = read_byte(cursor_row_addr)
    samples.append((tc, cc, cr))
    if cc != 2 or cr != 9:
        break
    time.sleep(0.05)

print(f"Samples (g_test_cmd, cursor_col, cursor_row): {samples}")

# Final state
tc_final = read_byte(g_test_cmd_addr)
cc_final = read_byte(cursor_col_addr)
cr_final = read_byte(cursor_row_addr)
print(f"Final: g_test_cmd={tc_final} cursor_col={cc_final} cursor_row={cr_final}")

# Now test 'Q' (move up) to check cursor_row
print("\nResetting and testing Q (move up)...")
# first reset to known position
write_byte(cursor_col_addr, 2)
write_byte(cursor_row_addr, 9)
time.sleep(0.05)
print(f"Reset to cursor_col=2, cursor_row=9")
write_byte(g_test_cmd_addr, 0x51)  # 'Q'
for i in range(20):
    tc = read_byte(g_test_cmd_addr)
    cc = read_byte(cursor_col_addr)
    cr = read_byte(cursor_row_addr)
    print(f"  sample {i}: g_test_cmd={tc} cursor_col={cc} cursor_row={cr}")
    if tc == 0:
        break
    time.sleep(0.05)
time.sleep(0.1)
cc_final2 = read_byte(cursor_col_addr)
cr_final2 = read_byte(cursor_row_addr)
print(f"After Q: cursor_col={cc_final2} cursor_row={cr_final2} (expect col=2 row=8)")

# Now try direct write to cursor_col to see if it sticks
write_byte(cursor_col_addr, 5)
time.sleep(0.1)
cc_after_direct = read_byte(cursor_col_addr)
print(f"\nAfter direct write cursor_col=5: {cc_after_direct}")

time.sleep(0.5)
cc_after_wait = read_byte(cursor_col_addr)
print(f"After 0.5s: cursor_col={cc_after_wait}")

s.close()

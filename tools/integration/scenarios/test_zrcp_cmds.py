import socket, time

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

def cmd(line, wait=3.0):
    s.sendall((line + "\n").encode("latin-1"))
    return readall(s, wait)

# List available commands
print("=== help output (first 100 lines) ===")
r = cmd("help")
lines = r.split('\n')
for l in lines[:100]:
    if l.strip():
        print(l)

s.close()

#!/usr/bin/env bash
set -euo pipefail

FILE="${1:-${FILE:-}}"
Q="${2:-${Q:-}}"
ORG="${ORG:-0x6000}"

[[ -n "$FILE" ]] || { echo "usage: investigate.sh <file> <question>" >&2; exit 2; }
[[ -n "$Q" ]] || { echo "question required" >&2; exit 2; }
[[ -f "$FILE" ]] || { echo "file not found: $FILE" >&2; exit 2; }

slug=$(echo "$Q" | tr A-Z a-z | sed 's/[^a-z0-9]/-/g; s/--*/-/g; s/^-//; s/-$//' | cut -c1-40)
ts=$(date +%Y%m%d-%H%M%S)
DIR="/work/artifacts/investigations/${ts}-${slug}"
mkdir -p "$DIR"

cp "$FILE" "$DIR/original.bin"

file "$FILE" > "$DIR/file.txt" 2>&1
# head -200 causes SIGPIPE to hexdump; pipefail would abort, so disable temporarily
{ hexdump -C "$FILE" | head -200 > "$DIR/hexdump.txt"; } || true
strings -n 4 "$FILE" > "$DIR/strings.txt"

# Find corresponding *_CODE.bin next to the .tap (same directory as the input file)
BIN=""
BASENAME=$(basename "$FILE" .tap)
CANDIDATE_DIR=$(dirname "$FILE")
if [[ -f "$CANDIDATE_DIR/${BASENAME}_CODE.bin" ]]; then
    BIN="$CANDIDATE_DIR/${BASENAME}_CODE.bin"
    cp "$BIN" "$DIR/code.bin"
elif [[ "$FILE" == *.bin ]]; then
    BIN="$FILE"
fi

if [[ -n "$BIN" ]]; then
    # Use .map if available alongside the binary
    MAP="$CANDIDATE_DIR/${BASENAME}.map"
    if [[ -f "$MAP" ]]; then
        cp "$MAP" "$DIR/symbols.map"
        z88dk-dis -mz80 -o "${ORG#0x}" -x "$MAP" "$BIN" > "$DIR/z88dk-dis.asm" 2>&1 || echo "z88dk-dis failed" > "$DIR/z88dk-dis.err"
    else
        z88dk-dis -mz80 -o "${ORG#0x}" "$BIN" > "$DIR/z88dk-dis.asm" 2>&1 || echo "z88dk-dis failed" > "$DIR/z88dk-dis.err"
    fi
    z80dasm --labels --origin="$ORG" --output="$DIR/z80dasm.asm" "$BIN" 2>"$DIR/z80dasm.err" || true
else
    echo "WARN: no raw .bin found near $FILE (need _CODE.bin or .bin input); disasm skipped" > "$DIR/disasm.warn"
fi

# prompt.md for the investigator agent
cat > "$DIR/prompt.md" <<EOF
# Investigation request

**File**: $FILE
**Question**: $Q
**ORG**: $ORG

## Recon artifacts (this directory)
- file.txt — \`file\` output
- hexdump.txt — first 200 lines hex
- strings.txt — strings -n 4
- z88dk-dis.asm / z80dasm.asm — disassembly (if binary was found)
- symbols.map — symbols (if available)
- original.bin — copy of the input file
- code.bin — extracted raw binary (if present)

## Instructions to agent
1. Read these artifacts.
2. Read .claude/skills/zx-arch/SKILL.md before drawing conclusions about ports, banks, or screen layout.
3. Answer the user's question and write report.md into this directory.
4. Honest split: Verified / Hypothesis.
EOF

echo "$DIR"
echo "Run: Task investigator <$DIR/prompt.md>"

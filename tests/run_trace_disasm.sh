#!/usr/bin/env bash
set -euo pipefail

bin="$1"
prog="$2"

if [[ ! -x "$bin" ]]; then
  echo "Binary not found or not executable: $bin"
  exit 1
fi

output="$(${bin} --trace --disasm "$prog")"

echo "$output" | grep -q "^== script ==" || {
  echo "disasm output missing header"
  exit 1
}

# Trace output prints "0000 OP_*" (no line/col). Disasm includes line/col.
echo "$output" | grep -E -q "^[0-9]{4} OP_" || {
  echo "trace output missing opcode lines"
  exit 1
}

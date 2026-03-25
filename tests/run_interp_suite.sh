#!/usr/bin/env bash
set -euo pipefail

bin="$1"
shift

if [[ ! -x "$bin" ]]; then
  echo "Binary not found or not executable: $bin"
  exit 1
fi

for file in "$@"; do
  "$bin" --interp "$file" >/dev/null
done

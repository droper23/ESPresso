#!/usr/bin/env bash
set -euo pipefail

bin="$1"
file="$2"

if [[ ! -x "$bin" ]]; then
  echo "Binary not found or not executable: $bin"
  exit 1
fi

if [[ "$(uname -s)" != "Darwin" ]]; then
  echo "SKIP: leaks is macOS-only"
  exit 0
fi

# leaks requires elevated permissions for target process access
if [[ "${EUID:-$(id -u)}" -ne 0 ]]; then
  echo "SKIP: leaks requires sudo"
  exit 0
fi

leaks --atExit -- "$bin" --interp "$file"

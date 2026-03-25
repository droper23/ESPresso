#!/usr/bin/env bash
set -euo pipefail

bin="$1"
file="$2"

if [[ ! -x "$bin" ]]; then
  echo "Binary not found or not executable: $bin"
  exit 1
fi

if ! command -v valgrind >/dev/null 2>&1; then
  echo "SKIP: valgrind not found"
  exit 0
fi

valgrind --leak-check=full --error-exitcode=1 --quiet \
  "$bin" --interp "$file"

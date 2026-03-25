#!/usr/bin/env bash
set -euo pipefail

bin="$1"
file="$2"

if [[ ! -x "$bin" ]]; then
  echo "Binary not found or not executable: $bin"
  exit 1
fi

base_opts="halt_on_error=1:allocator_may_return_null=0"
if [[ "$(uname -s)" == "Darwin" ]]; then
  # LeakSanitizer is not supported on macOS; disable leak detection.
  asan_opts="detect_leaks=0:${base_opts}"
else
  asan_opts="detect_leaks=1:${base_opts}"
fi

ASAN_OPTIONS=${ASAN_OPTIONS:-"${asan_opts}"} \
  "$bin" --interp "$file"

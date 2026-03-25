#!/usr/bin/env bash
set -euo pipefail

bin="$1"
shift

if [[ ! -x "$bin" ]]; then
  echo "Binary not found or not executable: $bin"
  exit 1
fi

run_case() {
  local mode="$1"
  local file="$2"
  local cmd=("$bin")
  if [[ "$mode" == "interp" ]]; then
    cmd+=(--interp)
  fi
  cmd+=("$file")

  set +e
  "${cmd[@]}" >/dev/null 2>&1
  local status=$?
  set -e

  # Fail only on crash-like statuses (signals).
  if [[ $status -ge 128 ]]; then
    echo "Crash detected (status=$status) for $mode: $file"
    exit 1
  fi
}

for file in "$@"; do
  run_case interp "$file"
  run_case vm "$file"
done

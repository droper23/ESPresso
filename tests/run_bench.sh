#!/usr/bin/env bash
set -euo pipefail

bin="${1:-./build/ESPresso}"
prog="${2:-examples/bench.espr}"
reps="${3:-20}"

if [[ ! -x "$bin" ]]; then
  echo "Binary not found or not executable: $bin"
  exit 1
fi
if [[ ! -f "$prog" ]]; then
  echo "Program not found: $prog"
  exit 1
fi

run_many() {
  local mode="$1"
  local out
  if [[ "$mode" == "interp" ]]; then
    out=$(/usr/bin/time -p bash -c "for i in \$(seq 1 $reps); do \"$bin\" --interp \"$prog\" >/dev/null; done" 2>&1)
  else
    out=$(/usr/bin/time -p bash -c "for i in \$(seq 1 $reps); do \"$bin\" \"$prog\" >/dev/null; done" 2>&1)
  fi
  echo "$out" | awk '/^real / {print $2}'
}

echo "Interpreter ($reps runs):" >&2
interp_real=$(run_many interp)

echo "VM ($reps runs):" >&2
vm_real=$(run_many vm)

printf "\nResults (real seconds for %s runs):\n" "$reps"
printf "  interp: %s\n" "${interp_real:-<missing>}"
printf "  vm:     %s\n" "${vm_real:-<missing>}"

if [[ -n "$interp_real" && -n "$vm_real" ]]; then
  speedup=$(awk -v i="$interp_real" -v v="$vm_real" 'BEGIN { if (v==0) { print "inf" } else { printf "%.2f", i/v } }')
  printf "  speedup: %sx\n" "$speedup"
fi

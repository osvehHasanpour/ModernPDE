#!/usr/bin/env bash
set -euo pipefail

RESULTS="${RESULTS_DIR:-/app/output}"
mkdir -p "$RESULTS"

CSV="$RESULTS/docker_benchmark.csv"
echo "file,binary,runtime_ms,exit_code" > "$CSV"

run_one() {
  local bin="$1"
  local file="$2"
  local name
  name="$(basename "$file")"

  local start end ms code
  start=$(date +%s%N)
  set +e
  "$bin" "$file" > /tmp/modernpde_out.txt 2>&1
  code=$?
  set -e
  end=$(date +%s%N)
  ms=$(( (end - start) / 1000000 ))

  echo "$name,$bin,$ms,$code" >> "$CSV"
  echo "[OK] $bin $name -> ${ms} ms (exit $code)"
}

echo "=== ModernPDE baseline samples ==="
for f in \
  /app/tests/samples/cfg.cpp \
  /app/tests/samples/dead.cpp \
  /app/tests/samples/oop.cpp \
  /app/tests/samples/partial.cpp \
  /app/tests/samples/recursion.cpp \
  /app/tests/samples/cfg_while.cpp \
  /app/tests/samples/cfg_for.cpp
do
  if [[ -f "$f" ]]; then
    run_one ModernPDE "$f"
  fi
done

echo "=== ModernPDE_Phase8 ==="
start=$(date +%s%N)
set +e
ModernPDE_Phase8 "$RESULTS/phase8_benchmark.txt" > /tmp/phase8_out.txt 2>&1
code=$?
set -e
end=$(date +%s%N)
ms=$(( (end - start) / 1000000 ))
echo "(internal sample CFG),ModernPDE_Phase8,$ms,$code" >> "$CSV"
echo "[OK] ModernPDE_Phase8 -> ${ms} ms (exit $code)"

echo ""
echo "Saved: $CSV"
cat "$CSV"

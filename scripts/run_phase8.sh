#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

RUN_ID=$(date +%Y-%m-%d_%H-%M-%S)
OUT="output/runs/${RUN_ID}_phase8"
mkdir -p "$OUT" output/benchmarks output/reports

echo "=============================================="
echo " ModernPDE Phase8 Run: ${RUN_ID}"
echo "=============================================="

if [ ! -f ./build/ModernPDE_Phase8 ]; then
    echo "Error: ./build/ModernPDE_Phase8 not found. Build first."
    exit 1
fi

./build/ModernPDE_Phase8 "$OUT/phase8_benchmark.txt" | tee "$OUT/full_output.txt"

python3 - << PY
import json, re, pathlib, datetime, platform
out = pathlib.Path("$OUT")
text = (out / "full_output.txt").read_text(encoding="utf-8", errors="replace")

def find(pat, default=None):
    m = re.search(pat, text)
    return m.group(1) if m else default

def find_float(pat, default=0.0):
    v = find(pat)
    try:
        return float(v) if v is not None else default
    except ValueError:
        return default

def find_int(pat, default=0):
    v = find(pat)
    try:
        return int(v) if v is not None else default
    except ValueError:
        return default

data = {
    "run_id": "$RUN_ID",
    "timestamp": datetime.datetime.now().isoformat(),
    "binary": "ModernPDE_Phase8",
    "host": platform.node(),
    "cfg_cache": {
        "hits": find_int(r"Hits:\s+(\d+)"),
        "misses": find_int(r"Misses:\s+(\d+)"),
        "hit_rate": find_float(r"Hit Rate:\s+([0-9.]+)"),
        "traversals": find_int(r"Traversals:\s+(\d+)"),
        "nodes_visited": find_int(r"Nodes Visited:\s+(\d+)"),
    },
    "du": {
        "chains": find_int(r"DU chains built:\s+(\d+)"),
        "chains_analyzed": find_int(r"Chains analyzed:\s+(\d+)"),
        "reaching_defs": find_int(r"Reaching defs computed:\s+(\d+)"),
    },
    "benchmark": {
        "runtime_ms": find_int(r"Runtime:\s+(\d+)"),
        "cache_hits": find_int(r"Cache Hits:\s+(\d+)"),
        "cache_misses": find_int(r"Cache Misses:\s+(\d+)"),
        "cache_hit_rate": find_float(r"Cache Hit Rate:\s+([0-9.]+)"),
    },
    "cfg_blocks": find_int(r"CFG created:\s*(\d+)\s*blocks"),
}
(out / "phase8_summary.json").write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
print("Wrote phase8_summary.json")
PY

rm -rf output/latest
ln -sfn "runs/${RUN_ID}_phase8" output/latest
cp -f "$OUT/phase8_summary.json" output/reports/latest_phase8_summary.json

echo ""
echo "Phase8 results saved to: $OUT"
echo "  - phase8_benchmark.txt"
echo "  - phase8_summary.json"
echo "=============================================="

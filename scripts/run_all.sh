#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

RUN_ID=$(date +%Y-%m-%d_%H-%M-%S)
OUT="output/runs/${RUN_ID}"
mkdir -p "$OUT" output/benchmarks output/reports output/archive

echo "=============================================="
echo " ModernPDE Run: ${RUN_ID}"
echo "=============================================="

if [ ! -f ./build/ModernPDE ]; then
    echo "Error: ./build/ModernPDE not found. Build first."
    exit 1
fi

./build/ModernPDE | tee "$OUT/full_output.txt"

grep "Variables:" "$OUT/full_output.txt" > "$OUT/benchmark.txt" || true
grep -E "Accuracy|Precision|Recall|F1" "$OUT/full_output.txt" > "$OUT/metrics.txt" || true
grep "Classification" "$OUT/full_output.txt" > "$OUT/pde_result.txt" || true
grep "Block" "$OUT/full_output.txt" > "$OUT/cfg_result.txt" || true
grep "Def(" "$OUT/full_output.txt" > "$OUT/duchain_result.txt" || true
grep "Path " "$OUT/full_output.txt" > "$OUT/path_result.txt" || true

python3 - << PY
import json, re, pathlib, collections, datetime, platform
out = pathlib.Path("$OUT")
text = (out / "full_output.txt").read_text(encoding="utf-8", errors="replace")

def find(pat, default=None):
    m = re.search(pat, text)
    return m.group(1) if m else default

metrics = {
    "run_id": "$RUN_ID",
    "accuracy": float(find(r"Accuracy\s*:\s*([0-9.]+)", "0") or 0),
    "precision": float(find(r"Precision\s*:\s*([0-9.]+)", "0") or 0),
    "recall": float(find(r"Recall\s*:\s*([0-9.]+)", "0") or 0),
    "f1": float(find(r"F1 Score\s*:\s*([0-9.]+)", "0") or 0),
}
(out / "metrics.json").write_text(json.dumps(metrics, indent=2) + "\n", encoding="utf-8")
print("Wrote metrics.json")

counts = collections.Counter()
for m in re.finditer(r"Classification\s*:\s*(DEAD|MOSTLY DEAD|PARTIALLY DEAD|MOSTLY LIVE|LIVE)\b", text):
    counts[m.group(1).strip()] += 1

summary = {
    "run_id": "$RUN_ID",
    "total": sum(counts.values()),
    "dead": counts.get("DEAD", 0),
    "mostly_dead": counts.get("MOSTLY DEAD", 0),
    "partially_dead": counts.get("PARTIALLY DEAD", 0),
    "mostly_live": counts.get("MOSTLY LIVE", 0),
    "live": counts.get("LIVE", 0),
    "raw_counts": dict(counts),
}
(out / "pde_summary.json").write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
print("Wrote pde_summary.json")

meta = {
    "run_id": "$RUN_ID",
    "timestamp": datetime.datetime.now().isoformat(),
    "host": platform.node(),
    "binary": "ModernPDE",
    "platform": platform.platform(),
}
(out / "run_meta.json").write_text(json.dumps(meta, indent=2) + "\n", encoding="utf-8")
print("Wrote run_meta.json")
PY

rm -rf output/latest
ln -sfn "runs/${RUN_ID}" output/latest
cp -f "$OUT/metrics.json" output/reports/latest_metrics.json
cp -f "$OUT/pde_summary.json" output/reports/latest_pde_summary.json

echo ""
echo "Results saved to: $OUT"
echo "  - metrics.json / pde_summary.json / run_meta.json"
echo "Latest: output/latest -> runs/${RUN_ID}"
echo "=============================================="

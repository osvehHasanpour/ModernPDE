#!/usr/bin/env python3
from pathlib import Path
import subprocess, time, re
import pandas as pd

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / "build"
TEST_DIR = ROOT / "tests" / "samples"
RESULTS_DIR = ROOT / "output" / "benchmarks"
RESULTS_DIR.mkdir(parents=True, exist_ok=True)

MODERNPDE = BUILD / "ModernPDE"
PHASE8 = BUILD / "ModernPDE_Phase8"

INPUTS = sorted(TEST_DIR.glob("*.cpp"))

def find(out, pattern):
    m = re.search(pattern, out)
    return m.group(1) if m else ""

def run_and_measure(cmd):
    start = time.perf_counter()
    proc = subprocess.run(cmd, capture_output=True, text=True)
    runtime_ms = (time.perf_counter() - start) * 1000.0
    return runtime_ms, proc.stdout, proc.stderr, proc.returncode

results = []

print("=== ModernPDE (baseline / phases 1-7) ===")
for src in INPUTS:
    print(f"  {src.name}")
    rt, out, err, code = run_and_measure([str(MODERNPDE), str(src)])
    results.append({
        "file": src.name,
        "binary": "ModernPDE",
        "runtime_ms": round(rt, 2),
        "exit_code": code,
        "functions": find(out, r"Functions detected\s*:\s*(\d+)"),
        "blocks": find(out, r"CFG Blocks\s*:\s*(\d+)"),
        "accuracy": find(out, r"Accuracy\s*:\s*([0-9.]+)"),
        "precision": find(out, r"Precision\s*:\s*([0-9.]+)"),
        "recall": find(out, r"Recall\s*:\s*([0-9.]+)"),
        "f1": find(out, r"F1 Score\s*:\s*([0-9.]+)"),
        "cache_hits": "",
        "cache_misses": "",
        "hit_rate": "",
    })

print("=== ModernPDE_Phase8 ===")
report = RESULTS_DIR / "phase8_benchmark.txt"
rt, out, err, code = run_and_measure([str(PHASE8), str(report)])
results.append({
    "file": "(internal sample CFG)",
    "binary": "ModernPDE_Phase8",
    "runtime_ms": round(rt, 2),
    "exit_code": code,
    "functions": "",
    "blocks": find(out, r"CFG created:\s*(\d+)\s*blocks"),
    "accuracy": "",
    "precision": "",
    "recall": "",
    "f1": "",
    "cache_hits": find(out, r"Hits:\s+(\d+)"),
    "cache_misses": find(out, r"Misses:\s+(\d+)"),
    "hit_rate": find(out, r"Hit Rate:\s+([0-9.]+)"),
})

df = pd.DataFrame(results)
df.to_csv(RESULTS_DIR / "benchmark.csv", index=False)

cols = list(df.columns)
lines = [
    "| " + " | ".join(cols) + " |",
    "| " + " | ".join(["---"] * len(cols)) + " |",
]
for _, row in df.iterrows():
    lines.append("| " + " | ".join(str(row[c]) for c in cols) + " |")
(RESULTS_DIR / "summary.md").write_text("\n".join(lines) + "\n", encoding="utf-8")

print("\n" + df.to_string(index=False))
print(f"\nSaved: {RESULTS_DIR / 'benchmark.csv'}")
print(f"Saved: {RESULTS_DIR / 'summary.md'}")

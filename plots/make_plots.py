"""
Generate publication-quality plots from ModernPDE benchmark/results data.

Inputs (expected in the same directory):
    - benchmark.csv            (results/benchmarks/benchmark.csv)
    - latest_pde_summary.json  (results/reports/latest_pde_summary.json)

Outputs:
    - runtime_per_testcase.png / .pdf
    - pde_classification.png  / .pdf
"""

import json
import pandas as pd
import matplotlib.pyplot as plt

# ---- journal-friendly style ----
plt.rcParams.update({
    "font.family": "serif",
    "font.size": 11,
    "axes.spines.top": False,
    "axes.spines.right": False,
    "axes.grid": True,
    "grid.alpha": 0.3,
    "grid.linewidth": 0.5,
    "figure.dpi": 300,
})

# =========================================================
# Plot 1: Runtime per test case (bar chart)
# =========================================================
df = pd.read_csv("benchmark.csv")

# drop the synthetic Phase8 cache-demo row; keep the real CFG test files
df = df[df["binary"] == "ModernPDE"].copy()
df = df.sort_values("runtime_ms", ascending=True)

fig, ax = plt.subplots(figsize=(8, 6))
bars = ax.barh(df["file"], df["runtime_ms"], color="#3b6ea5", edgecolor="black", linewidth=0.4)

ax.set_xlabel("Runtime (ms)")
ax.set_ylabel("Test case")
ax.set_title("ModernPDE: Analysis Runtime per Test Case", fontweight="bold")

for bar, val in zip(bars, df["runtime_ms"]):
    ax.text(val + 1, bar.get_y() + bar.get_height() / 2, f"{val:.1f}",
            va="center", fontsize=8)

fig.tight_layout()
fig.savefig("runtime_per_testcase.png", bbox_inches="tight")
fig.savefig("runtime_per_testcase.pdf", bbox_inches="tight")
plt.close(fig)

# =========================================================
# Plot 2: Partial Dead-code Elimination (PDE) classification
#          distribution (stacked/bar chart)
# =========================================================
with open("latest_pde_summary.json") as f:
    pde = json.load(f)

labels = ["DEAD", "MOSTLY DEAD", "PARTIALLY DEAD", "MOSTLY LIVE", "LIVE"]
counts = [pde["raw_counts"][k] for k in labels]
total = pde["total"]
pct = [100 * c / total for c in counts]

colors = ["#b23a48", "#d98c5f", "#e8c46a", "#8fb996", "#3b6ea5"]

fig, ax = plt.subplots(figsize=(8, 5))
bars = ax.bar(labels, counts, color=colors, edgecolor="black", linewidth=0.5)
ax.set_ylim(0, max(counts) * 1.18)

ax.set_ylabel("Number of variables")
ax.set_title(f"PDE Classification Distribution (n = {total})", fontweight="bold")
ax.set_xticks(range(len(labels)))
ax.set_xticklabels(labels, rotation=20, ha="right")

for bar, c, p in zip(bars, counts, pct):
    ax.text(bar.get_x() + bar.get_width() / 2, c + total * 0.01,
            f"{c}\n({p:.1f}%)", ha="center", va="bottom", fontsize=8)

fig.tight_layout()
fig.savefig("pde_classification.png", bbox_inches="tight")
fig.savefig("pde_classification.pdf", bbox_inches="tight")
plt.close(fig)

print("Saved: runtime_per_testcase.{png,pdf}, pde_classification.{png,pdf}")

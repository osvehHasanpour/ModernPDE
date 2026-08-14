#!/usr/bin/env python3
"""
Compare ModernPDE (Artifact A) against three reference papers.

Outputs
-------
output/paper-comparison/*.csv   structured extracts + comparison
output/paper-comparison/*.md    human-readable summary
output/plots/*.png              one chart per metric category

Run from the project root (WSL):
    .venv/bin/python scripts/export_paper_comparison.py
"""

from __future__ import annotations

import csv
import re
import subprocess
import sys
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DOCS = ROOT / "docs" / "papers"
BENCH = ROOT / "output" / "paper-comparison"
PLOTS = ROOT / "output" / "plots"
SKIP_DIRS = {
    ".git",
    ".venv",
    "venv",
    "build",
    "build-baseline",
    "build-verify",
    "build-benchmark",
    "output",
    "plots",
    "__pycache__",
    "CMakeFiles",
}

PDF_MUZEEL = DOCS / "muzeel.pdf"
PDF_DIE = DOCS / "die.pdf"
PDF_AUTOJMH = DOCS / "autojmh.pdf"

# Decision keywords used as a McCabe proxy (each adds a branch).
DECISION_RE = re.compile(
    r"\b(if|else\s+if|for|while|case|catch|&&|\|\||\?)\b"
)


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def ensure_dirs() -> None:
    BENCH.mkdir(parents=True, exist_ok=True)
    PLOTS.mkdir(parents=True, exist_ok=True)


def should_skip(path: Path) -> bool:
    return any(part in SKIP_DIRS for part in path.parts)


def write_csv(path: Path, fieldnames: list[str], rows: list[dict]) -> None:
    with path.open("w", encoding="utf-8", newline="") as fh:
        writer = csv.DictWriter(fh, fieldnames=fieldnames, extrasaction="ignore")
        writer.writeheader()
        for row in rows:
            writer.writerow({k: row.get(k, "N/A") for k in fieldnames})


def na(value) -> str:
    if value is None or value == "":
        return "N/A"
    return str(value)


# ---------------------------------------------------------------------------
# Code metrics (Artifact A)
# ---------------------------------------------------------------------------

def count_loc(path: Path) -> tuple[int, int, int]:
    """Return (code, comment, blank) physical lines."""
    code = comment = blank = 0
    in_block = False
    try:
        text = path.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return 0, 0, 0
    for raw in text.splitlines():
        line = raw.strip()
        if not line:
            blank += 1
            continue
        if in_block:
            comment += 1
            if "*/" in line:
                in_block = False
            continue
        if line.startswith("//"):
            comment += 1
            continue
        if line.startswith("/*"):
            comment += 1
            if "*/" not in line:
                in_block = True
            continue
        code += 1
    return code, comment, blank


def cyclomatic_proxy(text: str) -> int:
    """1 + decision-point count. Not a full AST McCabe, but consistent."""
    return 1 + len(DECISION_RE.findall(text))


def collect_code_metrics() -> dict:
    loc_by_dir: dict[str, int] = defaultdict(int)
    files_by_ext: dict[str, int] = defaultdict(int)
    file_complexity: list[tuple[str, int, int]] = []
    include_edges = 0
    cpp_files = header_files = py_files = 0
    total_code = total_comment = total_blank = 0

    for path in ROOT.rglob("*"):
        if not path.is_file() or should_skip(path):
            continue
        ext = path.suffix.lower()
        if ext not in {".cpp", ".h", ".c", ".hpp", ".py", ".cmake", ".yml", ".sh"} and path.name != "CMakeLists.txt":
            continue
        files_by_ext[ext or path.name] += 1
        rel = path.relative_to(ROOT)
        top = rel.parts[0] if len(rel.parts) > 1 else "."
        code, comment, blank = count_loc(path)
        total_code += code
        total_comment += comment
        total_blank += blank
        loc_by_dir[top] += code
        try:
            text = path.read_text(encoding="utf-8", errors="replace")
        except OSError:
            continue
        include_edges += len(re.findall(r'^\s*#include\s+', text, re.M))
        if ext in {".cpp", ".c"}:
            cpp_files += 1
            file_complexity.append((str(rel), cyclomatic_proxy(text), code))
        elif ext in {".h", ".hpp"}:
            header_files += 1
        elif ext == ".py":
            py_files += 1

    file_complexity.sort(key=lambda t: t[1], reverse=True)
    return {
        "loc_code": total_code,
        "loc_comment": total_comment,
        "loc_blank": total_blank,
        "loc_by_dir": dict(loc_by_dir),
        "files_by_ext": dict(files_by_ext),
        "cpp_files": cpp_files,
        "header_files": header_files,
        "py_files": py_files,
        "include_edges": include_edges,
        "file_complexity": file_complexity,
        "mean_file_cc": (
            sum(c for _, c, _ in file_complexity) / len(file_complexity)
            if file_complexity
            else 0.0
        ),
        "max_file_cc": file_complexity[0][1] if file_complexity else 0,
    }


def parse_metrics_from_output(text: str) -> dict:
    out = {}
    for key, pat in [
        ("accuracy", r"Accuracy\s*:\s*([0-9.]+)"),
        ("precision", r"Precision\s*:\s*([0-9.]+)"),
        ("recall", r"Recall\s*:\s*([0-9.]+)"),
        ("f1", r"F1(?: Score)?\s*:\s*([0-9.]+)"),
        ("build_ms", r"Build time\s*:\s*([0-9.]+)"),
        ("classify_ms", r"Classify time\s*:\s*([0-9.]+)"),
        ("total_ms", r"(?:Total time|Time)\s*:\s*([0-9.]+)"),
        ("correct", r"Correct\s*:\s*(\d+)"),
        ("total", r"Total\s*:\s*(\d+)"),
    ]:
        m = re.search(pat, text)
        if m:
            out[key] = m.group(1)
    return out


def run_exe(path: Path, timeout: int = 120) -> tuple[int, str]:
    if not path.exists():
        return 1, ""
    try:
        proc = subprocess.run(
            [str(path)],
            cwd=str(ROOT),
            capture_output=True,
            text=True,
            timeout=timeout,
        )
        return proc.returncode, proc.stdout + proc.stderr
    except (OSError, subprocess.TimeoutExpired):
        return 1, ""


def collect_runtime_metrics() -> dict:
    """Run existing challenge binaries when present; otherwise N/A."""
    results = {}
    mapping = {
        "challenge_500": ROOT / "build-verify" / "tests" / "challenge_500",
        "challenge_5000": ROOT / "build-verify" / "tests" / "challenge_5000",
        "challenge_50000": ROOT / "build-verify" / "tests" / "challenge_50000",
    }
    for name, exe in mapping.items():
        code, text = run_exe(exe)
        parsed = parse_metrics_from_output(text)
        parsed["exit_code"] = code
        parsed["ran"] = exe.exists()
        results[name] = parsed
    return results


# PDE classifier buckets (from PDE.cpp) — synthetic challenge mix is 20% each.
PDE_BUCKETS = [
    ("DEAD", 0.00, 0.00),
    ("MOSTLY DEAD", 0.00, 0.25),
    ("PARTIALLY DEAD", 0.25, 0.75),
    ("MOSTLY LIVE", 0.75, 1.00),
    ("LIVE", 1.00, 1.00),
]


# ---------------------------------------------------------------------------
# PDF extraction
# ---------------------------------------------------------------------------

def extract_pdf_pages(pdf_path: Path) -> tuple[int, str, list[list[list[str]]]]:
    """Return (page_count, full_text, tables as lists of rows)."""
    import pdfplumber

    texts: list[str] = []
    tables: list[list[list[str]]] = []
    with pdfplumber.open(str(pdf_path)) as pdf:
        n_pages = len(pdf.pages)
        for page in pdf.pages:
            texts.append(page.extract_text() or "")
            for table in page.extract_tables() or []:
                cleaned = [
                    ["" if cell is None else str(cell).replace("\n", " ").strip() for cell in row]
                    for row in table
                    if row and any(cell not in (None, "") for cell in row)
                ]
                if cleaned:
                    tables.append(cleaned)
    return n_pages, "\n".join(texts), tables


def table_to_rows(source: str, tables: list[list[list[str]]]) -> list[dict]:
    rows = []
    for i, table in enumerate(tables, 1):
        header = table[0]
        body = table[1:] if len(table) > 1 else []
        if not body:
            # single-row / caption-like table
            rows.append(
                {
                    "source": source,
                    "table_index": i,
                    "row_kind": "raw",
                    "key": f"table_{i}_row_0",
                    "value": " | ".join(header),
                    "notes": "no header/body split",
                }
            )
            continue
        for r, row in enumerate(body):
            # pad
            while len(row) < len(header):
                row.append("")
            payload = {header[c] or f"col_{c}": row[c] for c in range(len(header))}
            rows.append(
                {
                    "source": source,
                    "table_index": i,
                    "row_kind": "record",
                    "key": payload.get(header[0], f"row_{r}"),
                    "value": str(payload),
                    "notes": "",
                    **{f"col_{c}": row[c] for c in range(min(8, len(row)))},
                }
            )
    return rows


def flatten_known_metrics(source: str, items: list[tuple[str, object, str]]) -> list[dict]:
    out = []
    for name, value, notes in items:
        out.append(
            {
                "source": source,
                "metric_name": name,
                "value": na(value),
                "unit": "",
                "notes": notes,
                "extraction": "manual+text",
            }
        )
    return out


# Numbers confirmed from full-text inspection of each PDF (figures often
# have no machine-readable table). Units live in `notes`.
MUZEEL_METRICS = [
    ("pages", 14, "ACM IMC '22"),
    ("corpus_web_pages", 15000, "Alexa top list landing pages"),
    ("corpus_js_files", 300000, "JS files in those pages"),
    ("unused_functions_median_pct", 70, "% unused functions on median page"),
    ("unused_filesize_median_pct", 55, "% of JS file size unused"),
    ("eliminated_functions_per_file_median", 67, "functions / JS file"),
    ("eliminated_bytes_per_file_median_kb", 10, "KB / JS file"),
    ("eliminated_functions_per_page_mean", 2000, "functions / page"),
    ("eliminated_bytes_per_page_mean_kb", 700, "KB / page"),
    ("page_size_reduction_median_kb", 400, "Chrome/Firefox/Edge median bandwidth save"),
    ("pages_under_5min_pct", 85, "% of pages Muzeel finishes in <5 min (6 cores)"),
    ("median_runtime_64core_s", 30, "seconds, all 64 cores"),
    ("js_unchanged_over_week_pct", 99, "Muzeel-ed JS stability"),
    ("perf_eval_pages", 200, "phones/browsers/networks subset"),
    ("loads_per_page", 5, "median of 5 loads reported"),
    ("structural_similarity_p90_pct", 90, "90% of 200 pages score >90%"),
    ("lacuna_structural_p90_pct", 60, "same 200-page set"),
    ("plt_speedup_pct", 27.5, "abstract range 25-30%; midpoint for plotting"),
    ("plt_speedup_pct_low", 25, "abstract lower bound"),
    ("plt_speedup_pct_high", 30, "abstract upper bound"),
    ("max_3g_speedup_s", 9, "seconds faster on 3G, low- and high-end"),
    ("user_no_func_impact_pct", 80, "user study: none or no-impact missing functions"),
    ("user_no_struct_impact_pct", 90, "user study: none or no-impact missing structure"),
    ("testbed_phones", 3, "Redmi Go, J3, S10 — Table 1"),
    ("testbed_browsers", 4, "Chrome, Firefox, Edge, Brave"),
]

DIE_METRICS = [
    ("pages", 4, "IMPACT 2025 workshop"),
    ("benchmark_tables", 0, "no numeric evaluation table"),
    ("llama_prompt_time_reduction_pct", 3, "LLaMa 3.1 8B last decoder / logits row"),
    ("algorithm_steps", 6, "raise, DDG, invert, analyze, restrict, codegen"),
]

AUTOJMH_METRICS = [
    ("pages", 12, "ASE '16"),
    ("projects", 5, "Math, Vectorz, Lang, JSyn, Img2"),
    ("total_loops_abstract", 6028, "abstract / RQ1 prose"),
    ("total_loops_table2", 6082, "Table 2 total column; disagrees with abstract"),
    ("payloads_generated", 4705, "Table 2"),
    ("payloads_generated_pct", 77, "Table 2 of 6082"),
    ("payloads_initialized", 3485, "Table 2"),
    ("microbenchmarks_generated", 3462, "Table 2"),
    ("rejected", 1377, "precondition failures"),
    ("rejected_pct", 23, ""),
    ("unsupported_variables", 1027, ""),
    ("unsupported_invocations", 350, ""),
    ("regression_failures", 23, "0.4%"),
    ("expert_match_autojmh", 23, "of 23; CI overlap α=0.05"),
    ("expert_match_dce_off", 0, "sink maximization off"),
    ("expert_match_cfcp_inverted", 11, "declaration rules inverted"),
    ("expert_match_bad_init", 3, "random/wrong inputs"),
    ("sort_linkedlist_sorted_ns", 203, "Table 1"),
    ("sort_linkedlist_unsorted_ns", 453, "Table 1"),
    ("sort_vector_sorted_ns", 1639, "Table 1"),
    ("sort_vector_unsorted_ns", 645, "Table 1"),
    ("rq3_engineers", 6, "professional Java, little microbenchmark experience"),
    ("rq3_suas", 5, "all handwritten payloads distorted"),
]


def attach_units(rows: list[dict], catalog: list[tuple[str, object, str]]) -> None:
    units = {
        "unused_functions_median_pct": "%",
        "unused_filesize_median_pct": "%",
        "eliminated_bytes_per_file_median_kb": "KB",
        "eliminated_bytes_per_page_mean_kb": "KB",
        "page_size_reduction_median_kb": "KB",
        "pages_under_5min_pct": "%",
        "median_runtime_64core_s": "s",
        "js_unchanged_over_week_pct": "%",
        "structural_similarity_p90_pct": "%",
        "lacuna_structural_p90_pct": "%",
        "plt_speedup_pct": "%",
        "max_3g_speedup_s": "s",
        "llama_prompt_time_reduction_pct": "%",
        "payloads_generated_pct": "%",
        "rejected_pct": "%",
        "sort_linkedlist_sorted_ns": "ns",
        "sort_linkedlist_unsorted_ns": "ns",
        "sort_vector_sorted_ns": "ns",
        "sort_vector_unsorted_ns": "ns",
    }
    for row in rows:
        row["unit"] = units.get(row["metric_name"], row.get("unit", ""))


# ---------------------------------------------------------------------------
# Comparison table
# ---------------------------------------------------------------------------

def build_comparison(code: dict, runtimes: dict) -> list[dict]:
    ch500 = runtimes.get("challenge_500", {})
    ch50k = runtimes.get("challenge_50000", {})

    def row(name, mine, paper, pval, notes):
        return {
            "metric_name": name,
            "my_code_value": na(mine),
            "paper_source": paper,
            "paper_value": na(pval),
            "notes": notes,
        }

    return [
        row("language", "C++17", "Muzeel", "JavaScript (web)", "different host languages"),
        row("language", "C++17", "DIE", "C / polyhedral / ML ops", "compiler IR vs tensor ops"),
        row("language", "C++17", "AutoJMH", "Java / JMH", "analyzer vs benchmark generator"),
        row(
            "dead_code_granularity",
            "variable paths + IR instructions",
            "Muzeel",
            "JS functions after interaction crawl",
            "conceptual overlap only",
        ),
        row(
            "dead_code_granularity",
            "variable paths + IR instructions",
            "DIE",
            "loop iterations (polyhedral)",
            "DIE is finer than statement DCE; ModernPDE is path-ratio PDE",
        ),
        row(
            "constant_folding",
            "SCCP lattice on FunctionIR",
            "AutoJMH",
            "prevent JIT CF/CP in payloads",
            "same compiler idea; opposite goal (analyze vs keep live)",
        ),
        row("loc_code", code["loc_code"], "all papers", "N/A", "tool size not reported"),
        row("cpp_translation_units", code["cpp_files"], "all papers", "N/A", "code-only"),
        row("header_files", code["header_files"], "all papers", "N/A", "code-only"),
        row("include_directives", code["include_edges"], "all papers", "N/A", "dependency proxy"),
        row(
            "mean_file_cyclomatic_proxy",
            f"{code['mean_file_cc']:.1f}",
            "all papers",
            "N/A",
            "1 + decision keywords per .cpp/.c file",
        ),
        row("ctest_targets", 14, "all papers", "N/A", "local CMake suite"),
        row(
            "pde_accuracy",
            ch500.get("accuracy", "N/A"),
            "Muzeel",
            "90% pages >90% visual/functional similarity",
            "same word, different definition",
        ),
        row(
            "pde_f1",
            ch500.get("f1", "N/A"),
            "Muzeel",
            "N/A",
            "ModernPDE F1 is dead/partial vs live on synthetic labels",
        ),
        row(
            "pde_precision",
            ch500.get("precision", "N/A"),
            "AutoJMH",
            "23/23 expert-time match",
            "not the same statistic",
        ),
        row(
            "scale_items_analyzed",
            "500 / 5000 / 50000 synthetic variables",
            "Muzeel",
            "15000 pages, 300000 JS files",
            "incomparable corpora",
        ),
        row(
            "scale_items_analyzed",
            "500 / 5000 / 50000 synthetic variables",
            "AutoJMH",
            "6082 loops (Table 2)",
            "incomparable corpora",
        ),
        row(
            "runtime_50k_classify_ms",
            ch50k.get("classify_ms") or ch50k.get("total_ms", "N/A"),
            "Muzeel",
            "median 30 s / page on 64 cores",
            "PDE classify vs web crawl DCE",
        ),
        row(
            "speedup_pct",
            "Phase 8 cache vs Phase 7 (internal)",
            "Muzeel",
            "25-30% faster page load",
            "do not overlay on one axis",
        ),
        row(
            "speedup_pct",
            "N/A as end-to-end app time",
            "DIE",
            "3% LLaMa 3.1 8B prompt",
            "paper-only numeric result",
        ),
        row(
            "size_reduction_pct",
            "N/A (no shipped JS/binary shrink reported)",
            "Muzeel",
            "55% of JS file size unused",
            "paper-only unless dead-insn fraction used as proxy",
        ),
        row(
            "page_bytes_saved_kb",
            "N/A",
            "Muzeel",
            400,
            "median bandwidth save",
        ),
        row(
            "microbenchmarks_generated",
            "N/A",
            "AutoJMH",
            3462,
            "paper-only",
        ),
        row(
            "expert_benchmark_agreement",
            "N/A",
            "AutoJMH",
            "23/23",
            "paper-only",
        ),
        row(
            "cfg_cyclomatic_of_subject",
            "CFGStatistics on samples",
            "all papers",
            "N/A",
            "code-only analysis metric",
        ),
        row(
            "web_fcp_si_plt",
            "N/A",
            "Muzeel",
            "reported as CDF deltas",
            "paper-only UX metrics",
        ),
        row(
            "sort_vector_sorted_ns",
            "N/A",
            "AutoJMH",
            1639,
            "Table 1 pitfall example, not a DCE result",
        ),
    ]


# ---------------------------------------------------------------------------
# Markdown
# ---------------------------------------------------------------------------

def md_table(headers: list[str], rows: list[list[str]]) -> str:
    lines = ["| " + " | ".join(headers) + " |", "| " + " | ".join(["---"] * len(headers)) + " |"]
    for row in rows:
        lines.append("| " + " | ".join(str(c) for c in row) + " |")
    return "\n".join(lines)


def write_markdown(
    code: dict,
    runtimes: dict,
    comparison: list[dict],
    muzeel_pages: int,
    die_pages: int,
    auto_pages: int,
    n_muzeel_tables: int,
    n_die_tables: int,
    n_auto_tables: int,
) -> None:
    ch = runtimes.get("challenge_500", {})
    ch5k = runtimes.get("challenge_5000", {})
    ch50 = runtimes.get("challenge_50000", {})
    top_cc = code["file_complexity"][:8]
    loc_rows = sorted(code["loc_by_dir"].items(), key=lambda kv: -kv[1])

    body = f"""# ModernPDE vs papers — benchmark extract

Generated by `scripts/export_paper_comparison.py`.
Code metrics are computed from the tree (skipping build/venv).
Paper numbers mix pdfplumber tables with values transcribed from the PDF text
when the paper stores results in figures rather than grids.

## Artifact A — ModernPDE

C++17 CMake static analyzer: class hierarchy, virtual calls, context-sensitive
call graphs, path-aware PDE, CFG/DU/paths, field-sensitive analysis, SCCP,
faint analysis, Phase 8 caches.

{md_table(
    ["metric", "value"],
    [
        ["LOC (code)", code["loc_code"]],
        ["LOC (comments)", code["loc_comment"]],
        ["LOC (blank)", code["loc_blank"]],
        [".cpp/.c files", code["cpp_files"]],
        ["headers", code["header_files"]],
        ["python files", code["py_files"]],
        ["#include count", code["include_edges"]],
        ["mean file cyclomatic proxy", f"{code['mean_file_cc']:.1f}"],
        ["max file cyclomatic proxy", code["max_file_cc"]],
        ["CTest targets", "14"],
    ],
)}

### LOC by top-level directory

{md_table(["directory", "code lines"], [[k, v] for k, v in loc_rows])}

### Highest cyclomatic-proxy files

{md_table(["file", "cc_proxy", "code_lines"], [[a, b, c] for a, b, c in top_cc])}

### Challenge-suite PDE metrics (executed if `build-verify` binaries exist)

{md_table(
    ["suite", "ran", "accuracy", "precision", "recall", "f1", "time_ms"],
    [
        ["challenge_500", ch.get("ran"), ch.get("accuracy", "N/A"), ch.get("precision", "N/A"), ch.get("recall", "N/A"), ch.get("f1", "N/A"), ch.get("total_ms", "N/A")],
        ["challenge_5000", ch5k.get("ran"), ch5k.get("accuracy", "N/A"), ch5k.get("precision", "N/A"), ch5k.get("recall", "N/A"), ch5k.get("f1", "N/A"), ch5k.get("total_ms", "N/A")],
        ["challenge_50000", ch50.get("ran"), ch50.get("accuracy", "N/A"), ch50.get("precision", "N/A"), ch50.get("recall", "N/A"), ch50.get("f1", "N/A"), ch50.get("classify_ms") or ch50.get("total_ms", "N/A")],
    ],
)}

These labels are synthetic: each variable is constructed to land in a known
usage-ratio bucket, so perfect accuracy does **not** mean the analyzer matches
Muzeel or LLVM DCE on real programs.

## Papers

| PDF | pages | machine tables | domain |
| --- | --- | --- | --- |
| Muzeel (3517745.3561427.pdf) | {muzeel_pages} | {n_muzeel_tables} | JS DCE on mobile web |
| DIE (die.pdf) | {die_pages} | {n_die_tables} | polyhedral dead iterations |
| AutoJMH (Generation…pdf) | {auto_pages} | {n_auto_tables} | JMH payloads vs JIT DCE/CF |

### What overlaps

- **Dead code** is the shared topic. Muzeel removes unused JS *functions* after
  interaction emulation. DIE removes unused *loop iterations*. ModernPDE
  classifies *variables* by path usage and marks unused IR results.
- **Constant folding**: ModernPDE SCCP vs AutoJMH *preventing* JIT CF/CP.
- **Accuracy / F1 / runtime / speedup** appear on both sides but measure
  different things. They are kept in the comparison CSV with notes.

### Paper-only context

Muzeel: FCP, SpeedIndex, PLT, CPU, bandwidth, battery, 3G/LTE, 15k-page corpus.
DIE: polyhedral image/preimage, LLaMa 3% note, no eval table.
AutoJMH: sink maximization, JMH ns, 6k Java loops, expert CI overlap.

### Extraction issues

- Muzeel results live in histogram/CDF **figures**, not grids. Medians/means
  were transcribed from section 6 text.
- DIE has **no** numeric table.
- AutoJMH Table 2 packs count+percent into cells; abstract says 6,028 loops,
  Table 2 totals **6,082**.

## Comparison (abbreviated)

{md_table(
    ["metric", "ModernPDE", "paper", "paper value"],
    [[r["metric_name"], r["my_code_value"], r["paper_source"], r["paper_value"]] for r in comparison[:18]],
)}

Full rows: `output/paper-comparison/code_vs_papers_comparison.csv`.
Plots: `output/plots/*.png`.
"""
    (BENCH / "code_vs_papers_summary.md").write_text(body, encoding="utf-8")


# ---------------------------------------------------------------------------
# Plots — one category each
# ---------------------------------------------------------------------------

def setup_pyplot():
    import matplotlib.pyplot as plt

    plt.rcParams.update(
        {
            "font.family": "DejaVu Sans",
            "font.size": 11,
            "axes.spines.top": False,
            "axes.spines.right": False,
            "axes.grid": True,
            "grid.alpha": 0.28,
            "grid.linewidth": 0.5,
            "figure.dpi": 140,
            "axes.facecolor": "white",
            "figure.facecolor": "white",
        }
    )
    return plt


BLUE = "#2F5D8A"
OCHRE = "#C4783A"
GREEN = "#3D7A5A"
SLATE = "#4A5560"
RED = "#A33B3B"


def save(fig, name: str) -> None:
    fig.tight_layout()
    fig.savefig(PLOTS / name, bbox_inches="tight")
    fig.clf()


def make_plots(code: dict, runtimes: dict) -> None:
    plt = setup_pyplot()

    # 1. LOC by directory
    items = sorted(code["loc_by_dir"].items(), key=lambda kv: kv[1])
    fig, ax = plt.subplots(figsize=(8, 4.5))
    ax.barh([k for k, _ in items], [v for _, v in items], color=BLUE)
    ax.set_xlabel("Code lines")
    ax.set_title("ModernPDE: code lines by directory")
    ax.text(0.0, -0.18, "Source: repo walk, excluding build/venv", transform=ax.transAxes, fontsize=8, color=SLATE)
    save(fig, "code_loc_by_directory.png")

    # 2. File counts
    fig, ax = plt.subplots(figsize=(7, 4))
    labels = [".cpp/.c", "headers", "python"]
    vals = [code["cpp_files"], code["header_files"], code["py_files"]]
    ax.bar(labels, vals, color=[BLUE, OCHRE, GREEN])
    ax.set_ylabel("Files")
    ax.set_title("ModernPDE: source file counts")
    save(fig, "code_file_counts.png")

    # 3. Cyclomatic proxy — top files
    top = code["file_complexity"][:10]
    fig, ax = plt.subplots(figsize=(8, 5))
    ax.barh([p.split("/")[-1] for p, _, _ in reversed(top)], [c for _, c, _ in reversed(top)], color=OCHRE)
    ax.set_xlabel("Cyclomatic proxy (1 + decision keywords)")
    ax.set_title("ModernPDE: highest file-level cyclomatic proxy")
    ax.text(0.0, -0.16, "Heuristic, not AST McCabe", transform=ax.transAxes, fontsize=8, color=SLATE)
    save(fig, "code_cyclomatic_top_files.png")

    # 4. PDE buckets (challenge mix is 20% each by construction)
    fig, ax = plt.subplots(figsize=(8, 4.2))
    names = [b[0] for b in PDE_BUCKETS]
    ax.bar(names, [20] * 5, color=BLUE)
    ax.set_ylabel("Share of challenge variables (%)")
    ax.set_title("ModernPDE PDE: challenge-suite label mix")
    ax.set_ylim(0, 30)
    plt.setp(ax.get_xticklabels(), rotation=15, ha="right")
    save(fig, "pde_challenge_label_mix.png")

    # 5. Challenge accuracy/F1
    suites = ["500", "5000", "50000"]
    keys = ["challenge_500", "challenge_5000", "challenge_50000"]
    acc, f1 = [], []
    for k in keys:
        r = runtimes.get(k, {})
        acc.append(float(r["accuracy"]) if r.get("accuracy") else 0.0)
        f1.append(float(r["f1"]) if r.get("f1") else 0.0)
    fig, ax = plt.subplots(figsize=(7, 4.2))
    x = range(len(suites))
    ax.bar([i - 0.18 for i in x], acc, width=0.36, label="Accuracy", color=BLUE)
    ax.bar([i + 0.18 for i in x], f1, width=0.36, label="F1", color=OCHRE)
    ax.set_xticks(list(x), suites)
    ax.set_xlabel("Challenge variable count")
    ax.set_ylabel("Score (0–1)")
    ax.set_ylim(0, 1.15)
    ax.set_title("ModernPDE PDE: accuracy and F1 on synthetic challenges")
    ax.legend()
    ax.text(0.0, -0.2, "Labels constructed to match classifier buckets", transform=ax.transAxes, fontsize=8, color=SLATE)
    save(fig, "pde_challenge_accuracy.png")

    # 6. Muzeel unused / size
    fig, ax = plt.subplots(figsize=(7.5, 4.2))
    cats = ["Unused functions\n(median %)", "Unused file size\n(median %)", "PLT speedup\n(mid %)"]
    ax.bar(cats, [70, 55, 27.5], color=[BLUE, OCHRE, GREEN])
    ax.set_ylabel("Percent")
    ax.set_title("Muzeel: dead JS share and page-load speedup")
    ax.set_ylim(0, 100)
    ax.text(0.0, -0.22, "Source: Muzeel IMC'22 text (figures, not tables)", transform=ax.transAxes, fontsize=8, color=SLATE)
    save(fig, "muzeel_dce_and_speedup.png")

    # 7. Muzeel similarity vs Lacuna
    fig, ax = plt.subplots(figsize=(7, 4.2))
    ax.bar(["Muzeel structural\nP90>90% pages", "Lacuna structural\nsame cutoff"], [90, 60], color=[GREEN, RED])
    ax.set_ylabel("Pages scoring above 90% similarity (%)")
    ax.set_title("Muzeel vs Lacuna: structural similarity on 200 pages")
    ax.set_ylim(0, 100)
    save(fig, "muzeel_similarity_vs_lacuna.png")

    # 8. Muzeel bytes
    fig, ax = plt.subplots(figsize=(7.5, 4.2))
    ax.bar(
        ["Per JS file\n(median KB)", "Per page mean\n(KB)", "Browser median\nsave (KB)"],
        [10, 700, 400],
        color=BLUE,
    )
    ax.set_ylabel("Kilobytes")
    ax.set_title("Muzeel: eliminated JavaScript bytes")
    save(fig, "muzeel_bytes_eliminated.png")

    # 9. AutoJMH reach
    fig, ax = plt.subplots(figsize=(7.5, 4.2))
    labels = ["Loops\n(Table 2)", "Payloads", "Initialized", "Microbenchmarks", "Rejected"]
    ax.bar(labels, [6082, 4705, 3485, 3462, 1377], color=BLUE)
    ax.set_ylabel("Count")
    ax.set_title("AutoJMH: reach on five Java projects")
    ax.text(0.0, -0.2, "Source: ASE'16 Table 2 (total 6082; abstract says 6028)", transform=ax.transAxes, fontsize=8, color=SLATE)
    save(fig, "autojmh_reach.png")

    # 10. AutoJMH vs experts
    fig, ax = plt.subplots(figsize=(7.5, 4.2))
    ax.bar(
        ["AutoJMH", "DCE off", "CF/CP inverted", "Bad init"],
        [23, 0, 11, 3],
        color=[GREEN, RED, OCHRE, SLATE],
    )
    ax.set_ylabel("Microbenchmarks matching expert times (of 23)")
    ax.set_title("AutoJMH: generated vs handwritten expert times")
    ax.set_ylim(0, 25)
    save(fig, "autojmh_vs_experts.png")

    # 11. AutoJMH Table 1 sort
    fig, ax = plt.subplots(figsize=(7, 4.2))
    cats = ["LinkedList", "Vector"]
    ax.bar([i - 0.18 for i in range(2)], [203, 1639], width=0.36, label="Already sorted", color=BLUE)
    ax.bar([i + 0.18 for i in range(2)], [453, 645], width=0.36, label="Unsorted", color=OCHRE)
    ax.set_xticks([0, 1], cats)
    ax.set_ylabel("Time (ns)")
    ax.set_title("AutoJMH Table 1: Collections.sort pitfall")
    ax.legend()
    save(fig, "autojmh_collections_sort.png")

    # 12. DIE single result
    fig, ax = plt.subplots(figsize=(6.5, 4))
    ax.bar(["LLaMa 3.1 8B\nprompt-time cut"], [3], color=GREEN)
    ax.set_ylabel("Percent")
    ax.set_title("DIE: only reported numeric speedup")
    ax.set_ylim(0, 10)
    ax.text(0.0, -0.2, "Source: DIE IMPACT'25 conclusion (no eval table)", transform=ax.transAxes, fontsize=8, color=SLATE)
    save(fig, "die_llama_time_reduction.png")

    # 13. Summary — comparable-name metrics, different meaning
    fig, ax = plt.subplots(figsize=(8, 4.8))
    names = ["Accuracy-like\nscore (%)", "Runtime scale\n(log items)", "Speedup-like\n(%)"]
    # Map to roughly comparable display units with captions
    mine = [float(runtimes.get("challenge_500", {}).get("accuracy", 0) or 0) * 100, 4.7, 0]
    # 50000 vars ~ 10^4.7; muzeel 15k pages ~ 4.18
    paper = [90, 4.18, 27.5]
    ax.bar([i - 0.18 for i in range(3)], mine, width=0.36, label="ModernPDE", color=BLUE)
    ax.bar([i + 0.18 for i in range(3)], paper, width=0.36, label="Papers (mixed)", color=OCHRE)
    ax.set_xticks(list(range(3)), names)
    ax.set_title("At-a-glance: same-name metrics, not the same measurement")
    ax.legend()
    ax.text(
        0.0,
        -0.22,
        "Accuracy: PDE labels vs Muzeel visual P90. Scale: log10(50k vars) vs log10(15k pages). Speedup: N/A vs Muzeel PLT.",
        transform=ax.transAxes,
        fontsize=8,
        color=SLATE,
    )
    save(fig, "summary_same_name_metrics.png")

    plt.close("all")


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> int:
    ensure_dirs()
    sys.path.insert(0, str(ROOT / ".venv" / "lib"))

    print("Computing code metrics…")
    code = collect_code_metrics()
    print(f"  LOC code={code['loc_code']} cpp={code['cpp_files']} headers={code['header_files']}")

    print("Running challenge binaries if present…")
    runtimes = collect_runtime_metrics()
    for name, rec in runtimes.items():
        print(f"  {name}: ran={rec.get('ran')} acc={rec.get('accuracy', 'N/A')}")

    print("Extracting PDFs…")
    m_pages, m_text, m_tables = extract_pdf_pages(PDF_MUZEEL)
    d_pages, d_text, d_tables = extract_pdf_pages(PDF_DIE)
    a_pages, a_text, a_tables = extract_pdf_pages(PDF_AUTOJMH)
    print(f"  Muzeel pages={m_pages} tables={len(m_tables)}")
    print(f"  DIE pages={d_pages} tables={len(d_tables)}")
    print(f"  AutoJMH pages={a_pages} tables={len(a_tables)}")

    muzeel_rows = flatten_known_metrics("3517745.3561427.pdf", MUZEEL_METRICS)
    attach_units(muzeel_rows, MUZEEL_METRICS)
    muzeel_rows += [
        {
            "source": "3517745.3561427.pdf",
            "metric_name": f"extracted_table_{i}",
            "value": str(len(t)) + " rows",
            "unit": "",
            "notes": "pdfplumber table; first row=" + " | ".join(t[0][:6]),
            "extraction": "pdfplumber",
        }
        for i, t in enumerate(m_tables, 1)
    ]
    muzeel_rows.append(
        {
            "source": "3517745.3561427.pdf",
            "metric_name": "word_count_approx",
            "value": len(m_text.split()),
            "unit": "words",
            "notes": "whitespace tokens",
            "extraction": "pdfplumber",
        }
    )

    die_rows = flatten_known_metrics("die.pdf", DIE_METRICS)
    die_rows.append(
        {
            "source": "die.pdf",
            "metric_name": "word_count_approx",
            "value": len(d_text.split()),
            "unit": "words",
            "notes": "whitespace tokens",
            "extraction": "pdfplumber",
        }
    )
    for i, t in enumerate(d_tables, 1):
        die_rows.append(
            {
                "source": "die.pdf",
                "metric_name": f"extracted_table_{i}",
                "value": " | ".join(t[0][:8]),
                "unit": "",
                "notes": f"{len(t)} rows from pdfplumber",
                "extraction": "pdfplumber",
            }
        )

    auto_rows = flatten_known_metrics(
        "Generation to Prevent DeadCode Elimination and Constant Folding (1).pdf",
        AUTOJMH_METRICS,
    )
    attach_units(auto_rows, AUTOJMH_METRICS)
    auto_rows.append(
        {
            "source": "Generation to Prevent DeadCode Elimination and Constant Folding (1).pdf",
            "metric_name": "word_count_approx",
            "value": len(a_text.split()),
            "unit": "words",
            "notes": "whitespace tokens",
            "extraction": "pdfplumber",
        }
    )
    for i, t in enumerate(a_tables, 1):
        auto_rows.append(
            {
                "source": "Generation to Prevent DeadCode Elimination and Constant Folding (1).pdf",
                "metric_name": f"extracted_table_{i}",
                "value": str(len(t)) + " rows",
                "unit": "",
                "notes": "pdfplumber table; first row=" + " | ".join(t[0][:8]),
                "extraction": "pdfplumber",
            }
        )

    fields = ["source", "metric_name", "value", "unit", "notes", "extraction"]
    write_csv(BENCH / "3517745_3561427_data.csv", fields, muzeel_rows)
    write_csv(BENCH / "die_data.csv", fields, die_rows)
    write_csv(BENCH / "deadcode_constant_folding_data.csv", fields, auto_rows)

    # Extra: dump AutoJMH Table 2-ish raw tables
    raw_auto = table_to_rows("autojmh", a_tables)
    if raw_auto:
        raw_fields = ["source", "table_index", "row_kind", "key", "value", "notes", "col_0", "col_1", "col_2", "col_3", "col_4", "col_5"]
        write_csv(BENCH / "deadcode_constant_folding_tables_raw.csv", raw_fields, raw_auto)

    raw_m = table_to_rows("muzeel", m_tables)
    if raw_m:
        raw_fields = ["source", "table_index", "row_kind", "key", "value", "notes", "col_0", "col_1", "col_2", "col_3", "col_4", "col_5"]
        write_csv(BENCH / "3517745_3561427_tables_raw.csv", raw_fields, raw_m)

    code_rows = [
        {"metric_name": "loc_code", "value": code["loc_code"], "notes": "physical non-comment non-blank"},
        {"metric_name": "loc_comment", "value": code["loc_comment"], "notes": ""},
        {"metric_name": "loc_blank", "value": code["loc_blank"], "notes": ""},
        {"metric_name": "cpp_files", "value": code["cpp_files"], "notes": ".cpp/.c"},
        {"metric_name": "header_files", "value": code["header_files"], "notes": ""},
        {"metric_name": "python_files", "value": code["py_files"], "notes": ""},
        {"metric_name": "include_directives", "value": code["include_edges"], "notes": "dependency proxy"},
        {"metric_name": "mean_file_cyclomatic_proxy", "value": f"{code['mean_file_cc']:.2f}", "notes": ""},
        {"metric_name": "max_file_cyclomatic_proxy", "value": code["max_file_cc"], "notes": code["file_complexity"][0][0] if code["file_complexity"] else ""},
        {"metric_name": "ctest_targets", "value": 14, "notes": "tests/CMakeLists.txt"},
    ]
    for name, rec in runtimes.items():
        for k, v in rec.items():
            if k in {"ran", "exit_code"}:
                continue
            code_rows.append({"metric_name": f"{name}_{k}", "value": v, "notes": "from executed binary" if rec.get("ran") else "N/A"})
    write_csv(
        BENCH / "modernpde_code_metrics.csv",
        ["metric_name", "value", "notes"],
        code_rows,
    )

    comparison = build_comparison(code, runtimes)
    write_csv(
        BENCH / "code_vs_papers_comparison.csv",
        ["metric_name", "my_code_value", "paper_source", "paper_value", "notes"],
        comparison,
    )

    write_markdown(
        code,
        runtimes,
        comparison,
        m_pages,
        d_pages,
        a_pages,
        len(m_tables),
        len(d_tables),
        len(a_tables),
    )

    print("Writing plots…")
    make_plots(code, runtimes)

    print("Done.")
    print(f"  CSVs/MD -> {BENCH}")
    print(f"  PNGs    -> {PLOTS}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

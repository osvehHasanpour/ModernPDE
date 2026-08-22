#!/usr/bin/env python3
"""
compare_artifact_vs_papers.py
=============================

Step 2–4 of the Artifact A vs reference-paper comparison.

Artifact A  = ModernPDE (this repository)
B1          = Muzeel (IMC'22)           docs/papers/muzeel.pdf
B2          = Dead Iteration Elimination (IMPACT 2025)  docs/papers/die.pdf
B3          = AutoJMH (ASE'16)          docs/papers/autojmh.pdf

Rules enforced here
-------------------
* B1 / B2 / B3 are analyzed independently — never merged into one score.
* Identical metric *names* do not imply comparability; units, input size,
  hardware, OS, compiler, flags, run count, and methodology must match.
* Missing values are written as N/A — never invented.
* Performance numbers for A are taken only from harnesses/files that exist
  (or marked N/A if not re-measured in this run).

Outputs
-------
  output/paper-comparison/B1_muzeel_extract.csv
  output/paper-comparison/B1_muzeel_comparison.csv
  output/paper-comparison/B2_die_extract.csv
  output/paper-comparison/B2_die_comparison.csv
  output/paper-comparison/B3_autojmh_extract.csv
  output/paper-comparison/B3_autojmh_comparison.csv
  output/paper-comparison/A_code_metrics.csv
  output/plots/*.png          (one category per file)
  output/comparison-report.md

Run (project root, WSL):
  .venv/bin/python scripts/compare_artifact_vs_papers.py
"""

from __future__ import annotations

import csv
import re
import sys
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PAPERS = ROOT / "docs" / "papers"
OUT = ROOT / "output" / "paper-comparison"
PLOTS = ROOT / "output" / "plots"
REPORT = ROOT / "output" / "comparison-report.md"

PDF_B1 = PAPERS / "muzeel.pdf"
PDF_B2 = PAPERS / "die.pdf"
PDF_B3 = PAPERS / "autojmh.pdf"

SKIP_DIRS = {
    ".git",
    ".venv",
    "venv",
    "build",
    "build-baseline",
    "build-verify",
    "build-benchmark",
    "output",
    "node_modules",
    "__pycache__",
    "CMakeFiles",
}
SKIP_SUFFIX = {".o", ".a", ".so", ".dll", ".exe", ".pdf", ".png", ".zip"}

DECISION_RE = re.compile(
    r"\b(if|else\s+if|for|while|case|catch|&&|\|\||\?)\b"
)
CLASS_RE = re.compile(r"^\s*(class|struct)\s+\w+")
TEMPLATE_RE = re.compile(r"\btemplate\s*<")

DIRECT = "Directly comparable"
PARTIAL = "Partially comparable"
NOT_DIRECT = "Not directly comparable"
PAPER_ONLY = "Paper-only"
CODE_ONLY = "Code-only"

# Display names for plots (use article titles, not B1/B2/B3).
ARTIFACT_A = "ModernPDE (Artifact A)"
MUZEEL_TITLE = (
    "Muzeel: JS Dead Code Elimination on Mobile Web (IMC 2022)"
)
DIE_TITLE = "Dead Iteration Elimination (IMPACT 2025)"
AUTOJMH_TITLE = (
    "AutoJMH: Prevent DCE & Constant Folding (ASE 2016)"
)

CLASS_COLORS = {
    DIRECT: "#2d6a4f",
    PARTIAL: "#40916c",
    NOT_DIRECT: "#1b4965",
    PAPER_ONLY: "#b23a48",
    CODE_ONLY: "#bc6c25",
}
CLASS_LABELS = [
    (DIRECT, "Direct"),
    (PARTIAL, "Partial"),
    (NOT_DIRECT, "Not direct"),
    (PAPER_ONLY, "Paper-only"),
    (CODE_ONLY, "Code-only"),
]


def log(msg: str) -> None:
    print(msg, flush=True)


def ensure_dirs() -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    PLOTS.mkdir(parents=True, exist_ok=True)


def write_csv(path: Path, fieldnames: list[str], rows: list[dict]) -> None:
    with path.open("w", encoding="utf-8", newline="") as fh:
        w = csv.DictWriter(fh, fieldnames=fieldnames, extrasaction="ignore")
        w.writeheader()
        for row in rows:
            w.writerow({k: row.get(k, "N/A") for k in fieldnames})
    log(f"  wrote {path.relative_to(ROOT)} ({len(rows)} rows)")


def na(v) -> str:
    if v is None or v == "":
        return "N/A"
    return str(v)


def pdf_page_count(path: Path) -> str:
    if not path.exists():
        return "N/A"
    try:
        import pdfplumber

        with pdfplumber.open(path) as pdf:
            return str(len(pdf.pages))
    except Exception as exc:  # noqa: BLE001
        log(f"  warn: page count failed for {path.name}: {exc}")
        return "N/A"


def compute_artifact_a() -> dict:
    loc_code = loc_comm = loc_blank = 0
    by_ext: Counter[str] = Counter()
    cpp = headers = py = test_cpp = samples = 0
    classes = templates = includes = 0
    cc_by_file: dict[str, int] = {}

    for path in ROOT.rglob("*"):
        if not path.is_file():
            continue
        if any(p in SKIP_DIRS for p in path.parts):
            continue
        if path.suffix.lower() in SKIP_SUFFIX:
            continue
        rel = path.as_posix()
        try:
            text = path.read_text(encoding="utf-8", errors="replace")
        except OSError:
            continue

        by_ext[path.suffix or "(none)"] += 1
        in_block = False
        for raw in text.splitlines():
            line = raw.strip()
            if not line:
                loc_blank += 1
                continue
            if in_block:
                loc_comm += 1
                if "*/" in line:
                    in_block = False
                continue
            if line.startswith("/*"):
                loc_comm += 1
                if "*/" not in line:
                    in_block = True
                continue
            if line.startswith("//"):
                loc_comm += 1
                continue
            loc_code += 1

        if path.suffix in {".cpp", ".c", ".cc", ".cxx", ".h", ".hpp"}:
            includes += sum(
                1 for ln in text.splitlines() if ln.strip().startswith("#include")
            )
            for ln in text.splitlines():
                if CLASS_RE.search(ln):
                    classes += 1
                if TEMPLATE_RE.search(ln):
                    templates += 1

        if path.suffix in {".cpp", ".c", ".cc", ".cxx"}:
            cpp += 1
            if "tests/" in rel and "/samples/" not in rel:
                test_cpp += 1
            if "/samples/" in rel:
                samples += 1
            cc_by_file[rel] = len(DECISION_RE.findall(text)) + 1
        elif path.suffix in {".h", ".hpp"}:
            headers += 1
        elif path.suffix == ".py":
            py += 1

    cmake = (ROOT / "tests" / "CMakeLists.txt").read_text(
        encoding="utf-8", errors="replace"
    )
    always_tests = len(re.findall(r"modernpde_add_test\(\s*(\w+)", cmake))
    optional = 0
    if "cjson_pde_test" in cmake:
        optional += 1
    if "tinyexpr_pde_test" in cmake:
        optional += 1

    scale = {
        "challenge_500_median_wall_ms": "N/A",
        "challenge_5000_median_wall_ms": "N/A",
        "challenge_50000_median_wall_ms": "N/A",
        "scalability_runs": "N/A",
        "scalability_source": "N/A",
    }
    scale_csv = ROOT / "benchmarks" / "results" / "scalability_summary.csv"
    if scale_csv.exists():
        with scale_csv.open(encoding="utf-8", newline="") as fh:
            for row in csv.DictReader(fh):
                name = row.get("benchmark", "")
                med = row.get("median_wall_ms", "N/A")
                runs = row.get("runs", "N/A")
                if name == "challenge_500":
                    scale["challenge_500_median_wall_ms"] = med
                elif name == "challenge_5000":
                    scale["challenge_5000_median_wall_ms"] = med
                elif name == "challenge_50000":
                    scale["challenge_50000_median_wall_ms"] = med
                scale["scalability_runs"] = runs
        scale["scalability_source"] = str(scale_csv.relative_to(ROOT))

    top_cc = sorted(cc_by_file.items(), key=lambda x: -x[1])[:8]

    return {
        "loc_code": loc_code,
        "loc_comment": loc_comm,
        "loc_blank": loc_blank,
        "cpp_files": cpp,
        "headers": headers,
        "python_files": py,
        "test_cpp_files": test_cpp,
        "sample_cpp_files": samples,
        "include_directives": includes,
        "class_struct_approx": classes,
        "template_approx": templates,
        "ctest_always_on": always_tests,
        "ctest_optional_hooks": optional,
        "coverage_tooling": "none",
        "cmake_cxx_standard": "17",
        "core_dependencies": "none (optional cJSON/tinyexpr for tests)",
        "algorithms": (
            "CHA, VCA, CallGraph, PDE, CFG, DU, PathEnum, CFGPDE, "
            "SCCP, Faint, FieldSensitive, Phase8 caches"
        ),
        "top_cc_proxy": "; ".join(f"{k}={v}" for k, v in top_cc),
        **scale,
        "files_by_ext": dict(by_ext),
    }


def write_a_metrics_csv(a: dict) -> None:
    rows = []
    for k, v in a.items():
        if k == "files_by_ext":
            continue
        rows.append(
            {
                "metric_name": k,
                "value": na(v),
                "unit": "",
                "origin": "code-derived"
                if "wall_ms" not in k and "scalability" not in k
                else "prior-measured-csv",
                "notes": "host/compiler for prior CSV not recorded in file"
                if "wall_ms" in k or k.startswith("scalability")
                else "",
            }
        )
    write_csv(
        OUT / "A_code_metrics.csv",
        ["metric_name", "value", "unit", "origin", "notes"],
        rows,
    )


def _paper_rows(paper: str, rows: list[tuple]) -> list[dict]:
    return [
        {
            "paper": paper,
            "metric_name": n,
            "value": v,
            "unit": u,
            "cite": c,
            "origin": "paper-derived",
        }
        for n, v, u, c in rows
    ]


def extract_b1() -> list[dict]:
    pages = pdf_page_count(PDF_B1)
    rows = [
        ("title", "Muzeel: Assessing the Impact of JavaScript Dead Code Elimination on Mobile Web Performance", "", "p.1"),
        ("authors", "Kupoluyi; Chaqfeh; Varvello; Coke; Hashmi; Subramanian; Zaki", "", "p.1"),
        ("year_venue", "2022 / ACM IMC'22", "", "p.1 DOI 10.1145/3517745.3561427"),
        ("pages", pages, "", "pdfplumber"),
        ("domain", "JavaScript dead functions on mobile web pages", "", "abstract"),
        ("method", "Black-box dynamic analysis + post-load user-event emulation; eliminate unused JS functions", "", "§4"),
        ("baseline", "Lacuna (stops at onLoad); Lighthouse guidance", "", "§2"),
        ("corpus_pages", "15000", "pages", "Alexa landing pages; §6.1.1"),
        ("corpus_js_files", "300000", "files", "§6 / abstract"),
        ("crawl_hw", "64 cores, 1 TB RAM", "", "§6.1.1"),
        ("client_phones", "Redmi Go; Samsung J3; Samsung S10", "", "Table 1 p.7"),
        ("client_ram_gb", "1; 2; 8", "GB", "Table 1 p.7"),
        ("browsers", "Chrome; Firefox; Edge; Brave", "", "§6.1.2"),
        ("networks", "100 Mbps; 3G; LTE; LTE+", "", "§6.1.2"),
        ("runs_per_metric", "5", "loads", "median of 5; §6.3"),
        ("unused_functions_median_pct", "70", "%", "abstract / contrib"),
        ("unused_filesize_median_pct", "55", "%", "abstract"),
        ("elim_funcs_per_file_median", "67", "functions", "§6.2.1"),
        ("elim_bytes_per_file_median_kb", "10", "KB", "§6.2.1"),
        ("elim_funcs_per_page_mean", "2000", "functions", "§6.2.1"),
        ("muzeel_under_5min_pct", "85", "%", "≤6 cores; §6.2"),
        ("median_runtime_64core_s", "30", "s", "§6.2"),
        ("plt_speedup_pct_low", "25", "%", "abstract"),
        ("plt_speedup_pct_high", "30", "%", "abstract"),
        ("max_3g_speedup_s", "9", "s", "intro"),
        ("median_size_reduction_kb", "400", "KB", "intro"),
        ("similarity_ge90_pct_of_pages", "90", "%", "§6.2.2 structural/functional"),
        ("lacuna_structural_ge90_pct", "60", "%", "§6.2.2 same 200-page set"),
        ("perf_eval_pages", "200", "pages", "§6.1.1"),
        ("compiler_flags", "N/A", "", "browser/JS; not a C++ compile study"),
        ("opt_level", "N/A", "", "N/A"),
        ("statistical_method", "median of 5 loads; CDFs/figures", "", "§6"),
    ]
    return _paper_rows("B1_muzeel", rows)


def extract_b2() -> list[dict]:
    pages = pdf_page_count(PDF_B2)
    rows = [
        ("title", "Dead Iteration Elimination", "", "p.1 / HAL title"),
        ("authors", "Bastoul; Schmitt; Meister; Reddy", "", "p.2"),
        ("year_venue", "2025 / IMPACT 2025 Barcelona", "", "HAL header"),
        ("pages", pages, "", "pdfplumber"),
        ("domain", "Polyhedral removal of dead loop iterations (AI/DL fused ops)", "", "abstract"),
        ("method", "6-step polyhedral DIE: raise→DDG→invert→dead-space analysis→restrict→codegen", "", "§2 Fig.3"),
        ("algorithm_steps", "6", "", "§2"),
        ("baseline", "SSA DCE; partial DCE (Knoop); Polly/PPCG/AlphaZ (qualitative)", "", "§3"),
        ("benchmark_tables", "0", "", "no numeric eval table in PDF"),
        ("llama_prompt_time_reduction_pct", "3", "%", "LLaMa 3.1 8B last-decoder/logits; §2/conclusion"),
        ("hardware", "N/A", "", "not reported"),
        ("os", "N/A", "", "not reported"),
        ("compiler_flags", "N/A", "", "not reported"),
        ("opt_level", "N/A", "", "not reported"),
        ("run_count", "N/A", "", "not reported"),
        ("statistical_method", "N/A", "", "anecdotal application note only"),
    ]
    return _paper_rows("B2_die", rows)


def extract_b3() -> list[dict]:
    pages = pdf_page_count(PDF_B3)
    rows = [
        ("title", "Automatic Microbenchmark Generation to Prevent Dead Code Elimination and Constant Folding", "", "p.1"),
        ("authors", "Rodriguez-Cancio; Combemale; Baudry", "", "p.1"),
        ("year_venue", "2016 / ASE'16 Singapore", "", "footer ASE'16"),
        ("pages", pages, "", "pdfplumber"),
        ("domain", "Java JMH payload generation preventing DCE and constant folding", "", "abstract"),
        ("method", "Static slice + sink maximization + anti-CF field rules + test-derived init + regression tests", "", "§2–3"),
        ("projects", "5", "", "Math; Vectorz; Lang; JSyn; Img2"),
        ("total_loops_abstract", "6028", "loops", "abstract / RQ1 prose"),
        ("total_loops_table2", "6082", "loops", "Table 2 total; disagrees with abstract"),
        ("payloads_generated", "4705", "", "Table 2"),
        ("payloads_generated_pct", "77", "%", "Table 2 of 6082"),
        ("payloads_initialized", "3485", "", "Table 2"),
        ("microbenchmarks_generated", "3462", "", "Table 2"),
        ("rejected", "1377", "", "Table 2 preconditions"),
        ("rejected_pct", "23", "%", "Table 2"),
        ("unsupported_variables", "1027", "", "Table 2"),
        ("unsupported_invocations", "350", "", "Table 2"),
        ("regression_failures", "23", "", "Table 2; 0.4%"),
        ("expert_match_autojmh", "23/23", "", "Table 3; CI overlap α=0.05"),
        ("expert_match_dce_off", "0/23", "", "Table 3 sink maximization off"),
        ("expert_match_cfcp_inverted", "11/23", "", "Table 3"),
        ("expert_match_bad_init", "3/23", "", "Table 3"),
        ("sort_linkedlist_sorted_ns", "203", "ns", "Table 1"),
        ("sort_linkedlist_unsorted_ns", "453", "ns", "Table 1"),
        ("sort_vector_sorted_ns", "1639", "ns", "Table 1"),
        ("sort_vector_unsorted_ns", "645", "ns", "Table 1"),
        ("jmh_invocations", "30", "VM invocations", "§4.2.2 Georges methodology"),
        ("jmh_warmup_measure", "10 warmup + 10 measure (stated)", "", "§4.2.2"),
        ("confidence_level", "0.05", "", "§4.2.2"),
        ("rq3_engineers", "6", "", "§4.3"),
        ("rq3_suas", "5", "", "§4.3"),
        ("hardware", "N/A", "", "not fully specified in PDF extract"),
        ("compiler_flags", "N/A", "", "HotSpot/JMH; flags not tabulated"),
        ("opt_level", "N/A", "", "JIT; not a fixed -O level study"),
    ]
    return _paper_rows("B3_autojmh", rows)


def cmp_row(
    metric: str,
    paper_value: str,
    paper_unit: str,
    paper_cite: str,
    a_value: str,
    a_unit: str,
    a_origin: str,
    classification: str,
    rationale: str,
) -> dict:
    return {
        "metric": metric,
        "paper_value": na(paper_value),
        "paper_unit": na(paper_unit),
        "paper_cite": na(paper_cite),
        "artifact_a_value": na(a_value),
        "artifact_a_unit": na(a_unit),
        "artifact_a_origin": na(a_origin),
        "classification": classification,
        "rationale": rationale,
    }


def comparison_b1(a: dict) -> list[dict]:
    rows = []
    rows.append(
        cmp_row(
            "dead_code_granularity",
            "unused JS functions (dynamic, post-interaction)",
            "",
            "§4",
            "variables by path usage ratio (static PDE)",
            "",
            "code",
            PARTIAL,
            "Both identify unused computation, but JS functions≠C++ variables; dynamic browser≠static analyzer.",
        )
    )
    rows.append(
        cmp_row(
            "unused_functions_median_pct",
            "70",
            "%",
            "abstract",
            "N/A",
            "",
            "",
            PAPER_ONLY,
            "A does not measure % unused JS functions on web corpora.",
        )
    )
    rows.append(
        cmp_row(
            "plt_speedup_pct",
            "25–30",
            "%",
            "abstract",
            "N/A",
            "",
            "",
            PAPER_ONLY,
            "Page-load time on Android/browsers; A reports analyzer wall time, not PLT.",
        )
    )
    rows.append(
        cmp_row(
            "median_size_reduction_kb",
            "400",
            "KB",
            "intro",
            "N/A",
            "",
            "",
            PAPER_ONLY,
            "Transferred JS bytes; A has no page-size metric.",
        )
    )
    rows.append(
        cmp_row(
            "structural_similarity_ge90",
            "90% of 200 pages",
            "",
            "§6.2.2",
            "N/A",
            "",
            "",
            PAPER_ONLY,
            "Visual/functional page similarity vs Lacuna; not classifier accuracy.",
        )
    )
    rows.append(
        cmp_row(
            "accuracy_named_metric",
            "page similarity / Lacuna comparison",
            "",
            "§6.2.2",
            "PDE Accuracy/Precision/Recall/F1 on synthetic labels (when main/challenges run)",
            "",
            "code+tests",
            NOT_DIRECT,
            "Same English word 'accuracy' but different definitions, units, and inputs.",
        )
    )
    rows.append(
        cmp_row(
            "runtime_of_tool",
            "median ~30 s/page on 64 cores; 85% <5 min on ≤6 cores",
            "s",
            "§6.2",
            a.get("challenge_50000_median_wall_ms", "N/A"),
            "ms",
            "prior-measured-csv"
            if a.get("challenge_50000_median_wall_ms") != "N/A"
            else "N/A",
            NOT_DIRECT,
            "Muzeel: crawl+emulate web pages on 64c server. A: synthetic PDE challenge wall ms on different HW (host N/A in CSV).",
        )
    )
    rows.append(
        cmp_row(
            "corpus_scale",
            "15000 pages / 300000 JS files",
            "",
            "§6.1",
            f"{a['sample_cpp_files']} samples; challenges 500/5000/50000 vars",
            "",
            "code",
            NOT_DIRECT,
            "Web corpus ≠ synthetic variable counts.",
        )
    )
    rows.append(
        cmp_row(
            "hardware_match",
            "64c/1TB server + Android phones Table 1",
            "",
            "§6.1 / Table 1",
            "N/A (not recorded for prior CSV; WSL g++ available locally)",
            "",
            "",
            NOT_DIRECT,
            "No shared HW profile → no direct speedup comparison.",
        )
    )
    rows.append(
        cmp_row(
            "loc_code",
            "N/A",
            "",
            "",
            str(a["loc_code"]),
            "lines",
            "code-derived",
            CODE_ONLY,
            "Paper does not publish Muzeel LOC in the inspected PDF.",
        )
    )
    rows.append(
        cmp_row(
            "ctest_targets",
            "N/A",
            "",
            "",
            str(a["ctest_always_on"]),
            "targets",
            "code-derived",
            CODE_ONLY,
            "A regression suite; paper evaluates mobile web metrics.",
        )
    )
    rows.append(
        cmp_row(
            "sccp_constant_propagation",
            "N/A (JS browser DCE)",
            "",
            "",
            "SCCP implemented (Unknown/Constant/Overdefined)",
            "",
            "code",
            CODE_ONLY,
            "B1 does not study sparse conditional constant propagation.",
        )
    )
    return rows


def comparison_b2(a: dict) -> list[dict]:
    rows = []
    rows.append(
        cmp_row(
            "dead_code_granularity",
            "dead loop iterations (polyhedral iteration space)",
            "",
            "abstract / §2",
            "variables by path usage ratio; statement-level IR faint/SCCP",
            "",
            "code",
            PARTIAL,
            "Both refine classical DCE; B2 acts on iterations, A on variables/IR ops.",
        )
    )
    rows.append(
        cmp_row(
            "technique_family",
            "polyhedral image/preimage on DDG",
            "",
            "Algorithm 1",
            "CFG/DU/path enumeration + worklist dataflow",
            "",
            "code",
            PARTIAL,
            "Shared dataflow-over-dependence idea; different math and IR.",
        )
    )
    rows.append(
        cmp_row(
            "partial_dead_code_literature",
            "cites Knoop partial DCE; proposes iteration-level alternative",
            "",
            "§3",
            "PDE path-ratio classification (MOSTLY/PARTIALLY DEAD bands)",
            "",
            "code",
            PARTIAL,
            "Conceptual kinship with partial deadness; algorithms and targets differ.",
        )
    )
    rows.append(
        cmp_row(
            "llama_prompt_time_reduction_pct",
            "3",
            "%",
            "§2 / conclusion",
            "N/A",
            "",
            "",
            PAPER_ONLY,
            "A has no LLaMa/transformer operator pipeline or DIE codegen.",
        )
    )
    rows.append(
        cmp_row(
            "benchmark_suite",
            "N/A (no numeric evaluation tables)",
            "",
            "PDF 4 pages",
            "challenge_500/5000/50000; samples; optional cJSON",
            "",
            "code",
            NOT_DIRECT,
            "Paper is technique-focused; A has runnable synthetic benches — not matching inputs.",
        )
    )
    rows.append(
        cmp_row(
            "polyhedral_codegen",
            "yes (step f)",
            "",
            "§2",
            "no",
            "",
            "code",
            PAPER_ONLY,
            "Missing capability in A.",
        )
    )
    rows.append(
        cmp_row(
            "desired_output_specification",
            "yes (specialization/sparsity/subsampling)",
            "",
            "§2",
            "no",
            "",
            "code",
            PAPER_ONLY,
            "Missing capability in A.",
        )
    )
    rows.append(
        cmp_row(
            "cha_vca_callgraph",
            "N/A",
            "",
            "",
            "implemented",
            "",
            "code",
            CODE_ONLY,
            "OO call-structure analyses not part of DIE paper.",
        )
    )
    rows.append(
        cmp_row(
            "loc_code",
            "N/A",
            "",
            "",
            str(a["loc_code"]),
            "lines",
            "code-derived",
            CODE_ONLY,
            "Paper does not report implementation LOC.",
        )
    )
    rows.append(
        cmp_row(
            "hardware_match",
            "N/A",
            "",
            "not reported",
            "N/A",
            "",
            "",
            NOT_DIRECT,
            "Cannot validate any runtime comparison.",
        )
    )
    return rows


def comparison_b3(a: dict) -> list[dict]:
    rows = []
    rows.append(
        cmp_row(
            "relationship_to_dce",
            "PREVENT DCE so timed code actually executes",
            "",
            "§2.1 / abstract",
            "DETECT/classify dead or faint computation (PDE, faint, SCCP dead)",
            "",
            "code",
            PARTIAL,
            "Opposite goals around DCE: B3 keeps code alive for measurement; A finds deadness.",
        )
    )
    rows.append(
        cmp_row(
            "relationship_to_constant_folding",
            "PREVENT CF/CP via field declaration / non-literal init rules",
            "",
            "§2 / Table 3 CF/CP row",
            "PERFORM SCCP constant propagation on IR",
            "",
            "code",
            PARTIAL,
            "B3 defeats CF; A implements CF-like SCCP. Related topic, opposite intent.",
        )
    )
    rows.append(
        cmp_row(
            "ensures_code_executed",
            "sink maximization + regression tests + JMH harness",
            "",
            "§3–4",
            "N/A as microbenchmark generator",
            "",
            "code",
            PAPER_ONLY,
            "A is not a JMH payload generator; no sink-maximization harness.",
        )
    )
    rows.append(
        cmp_row(
            "microbenchmarks_generated",
            "3462 / 6082 loops (Table 2; abstract says 6028)",
            "",
            "Table 2 / abstract",
            "N/A",
            "",
            "",
            PAPER_ONLY,
            "A does not extract Java loops into JMH payloads.",
        )
    )
    rows.append(
        cmp_row(
            "expert_time_similarity",
            "23/23 AutoJMH; 0/23 if DCE not prevented",
            "",
            "Table 3",
            "N/A",
            "",
            "",
            PAPER_ONLY,
            "Statistical overlap of JMH ns distributions — not A's PDE F1.",
        )
    )
    rows.append(
        cmp_row(
            "execution_time_ns_sort",
            "LinkedList 203–453 ns; Vector 645–1639 ns",
            "ns",
            "Table 1",
            a.get("challenge_50000_median_wall_ms", "N/A"),
            "ms",
            "prior-measured-csv"
            if a.get("challenge_50000_median_wall_ms") != "N/A"
            else "N/A",
            NOT_DIRECT,
            "JMH steady-state ns for Collections.sort ≠ analyzer wall ms on synthetic PDE.",
        )
    )
    rows.append(
        cmp_row(
            "input_generation",
            "values observed from covering unit tests; anti-random for RQ2",
            "",
            "§3 / §4.2",
            "handwritten samples + synthetic challenge generators",
            "",
            "code",
            NOT_DIRECT,
            "Different languages and goals; both care about realistic inputs but not comparable protocols.",
        )
    )
    rows.append(
        cmp_row(
            "statistical_methodology",
            "Georges et al.; 30 VM inv; CI α=0.05",
            "",
            "§4.2.2",
            "scalability script: 3 warmups + 20 runs (median/mean/stddev) when used",
            "",
            "code",
            NOT_DIRECT,
            "Both use repeated runs, but JMH/CI-overlap ≠ wall-ms summary on C++ challenges.",
        )
    )
    rows.append(
        cmp_row(
            "sccp_vs_anti_cf",
            "anti-CF rules required for valid micros (11/23 if inverted)",
            "",
            "Table 3",
            "SCCP present",
            "",
            "code",
            PARTIAL,
            "Evidence that CF matters on both sides; still not a numeric head-to-head.",
        )
    )
    rows.append(
        cmp_row(
            "java_ast_slicing",
            "yes",
            "",
            "§3",
            "C-like lexer/parser/CFG builder (not Java AST slice)",
            "",
            "code",
            NOT_DIRECT,
            "Front-ends differ.",
        )
    )
    rows.append(
        cmp_row(
            "loc_code",
            "N/A",
            "",
            "",
            str(a["loc_code"]),
            "lines",
            "code-derived",
            CODE_ONLY,
            "AutoJMH LOC not in inspected PDF tables.",
        )
    )
    rows.append(
        cmp_row(
            "pde_path_classification",
            "N/A",
            "",
            "",
            "DEAD/MOSTLY DEAD/PARTIALLY DEAD/MOSTLY LIVE/LIVE",
            "",
            "code",
            CODE_ONLY,
            "A advantage area; not AutoJMH's task.",
        )
    )
    return rows


CMP_FIELDS = [
    "metric",
    "paper_value",
    "paper_unit",
    "paper_cite",
    "artifact_a_value",
    "artifact_a_unit",
    "artifact_a_origin",
    "classification",
    "rationale",
]


def make_plots(a: dict) -> None:
    import matplotlib.pyplot as plt
    import numpy as np

    plt.rcParams.update(
        {
            "font.family": "DejaVu Sans",
            "font.size": 11,
            "axes.spines.top": False,
            "axes.spines.right": False,
            "figure.dpi": 150,
        }
    )

    b1_rows = comparison_b1(a)
    b2_rows = comparison_b2(a)
    b3_rows = comparison_b3(a)

    def save(fig, stem: str) -> None:
        fig.savefig(PLOTS / f"{stem}.png", bbox_inches="tight")
        fig.savefig(PLOTS / f"{stem}.pdf", bbox_inches="tight")
        plt.close(fig)
        log(f"  plot {stem}.png + .pdf")

    def comparability_counts(rows: list[dict]) -> Counter:
        return Counter(r["classification"] for r in rows)

    def plot_comparability_panel(
        rows: list[dict],
        paper_title: str,
        stem: str,
    ) -> None:
        """Per-paper: how many metrics fall in each comparability class vs ModernPDE."""
        counts = comparability_counts(rows)
        labels = [short for _, short in CLASS_LABELS]
        vals = [counts[full] for full, _ in CLASS_LABELS]
        colors = [CLASS_COLORS[full] for full, _ in CLASS_LABELS]

        fig, ax = plt.subplots(figsize=(8, 4.5))
        y = np.arange(len(labels))
        ax.barh(y, vals, color=colors, edgecolor="white", linewidth=0.5)
        ax.set_yticks(y)
        ax.set_yticklabels(labels)
        ax.set_xlabel("Number of compared metrics")
        ax.set_title(
            f"ModernPDE vs {paper_title}\n"
            "metric comparability (independent analysis)"
        )
        ax.invert_yaxis()
        for i, v in enumerate(vals):
            if v:
                ax.text(v + 0.05, i, str(v), va="center", fontsize=9)
        fig.tight_layout()
        save(fig, stem)

    # --- Artifact A only ---
    fig, ax = plt.subplots(figsize=(7, 4))
    labels = ["code", "comment", "blank"]
    vals = [a["loc_code"], a["loc_comment"], a["loc_blank"]]
    ax.bar(labels, vals, color="#3b6ea5")
    ax.set_ylabel("Lines")
    ax.set_title(f"{ARTIFACT_A} — physical LOC breakdown")
    for i, v in enumerate(vals):
        ax.text(i, v, str(v), ha="center", va="bottom", fontsize=9)
    fig.tight_layout()
    save(fig, "code_size")

    fig, ax = plt.subplots(figsize=(7, 4))
    flabels = [".cpp", "headers", ".py", "tests", "samples"]
    fvals = [
        a["cpp_files"],
        a["headers"],
        a["python_files"],
        a["test_cpp_files"],
        a["sample_cpp_files"],
    ]
    ax.bar(flabels, fvals, color="#5a7d4f")
    ax.set_ylabel("Count")
    ax.set_title(f"{ARTIFACT_A} — file counts")
    fig.tight_layout()
    save(fig, "code_file_counts")

    ms = [
        a.get("challenge_500_median_wall_ms"),
        a.get("challenge_5000_median_wall_ms"),
        a.get("challenge_50000_median_wall_ms"),
    ]
    if all(x not in (None, "N/A") for x in ms):
        fig, ax = plt.subplots(figsize=(7, 4))
        xs = ["500", "5000", "50000"]
        ys = [float(x) for x in ms]
        ax.plot(xs, ys, marker="o", color="#3b6ea5", linewidth=2)
        ax.set_xlabel("Synthetic variables (challenge suite)")
        ax.set_ylabel("Median wall time (ms)")
        ax.set_title(
            f"{ARTIFACT_A} — PDE challenge scalability\n"
            "(prior run; host/compiler not recorded in CSV)"
        )
        fig.tight_layout()
        save(fig, "a_scalability_wall_ms")

    # --- ModernPDE snapshot vs each paper (code-only side; not numeric contest) ---
    fig, ax = plt.subplots(figsize=(7, 4))
    ax.bar(
        ["CTest targets", "Sample inputs", "LOC (code)"],
        [a["ctest_always_on"], a["sample_cpp_files"], a["loc_code"] / 1000],
        color="#3b6ea5",
    )
    ax.set_ylabel("Count (LOC in thousands)")
    ax.set_title(f"{ARTIFACT_A} — evaluation footprint")
    fig.tight_layout()
    save(fig, "modernpde_evaluation_footprint")

    # --- Muzeel (IMC 2022) paper metrics ---
    fig, ax = plt.subplots(figsize=(8, 4.5))
    m_labels = [
        "unused fn\nmedian %",
        "unused size\nmedian %",
        "PLT speedup\nlow %",
        "PLT speedup\nhigh %",
        "pages sim≥90%",
    ]
    m_vals = [70, 55, 25, 30, 90]
    ax.bar(m_labels, m_vals, color="#b23a48")
    ax.set_ylabel("Percent (paper-reported)")
    ax.set_title(
        f"{MUZEEL_TITLE}\n"
        "selected evaluation metrics (paper-only; not vs ModernPDE times)"
    )
    fig.tight_layout()
    save(fig, "compare_muzeel_paper_metrics")

    fig, ax = plt.subplots(figsize=(7, 4))
    ax.bar(
        ["Alexa pages", "JS files (÷1000)", "perf eval pages"],
        [15000, 300, 200],
        color="#b23a48",
    )
    ax.set_ylabel("Count (JS files shown in thousands)")
    ax.set_title(f"{MUZEEL_TITLE}\n corpus scale")
    fig.tight_layout()
    save(fig, "compare_muzeel_corpus_scale")

    fig, ax = plt.subplots(figsize=(6, 4))
    ax.bar(
        ["Muzeel-ed pages", "Lacuna (baseline)"],
        [90, 60],
        color=["#b23a48", "#888888"],
    )
    ax.set_ylabel("% of 200 pages with structural similarity ≥ 90%")
    ax.set_title(
        f"{MUZEEL_TITLE}\n"
        "structural similarity vs Lacuna (paper-reported)"
    )
    ax.set_ylim(0, 100)
    fig.tight_layout()
    save(fig, "compare_muzeel_similarity_vs_lacuna")

    fig, ax = plt.subplots(figsize=(7, 4))
    ax.bar(
        ["Elim. funcs/file\n(median)", "Elim. KB/file\n(median)", "Page size\nreduction KB"],
        [67, 10, 400],
        color="#b23a48",
    )
    ax.set_ylabel("Paper-reported value")
    ax.set_title(f"{MUZEEL_TITLE}\n dead-code elimination savings")
    fig.tight_layout()
    save(fig, "compare_muzeel_elimination_savings")

    plot_comparability_panel(
        b1_rows,
        MUZEEL_TITLE,
        "compare_modernpde_vs_muzeel_comparability",
    )

    # --- DIE (IMPACT 2025) ---
    fig, ax = plt.subplots(figsize=(6, 4))
    ax.bar(["LLaMa 3.1 8B\nprompt processing"], [3], color="#d98c5f")
    ax.set_ylabel("Time reduction (%)")
    ax.set_title(
        f"{DIE_TITLE}\n"
        "application note: last-decoder iteration removal (paper-only)"
    )
    ax.set_ylim(0, 10)
    fig.tight_layout()
    save(fig, "compare_die_llama_time_reduction")

    plot_comparability_panel(
        b2_rows,
        DIE_TITLE,
        "compare_modernpde_vs_die_comparability",
    )

    # --- AutoJMH (ASE 2016) ---
    fig, ax = plt.subplots(figsize=(8, 4.5))
    t2_labels = ["loops\n(Table 2)", "payloads", "initialized", "microbenches", "rejected"]
    t2_vals = [6082, 4705, 3485, 3462, 1377]
    ax.bar(t2_labels, t2_vals, color="#6b5b95")
    ax.set_ylabel("Count (Table 2)")
    ax.set_title(
        f"{AUTOJMH_TITLE}\n"
        "automatic extraction reach (abstract: 6028 loops)"
    )
    fig.tight_layout()
    save(fig, "compare_autojmh_extraction_reach")

    fig, ax = plt.subplots(figsize=(7, 4))
    ax.bar(
        ["Full AutoJMH", "DCE not prevented", "CF/CP rules inverted", "Bad initialization"],
        [23, 0, 11, 3],
        color="#6b5b95",
    )
    ax.set_ylabel("Expert-similar microbenchmarks / 23")
    ax.set_title(
        f"{AUTOJMH_TITLE}\n"
        "Table 3 — similarity to handwritten JMH (paper-reported)"
    )
    ax.set_ylim(0, 25)
    fig.tight_layout()
    save(fig, "compare_autojmh_expert_similarity")

    fig, ax = plt.subplots(figsize=(7, 4))
    x = np.arange(2)
    w = 0.35
    ax.bar(x - w / 2, [203, 453], w, label="LinkedList", color="#6b5b95")
    ax.bar(x + w / 2, [1639, 645], w, label="Vector", color="#9b8ec4")
    ax.set_xticks(x)
    ax.set_xticklabels(["Sorted input", "Unsorted input"])
    ax.set_ylabel("Time (ns)")
    ax.set_title(
        f"{AUTOJMH_TITLE}\n"
        "Table 1 — Collections.sort (paper-only; JMH ns)"
    )
    ax.legend()
    fig.tight_layout()
    save(fig, "compare_autojmh_collections_sort_table1")

    fig, ax = plt.subplots(figsize=(7, 4))
    ax.bar(
        ["Unsupported\nvariables", "Unsupported\ninvocations", "Regression\nfailures"],
        [1027, 350, 23],
        color="#6b5b95",
    )
    ax.set_ylabel("Loops rejected (Table 2)")
    ax.set_title(f"{AUTOJMH_TITLE}\n rejection reasons (total corpus)")
    fig.tight_layout()
    save(fig, "compare_autojmh_rejection_reasons")

    plot_comparability_panel(
        b3_rows,
        AUTOJMH_TITLE,
        "compare_modernpde_vs_autojmh_comparability",
    )

    # --- Conceptual: each paper's role vs dead code (qualitative, not numeric) ---
    fig, ax = plt.subplots(figsize=(9, 4.5))
    roles = [
        ("Muzeel\n(IMC'22)", "Eliminate unused\nJS functions", 3),
        ("DIE\n(IMPACT'25)", "Remove dead\nloop iterations", 3),
        ("AutoJMH\n(ASE'16)", "Prevent DCE/CF\nin microbenchmarks", 3),
        (ARTIFACT_A.split()[0], "Classify dead/partial\nvariables + SCCP", 3),
    ]
    names = [r[0] for r in roles]
    scores = [r[2] for r in roles]
    ax.barh(names, scores, color=["#b23a48", "#d98c5f", "#6b5b95", "#3b6ea5"])
    ax.set_xlabel("Primary focus (qualitative scale 1–3)")
    ax.set_title(
        "Conceptual comparison — role in dead-code / optimization space\n"
        "(not a numeric benchmark score)"
    )
    for i, (_, desc, _) in enumerate(roles):
        ax.text(3.05, i, desc, va="center", fontsize=8)
    ax.set_xlim(0, 5.5)
    fig.tight_layout()
    save(fig, "compare_conceptual_dce_roles")

    # --- Overview: three independent comparability profiles ---
    c1 = comparability_counts(b1_rows)
    c2 = comparability_counts(b2_rows)
    c3 = comparability_counts(b3_rows)
    cats = [full for full, _ in CLASS_LABELS]
    short = [s for _, s in CLASS_LABELS]
    x = np.arange(len(cats))
    w = 0.25
    fig, ax = plt.subplots(figsize=(11, 5))
    ax.bar(
        x - w,
        [c1[c] for c in cats],
        w,
        label="Muzeel (IMC'22)",
        color="#b23a48",
    )
    ax.bar(
        x,
        [c2[c] for c in cats],
        w,
        label="Dead Iteration Elimination (IMPACT'25)",
        color="#d98c5f",
    )
    ax.bar(
        x + w,
        [c3[c] for c in cats],
        w,
        label="AutoJMH (ASE'16)",
        color="#6b5b95",
    )
    ax.set_xticks(x)
    ax.set_xticklabels(short, rotation=15, ha="right")
    ax.set_ylabel("# metrics in comparison tables")
    ax.set_title(
        f"ModernPDE vs three reference papers — comparability overview\n"
        "(each paper analyzed independently)"
    )
    ax.legend(loc="upper right", fontsize=9)
    fig.tight_layout()
    save(fig, "compare_overview_three_papers")

    # --- Three-panel comparability (one subplot per article) ---
    fig, axes = plt.subplots(1, 3, figsize=(12, 4.5), sharey=True)
    panels = [
        (b1_rows, "Muzeel\n(IMC'22)", "#b23a48"),
        (b2_rows, "Dead Iteration\nElimination", "#d98c5f"),
        (b3_rows, "AutoJMH\n(ASE'16)", "#6b5b95"),
    ]
    for ax, (rows, subtitle, color) in zip(axes, panels):
        counts = comparability_counts(rows)
        vals = [counts[full] for full, _ in CLASS_LABELS]
        ax.bar([s for _, s in CLASS_LABELS], vals, color=color, alpha=0.85)
        ax.set_title(subtitle, fontsize=10)
        ax.set_xlabel("Comparability class")
        ax.tick_params(axis="x", rotation=35)
    axes[0].set_ylabel("# metrics")
    fig.suptitle(
        f"{ARTIFACT_A} — per-article metric comparability",
        fontsize=12,
        fontweight="bold",
    )
    fig.tight_layout()
    save(fig, "compare_comparability_panels")

    # Legacy filenames (retitled) for backward compatibility
    fig, ax = plt.subplots(figsize=(8, 4.5))
    ax.bar(m_labels, m_vals, color="#b23a48")
    ax.set_ylabel("Percent (paper-reported)")
    ax.set_title(f"{MUZEEL_TITLE}\nselected paper metrics")
    fig.tight_layout()
    save(fig, "benchmark_comparison_3517745_paper_only")

    fig, ax = plt.subplots(figsize=(5, 4))
    ax.bar(["LLaMa prompt time"], [3], color="#d98c5f")
    ax.set_ylabel("Percent (paper-reported)")
    ax.set_title(f"{DIE_TITLE}\nnumeric claim in PDF")
    ax.set_ylim(0, 10)
    fig.tight_layout()
    save(fig, "execution_time_vs_die_paper_only")

    fig, ax = plt.subplots(figsize=(8, 4.5))
    ax.bar(t2_labels, t2_vals, color="#6b5b95")
    ax.set_ylabel("Count (Table 2)")
    ax.set_title(f"{AUTOJMH_TITLE}\nTable 2 reach")
    fig.tight_layout()
    save(fig, "autojmh_table2_reach")

    fig, ax = plt.subplots(figsize=(7, 4))
    ax.bar(["Full AutoJMH", "DCE off", "CF/CP bad", "Bad init"], [23, 0, 11, 3], color="#6b5b95")
    ax.set_ylabel("Successful tests / 23")
    ax.set_title(f"{AUTOJMH_TITLE}\nTable 3 vs experts")
    ax.set_ylim(0, 25)
    fig.tight_layout()
    save(fig, "autojmh_table3_expert_match")

    fig, ax = plt.subplots(figsize=(11, 5))
    ax.bar(x - w, [c1[c] for c in cats], w, label="Muzeel (IMC'22)", color="#b23a48")
    ax.bar(x, [c2[c] for c in cats], w, label="DIE (IMPACT'25)", color="#d98c5f")
    ax.bar(x + w, [c3[c] for c in cats], w, label="AutoJMH (ASE'16)", color="#6b5b95")
    ax.set_xticks(x)
    ax.set_xticklabels(short, rotation=15, ha="right")
    ax.set_ylabel("# metrics")
    ax.set_title("ModernPDE vs reference papers — comparability summary")
    ax.legend()
    fig.tight_layout()
    save(fig, "summary")


def write_report(a: dict) -> None:
    b1 = comparison_b1(a)
    b2 = comparison_b2(a)
    b3 = comparison_b3(a)

    def fmt_table(rows: list[dict]) -> str:
        lines = [
            "| metric | paper | A | class |",
            "| --- | --- | --- | --- |",
        ]
        for r in rows:
            pv = r["paper_value"]
            if r["paper_unit"] and r["paper_unit"] != "N/A":
                pv = f"{pv} {r['paper_unit']}"
            av = r["artifact_a_value"]
            if r["artifact_a_unit"] and r["artifact_a_unit"] != "N/A":
                av = f"{av} {r['artifact_a_unit']}"
            lines.append(
                f"| {r['metric']} | {pv} | {av} | {r['classification']} |"
            )
        return "\n".join(lines)

    body = f"""# Artifact A vs reference papers — comparison report

Generated by `scripts/compare_artifact_vs_papers.py`.
Analyses of B1, B2, and B3 are **independent**. Identical metric names are not treated as comparable without matching units, inputs, hardware, and methodology.
Missing data is **N/A** (never invented).

## Artifact A snapshot

| metric | value | origin |
| --- | --- | --- |
| LOC code / comment / blank | {a['loc_code']} / {a['loc_comment']} / {a['loc_blank']} | code-derived |
| .cpp / headers / .py | {a['cpp_files']} / {a['headers']} / {a['python_files']} | code-derived |
| CTest always-on targets | {a['ctest_always_on']} | code-derived |
| Samples | {a['sample_cpp_files']} | code-derived |
| C++ standard | {a['cmake_cxx_standard']} | CMakeLists.txt |
| Coverage tooling | {a['coverage_tooling']} | none found |
| Algorithms | {a['algorithms']} | code |
| Prior challenge_50000 median wall | {a.get('challenge_50000_median_wall_ms', 'N/A')} ms | {a.get('scalability_source', 'N/A')} (host N/A in CSV) |

PDF paths: `docs/papers/muzeel.pdf` (B1), `die.pdf` (B2), `autojmh.pdf` (B3).

---

## A vs B1 (Muzeel, IMC'22)

### Similarities
- Both address **dead / unused computation** and argue that removing it can improve performance.
- Both evaluate (in their own domains) whether elimination preserves observable behavior (B1: page similarity; A: synthetic label agreement / tests).

### Differences
- B1: **dynamic** JS analysis in browsers with **user-event emulation** after load; eliminates **functions**; metrics are **page load / SI / FCP / KB / CPU on phones**.
- A: **static** C++ pipeline (CHA→…→PDE→CFG/SCCP); classifies **variables**; metrics are **analyzer correctness/time** on samples and synthetic challenges.

### Comparable metrics
{fmt_table(b1)}

### Benchmark results
**No directly comparable numeric benchmark.** Muzeel PLT speedups (25–30%, up to ~9 s on 3G) and A's challenge wall-ms use incompatible inputs and hardware. Do not score A against Muzeel's % speedup.

### Missing capabilities (in B1, absent from A)
- Web crawl of 15k pages / 300k JS files
- Browser automation and post-load interaction emulation
- Mobile PLT / Speed Index / FCP / bandwidth measurement
- Lacuna-style head-to-head on live pages

### Strengths of A (evidence-based)
- Multi-phase static analyzer with CFG/DU/paths, PDE spectrum, SCCP, faint, field-sensitive, Phase 8 caches
- Automated CTest suite (`{a['ctest_always_on']}` always-on targets)
- Zero core third-party deps for the main binary

### Strengths of B1
- Real-world mobile web corpus and end-user metrics
- Demonstrated size reduction (~400 KB median) and similarity retention (~90% of pages ≥90%)
- Public system evaluated across phones, browsers, and networks

### Methodology limits
Comparing "accuracy" or "speedup" by name alone is invalid. B1 accuracy is page similarity; A's F1 is synthetic PDE labels.

---

## A vs B2 (Dead Iteration Elimination, IMPACT 2025)

### Similarities
- Both refine classical **dead-code elimination** beyond "delete whole unused statements only."
- B2 cites **partial DCE**; A implements **path-ratio partial deadness** for variables — conceptual kinship only.

### Differences
- B2: **polyhedral** dead **iteration spaces**, optional desired-output specs, AI/DL operator fusion (MatMul+BandPart).
- A: general C++ teaching/demo analyzer; **no** polyhedral codegen, no iteration-space restriction API.

### Comparable metrics
{fmt_table(b2)}

### Benchmark results
Paper reports essentially **one** numeric application claim (~**3%** LLaMa 3.1 8B prompt-processing reduction). **Not comparable** to A's challenge wall times (different problem, no shared HW/protocol). Machine tables in PDF: **0**.

### Missing capabilities (in B2, absent from A)
- Polyhedral raising / DDG inversion / iteration-domain restriction / polyhedral codegen
- Desired output data-space specification (sparsity/subsampling/specialization)
- Integration with TF-like operator graphs / LLM decoder specialization

### Strengths of A
- Runnable end-to-end static pipeline with tests and sample inputs
- Broader OO analyses (CHA, VCA, call graph) not claimed by DIE

### Strengths of B2
- Precise formulation of iteration-level deadness complementary to statement DCE
- Clear 6-step algorithm (Algorithm 1) and application hook for transformers

### Methodology limits
4-page workshop paper with almost no experimental tables — cannot validate runtime claims beyond the single LLaMa percentage as paper-stated.

---

## A vs B3 (AutoJMH, ASE'16)

### Similarities
- Shared vocabulary: **DCE** and **constant folding/propagation**.
- Both care whether optimized-away code distorts results (B3: timing; A: classification of dead/faint/constant values).

### Differences (critical)
- B3 **prevents** DCE/CF so microbenchmarks measure real work (sink maximization, field rules, test-derived inputs).
- A **detects** deadness and **runs SCCP** (performs constant propagation).
- B3 targets **Java/JMH ns**; A targets **C++ static analysis ms** and labels.

### Extra focus — DCE/CF prevention & executed code
- Table 3: full AutoJMH **23/23** expert-similar; **0/23** when DCE not prevented; **11/23** when CF/CP rules inverted; **3/23** with bad init.
- That is strong evidence that preventing elimination/folding is necessary for valid micros — orthogonal to A's goal of finding dead code.
- A has **no** JMH payload generator, no sink maximization, no anti-CF declaration rewriting.

### Comparable metrics
{fmt_table(b3)}

### Benchmark results
**No directly comparable times.** Table 1 sort ns and A's challenge ms must not be plotted against each other as a contest. Table 2 note: abstract loops **6028** vs table total **6082** — both preserved (do not merge).

### Missing capabilities (in B3, absent from A)
- Automatic Java AST slicing into JMH payloads
- Sink maximization / anti-CF field policy / reset strategies for steady state
- Georges-style CI overlap testing against expert micros

### Strengths of A
- Static PDE path classification and multi-phase analyses B3 does not provide
- C++ CFG/DU/SCCP stack with CTest

### Strengths of B3
- Empirically shows prevention features are necessary (Table 3 ablation)
- Large reach study (thousands of loops) and expert/engineer studies

### Methodology limits
Opposite optimization goals around DCE/CF make "who is better at DCE" a malformed question.

---

## Overall comparison

| Question | Answer |
| --- | --- |
| Most relevant paper to A's *topic word* "dead code" | **B1 (Muzeel)** for elimination impact; **B2** for compiler-style deadness refinement; **B3** for DCE/CF as measurement hazard |
| Closest *methodology* to A | **B2** (static compiler analysis of dead computation) — still polyhedral≠CFG/PDE |
| Strongest *numeric* paper evaluation | **B1** (large mobile study) and **B3** (Tables 2–3); **B2** weakest empirically |
| Strongest *benchmark comparison with A* | **None are directly comparable**; closest *concept* pairing is A-PDE vs B2-DIE / partial DCE literature |
| Metrics A already handles well | LOC/structure; CTest; synthetic PDE Acc/P/R/F1 (when run); analyzer wall time; Phase 8 cache stats |
| Metrics A does not measure yet | Web PLT/SI/KB; polyhedral iteration deadness; JMH ns; statistical CI vs expert micros; coverage % |
| What to add next (suggestions only) | (1) Document HW/compiler/flags beside every CSV; (2) optional real C/C++ corpus timing beyond cJSON; (3) explicit SCCP vs anti-CF discussion in docs; (4) do **not** chase Muzeel PLT numbers inside A |

### Plots produced
See `output/plots/` for PNG and PDF pairs. Titles use **article names** (Muzeel IMC'22, Dead Iteration Elimination IMPACT'25, AutoJMH ASE'16), not B1/B2/B3.

Key comparison charts:
- `compare_modernpde_vs_muzeel_comparability` / `compare_modernpde_vs_die_comparability` / `compare_modernpde_vs_autojmh_comparability`
- `compare_muzeel_paper_metrics`, `compare_muzeel_corpus_scale`, `compare_muzeel_similarity_vs_lacuna`, `compare_muzeel_elimination_savings`
- `compare_die_llama_time_reduction`
- `compare_autojmh_extraction_reach`, `compare_autojmh_expert_similarity`, `compare_autojmh_collections_sort_table1`, `compare_autojmh_rejection_reasons`
- `compare_conceptual_dce_roles`, `compare_overview_three_papers`, `compare_comparability_panels`, `summary`

Paper-only metrics are labeled as such — no fake ModernPDE-vs-paper runtime overlays.

### CSV outputs
- `output/paper-comparison/A_code_metrics.csv`
- `output/paper-comparison/B1_muzeel_extract.csv` / `B1_muzeel_comparison.csv`
- `output/paper-comparison/B2_die_extract.csv` / `B2_die_comparison.csv`
- `output/paper-comparison/B3_autojmh_extract.csv` / `B3_autojmh_comparison.csv`
"""
    REPORT.write_text(body, encoding="utf-8")
    log(f"  wrote {REPORT.relative_to(ROOT)}")


def main() -> int:
    log("=== Step 2–4: Artifact A vs B1 / B2 / B3 (independent) ===")
    ensure_dirs()

    for label, path in (("B1", PDF_B1), ("B2", PDF_B2), ("B3", PDF_B3)):
        log(f"PDF {label}: {path} exists={path.exists()}")

    log("\n[1/5] Computing Artifact A metrics...")
    a = compute_artifact_a()
    write_a_metrics_csv(a)
    log(f"  LOC code={a['loc_code']} cpp={a['cpp_files']} ctest={a['ctest_always_on']}")

    log("\n[2/5] Extracting B1 (Muzeel) — independent...")
    b1e = extract_b1()
    write_csv(
        OUT / "B1_muzeel_extract.csv",
        ["paper", "metric_name", "value", "unit", "cite", "origin"],
        b1e,
    )
    write_csv(OUT / "B1_muzeel_comparison.csv", CMP_FIELDS, comparison_b1(a))

    log("\n[3/5] Extracting B2 (DIE) — independent...")
    b2e = extract_b2()
    write_csv(
        OUT / "B2_die_extract.csv",
        ["paper", "metric_name", "value", "unit", "cite", "origin"],
        b2e,
    )
    write_csv(OUT / "B2_die_comparison.csv", CMP_FIELDS, comparison_b2(a))

    log("\n[4/5] Extracting B3 (AutoJMH) — independent...")
    b3e = extract_b3()
    write_csv(
        OUT / "B3_autojmh_extract.csv",
        ["paper", "metric_name", "value", "unit", "cite", "origin"],
        b3e,
    )
    write_csv(OUT / "B3_autojmh_comparison.csv", CMP_FIELDS, comparison_b3(a))

    log("\n[5/5] Plots + final report...")
    make_plots(a)
    write_report(a)

    log("\nDone. No commits performed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())

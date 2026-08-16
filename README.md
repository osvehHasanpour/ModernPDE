<div align="center">

# ModernPDE

### Artifact A — A self-contained C++17 static-analysis pipeline

[![C++17](https://img.shields.io/badge/standard-C%2B%2B17-0B6E4F?style=for-the-badge)](./CMakeLists.txt)
[![CMake](https://img.shields.io/badge/build-CMake%203.16%2B-1B4965?style=for-the-badge)](./CMakeLists.txt)
[![Deps](https://img.shields.io/badge/core%20deps-none-C9A227?style=for-the-badge)](./CMakeLists.txt)
[![CI](https://img.shields.io/badge/CI-CTest-5C4B51?style=for-the-badge)](./.github/workflows/ci.yml)

</div>

<p align="center">
  <em>Class hierarchy → virtual calls → call graph → path-aware PDE → CFG / DU / paths → SCCP &amp; faint analysis → Phase&nbsp;8 caches</em>
</p>

---

## Artifact overview

**ModernPDE** is a zero-dependency static-analysis engine written in modern C++. It is intended as a readable research / teaching artifact: every major pass is implemented in-tree, without LLVM or libclang.

| Item | Detail |
|------|--------|
| **Artifact ID** | Artifact A — ModernPDE |
| **Language** | C++17 |
| **Build** | CMake ≥ 3.16 |
| **Core libraries** | None (optional sibling cJSON / tinyexpr for extra tests only) |
| **Primary binaries** | `ModernPDE`, `ModernPDE_Phase8` |
| **Validation** | CMake/CTest (regression, CFG, challenges, Phase 8) |

> **Scope note.** Synthetic challenge suites can report perfect Accuracy / F1 because labels are constructed to match the classifier. Those scores are **not** claims of equivalence to industrial DCE (Muzeel, LLVM, JMH, etc.). See `output/comparison-report.md` for a paper-by-paper comparison.

---

## What this artifact does

The default binary runs a staged pipeline on a built-in subject program (and can also analyze sample sources under `tests/samples/`):

| Stage | Module | Role |
|------:|--------|------|
| 1 | Class Hierarchy (CHA) | Inheritance queries |
| 2 | Virtual Call Analysis | Resolve dispatch targets via CHA |
| 3 | Call Graph | Context-sensitive reachability & recursion |
| 4 | PDE | Path-ratio dead / partial / live labels |
| 5 | CFG · DU · Paths | Blocks, def–use chains, path enumeration |
| 5.4 | CFG × PDE | Path-precise deadness |
| 5.5–5.7 | Metrics & scale | Labeled check, Acc/P/R/F1, timing |
| 6–7 | Field-sensitive · SCCP · Faint | Heap fields, constants, faint vars |
| 8 | Phase 8 | CFG/DU caches, lazy paths, report |

**PDE label bands**

| Label | Usage ratio | Meaning |
|-------|-------------|---------|
| DEAD | 0% | Defined, never used |
| MOSTLY DEAD | &lt; 25% | Rarely used |
| PARTIALLY DEAD | 25–74% | Used on some paths only |
| MOSTLY LIVE | 75–99% | Used on most paths |
| LIVE | 100% | Used on every path |

---

## Repository layout

```
ModernPDE/
├── include/                 Public headers
├── src/                     Analyzer + mains
├── tests/                   CTest sources
│   └── samples/             Input programs for lexer/CFG demos
├── scripts/                 Build, run, plot, paper comparison
├── benchmarks/              Scalability / cJSON runners
├── docs/                    Guides & reports
│   └── papers/              Reference PDFs (Muzeel, DIE, AutoJMH)
├── output/                  Generated runs, plots, comparison CSVs
├── Dockerfile               Ubuntu 24.04 Release image
└── .github/workflows/ci.yml Configure → build → ctest
```

---

## Requirements

- CMake **3.16+**
- A C++17 compiler (GCC, Clang, or MSVC)
- Optional: Docker; Python 3 + venv for paper-comparison plots

Verified on WSL (Ubuntu) with g++ 13 and CMake 3.28.

---

## Build

```bash
git clone https://github.com/osvehHasanpour/ModernPDE.git
cd ModernPDE

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

Binaries land in `build/` (`ModernPDE`, `ModernPDE_Phase8`) and under `build/tests/` for CTest targets.

---

## Run

**Full demo pipeline**

```bash
./build/ModernPDE
```

**Analyze a sample**

```bash
./build/ModernPDE tests/samples/cfg.cpp
```

**Phase 8 report** (default path `output/phase8_benchmark.txt`)

```bash
mkdir -p output
./build/ModernPDE_Phase8
```

**Test suite**

```bash
cd build && ctest --output-on-failure
# or from repo root:
./scripts/run_tests.sh
```

**Logged full run** (writes under `output/runs/<timestamp>/`)

```bash
./scripts/run_all.sh   # expects ./build/ModernPDE
```

**Docker**

```bash
docker compose run --rm analyze
docker compose run --rm test
```

---

## Evaluation & reproducibility

| Claim type | How to check | Notes |
|------------|--------------|-------|
| Build & unit/regression | `ctest` in CI and locally | Always-on targets + optional cJSON/tinyexpr |
| Sample front-end / CFG | `ModernPDE tests/samples/*.cpp` | 14 sample inputs |
| Synthetic PDE scale | `challenge_500` / `_5000` / `_50000` | Labels are synthetic |
| Phase 8 caches | `ModernPDE_Phase8`, `phase8_test` | Text report under `output/` |
| Paper comparison (optional) | `scripts/compare_artifact_vs_papers.py` | Independent B1/B2/B3 extracts; see `output/comparison-report.md` |

Reference papers (read-only inputs) live in `docs/papers/`:

- **B1** Muzeel (IMC ’22) — JS dead-function elimination on mobile web  
- **B2** Dead Iteration Elimination (IMPACT 2025) — polyhedral dead iterations  
- **B3** AutoJMH (ASE ’16) — microbenchmarks that *prevent* DCE / constant folding  

Numeric head-to-heads with those papers are **not** claimed; methodology and units differ. The comparison script records that explicitly.

---

## Design principles

1. **Readable passes** — each analysis is a normal C++ module, not a compiler-plugin opaque blob.  
2. **No core deps** — the main library links only the standard library.  
3. **Testable** — CTest targets share the same sources as the product binaries.  
4. **Honest metrics** — perfect F1 on synthetic challenges is expected; real-world DCE claims require different corpora and protocols.

---

## Documentation map

| Path | Contents |
|------|----------|
| `docs/PHASE8_GUIDE.md` | Phase 8 usage |
| `docs/PHASE8_REPORT.md` | Phase 8 write-up |
| `docs/Test-Report.md` | Test inventory |
| `output/comparison-report.md` | Artifact A vs B1/B2/B3 |
| `output/README.md` | Output directory layout |

---

## License / citation

Use this repository as **Artifact A (ModernPDE)** in reports and comparisons. Prefer citing the GitHub revision you evaluated and, when discussing papers, the DOIs / venues listed under `docs/papers/`.

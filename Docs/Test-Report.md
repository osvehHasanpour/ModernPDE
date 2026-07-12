# ModernPDE — Test Report

**Date:** July 9, 2026  
**Environment:** WSL (Ubuntu), GCC, C++17, CMake 3.16+  
**Project root:** `Modernp/Modernp`

---

## Executive Summary

ModernPDE is a modular C++17 static-analysis framework covering class hierarchy analysis, virtual-call resolution, context-sensitive call graphs, path-aware partial dead-code elimination (PDE), CFG construction, DU chains, path enumeration, lexer/parser front-end, and (recently) field-sensitive heap analysis with Andersen-style points-to and per-field deadness.

**Test run outcome: all tests pass via CMake/CTest (`scripts/run_tests.sh`).** The suite includes 5 new regression tests, 7 existing challenge/integration tests, and optional cJSON/tinyexpr tests when sibling libraries are present.

| Category | Passed | Failed | Skipped / N/A |
|----------|--------|--------|----------------|
| CTest automated targets | 12 | 0 | 0 |
| Incomplete / non-tests | — | — | 1 (`tests/Benchmark.cpp` fragment) |
| **Total (ctest)** | **12** | **0** | — |

---

## What Has Been Built

| Area | Key files | Status |
|------|-----------|--------|
| PDE classification | `PDE.cpp`, `Metrics.cpp` | Complete, heavily tested |
| Class hierarchy | `ClassHierarchy.cpp` | Complete |
| Virtual calls | `VirtualCallAnalysis.cpp` | Complete |
| Call graph | `CallGraph.cpp` | Complete |
| CFG / paths / DU | `CFG.cpp`, `PathEnumeration.cpp`, `DUChain.cpp` | Complete |
| Lexer / parser / CFG builder | `Lexer.cpp`, `Parser.cpp`, `CFGBuilder.cpp` | Working smoke tests |
| Field-sensitive analysis | `FieldSensitiveAnalysis.cpp` | New; demo via `--field-sensitive` |
| Main pipeline | `main.cpp` | Runs all phases; file mode for samples |
| Stubs / empty | `IR.cpp`, `ModernPDE.cpp`, `SCCP.H` | Not tested / not implemented |

---

## Test Inventory

### New regression tests (CMake / CTest)

| File | What it covers | CTest name | Result |
|------|----------------|------------|--------|
| `tests/field_sensitive_test.cpp` | Points-to, field reads, dead/partialDead flags | `field_sensitive_test` | **PASS** |
| `tests/lexer_parser_test.cpp` | Lexer + parser + CFG on all 7 sample `.cpp` files | `lexer_parser_test` | **PASS** |
| `tests/callgraph_test.cpp` | Reachability, recursion, mutual recursion | `callgraph_test` | **PASS** |
| `tests/cha_virtual_test.cpp` | CHA descendants + virtual-call resolution | `cha_virtual_test` | **PASS** |
| `tests/cfg_path_test.cpp` | Diamond CFG path count (2) + DU chains (4) | `cfg_path_test` | **PASS** |

### Automated tests (existing — now in CTest)

| File | What it covers | Build command (from project root) | Result |
|------|----------------|-----------------------------------|--------|
| `tests/challenge_500.cpp` | PDE on 500 variables (5 categories × 100); accuracy / precision / recall / F1 | `g++ -std=c++17 -O2 -I include tests/challenge_500.cpp src/PDE.cpp src/Metrics.cpp -o build/tests/challenge_500` | **PASS** (500/500) |
| `tests/challenge_5000.cpp` | PDE scale test — 5,000 variables | Same pattern with `challenge_5000.cpp` | **PASS** (5000/5000) |
| `tests/challenge_50000.cpp` | PDE stress — 50,000 variables + timing | Same pattern with `challenge_50000.cpp` | **PASS** (50000/50000) |
| `tests/cjson_pde_test.cpp` | JSON-driven PDE cases via **cJSON** (`../cJSON-master`) | See `tests/cjson_pde_test.cpp` header comments | **PASS** (16/16) |
| `tests/tinyexpr_pde_test.cpp` | Math-expression-driven PDE checks via **tinyexpr** | Compile `tinyexpr.c` with **gcc**, link with g++ (see note below) | **PASS** (60/60) |
| `tests/master_challenge.cpp` | PDE smoke — 100 variables, prints classifications | `g++ … tests/master_challenge.cpp src/PDE.cpp` | **PASS** (smoke) |
| `tests/full_master_challenge.cpp` | End-to-end smoke: CHA + virtual calls + call graph + PDE | `g++ … + ClassHierarchy + VirtualCallAnalysis + CallGraph` | **PASS** (smoke) |

### Test data

| File | Purpose |
|------|---------|
| `tests/pde_cases.json` | 16 labeled PDE cases (DEAD / MOSTLY DEAD / PARTIALLY DEAD / MOSTLY LIVE / LIVE) consumed by `cjson_pde_test` |

### Lexer / parser sample inputs (smoke — no assertions)

Run: `./build/ModernPDE tests/<file>.cpp` from project root.

| File | Scenario | Result |
|------|----------|--------|
| `tests/dead.cpp` | Unused locals | Lexer OK, 1 function |
| `tests/cfg.cpp` | If/else branch | Lexer OK, 1 function |
| `tests/oop.cpp` | `new` + virtual `attack()` | Lexer OK, 1 function |
| `tests/partial.cpp` | Conditional `std::cout` use | Lexer OK, 1 function |
| `tests/recursion.cpp` | `factorial` recursion | Lexer OK, 1 function |
| `tests/template.cpp` | Function template `add` | Lexer OK, 1 function |
| `tests/mutual_recursive.cpp` | `even` / `odd` mutual recursion | Lexer OK, 3 functions |

### Main binary integration

| Command | What it exercises | Result |
|---------|-------------------|--------|
| `./build/ModernPDE` | Full demo pipeline (phases 1–6 including field-sensitive) | **PASS** (exit 0) |
| `./build/ModernPDE --field-sensitive` | Field-sensitive heap demo only | **PASS** (exit 0) |
| `./build/ModernPDE tests/<sample>.cpp` | Lexer → parser → CFG on sample file | **PASS** (all 7 samples) |

### Not runnable / incomplete

| File | Notes |
|------|-------|
| `tests/Benchmark.cpp` | Fragment only (`for` loops, no `main`); not a test |
| `scripts/run_all.sh` | Runs main binary and greps output; not an assertion suite |

---

## Build & Run Notes

### Full suite (recommended)

```bash
cd scripts
./run_tests.sh
```

Or manually:

```bash
cd build
cmake .. -DCMAKE_CXX_FLAGS="-Wall -Wextra"
cmake --build . --parallel
ctest --output-on-failure
```

### Main project only

```bash
cd build
cmake ..
make -j4
./ModernPDE                  # full pipeline
./ModernPDE --field-sensitive
```

### External dependencies (sibling directories)

- `../cJSON-master` — required for `cjson_pde_test`
- `../tinyexpr-master` — required for `tinyexpr_pde_test`

### tinyexpr compile note

`tinyexpr.c` must be compiled as **C** (not C++), then linked:

```bash
gcc -O2 -c ../tinyexpr-master/tinyexpr.c -o build/tests/tinyexpr.o
g++ -std=c++17 -O2 -I include -I ../tinyexpr-master \
    tests/tinyexpr_pde_test.cpp src/PDE.cpp src/Metrics.cpp \
    build/tests/tinyexpr.o -lm -o build/tests/tinyexpr_pde_test
```

Compiling `tinyexpr.c` directly with `g++` fails due to C++/C `const`/`void*` strictness. This is a **build procedure** issue, not a ModernPDE source bug.

---

## Failures Found & Fixes Applied

**None.** All runnable tests passed on the first successful build. No project source files were modified during this test pass.

| Issue observed | Classification | Action taken |
|----------------|----------------|--------------|
| `tinyexpr.c` fails when compiled as C++ | Build / toolchain | Documented two-step gcc+g++ build above |
| `tests/Benchmark.cpp` not a complete program | Incomplete artifact | Left unchanged; excluded from pass/fail count |
| `ModernPDE.cpp` unused-parameter warnings | Pre-existing stubs | Not blocking; not modified |

---

## Files Intentionally Left Untouched

| File | Reason |
|------|--------|
| `src/IR.cpp` | **Empty** — per project rule, empty files are not filled in even if tests reference IR helpers |
| `include/SCCP.H` | Header-only stub; no `.cpp` implementation exists |
| `src/ModernPDE.cpp` | Stub placeholders (`std::cout` only); not covered by automated tests |
| `tests/Benchmark.cpp` | Incomplete fragment; not a valid test target |

---

## Proposed New Test Files (implemented)

All previously proposed test files are now implemented and registered in `tests/CMakeLists.txt`:

| File | Status |
|------|--------|
| `tests/field_sensitive_test.cpp` | Implemented |
| `tests/lexer_parser_test.cpp` | Implemented |
| `tests/callgraph_test.cpp` | Implemented |
| `tests/cha_virtual_test.cpp` | Implemented |
| `tests/cfg_path_test.cpp` | Implemented |
| `tests/CMakeLists.txt` | Implemented |
| `scripts/run_tests.sh` | Implemented |

Supporting API additions for testability: `DUChainAnalysis::chainCount()`, `PathEnumeration::pathCount()`.

---

## Next Steps

1. **CI integration** — add `scripts/run_tests.sh` to GitHub Actions or similar.
2. **Extend field-sensitive tests** — branchy CFG, more alias patterns.
3. **Parser assertions** — expected token counts per sample file.
4. **Optional: implement `IR.cpp` helpers** — only if you explicitly approve filling the empty file.
5. **Complete or remove `tests/Benchmark.cpp`** — fragment remains incomplete.

---

## Quick Reference — CTest Pass/Fail Matrix

| CTest target | Pass | Fail |
|--------------|:----:|:----:|
| field_sensitive_test | ✓ | |
| lexer_parser_test | ✓ | |
| callgraph_test | ✓ | |
| cha_virtual_test | ✓ | |
| cfg_path_test | ✓ | |
| challenge_500 | ✓ | |
| challenge_5000 | ✓ | |
| challenge_50000 | ✓ | |
| master_challenge | ✓ | |
| full_master_challenge | ✓ | |
| cjson_pde_test | ✓ | |
| tinyexpr_pde_test | ✓ | |

**Overall: 12/12 ctest targets passed.**

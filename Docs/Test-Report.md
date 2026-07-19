# ModernPDE Test Report

This document describes the automated test suite for ModernPDE, including the **Phase 5 integration test** (`cfg_phase5_test`) and how to run everything on your machine.

**Last verified:** 13/13 tests passing (ctest).

---

## Quick Start

From the project root (WSL or Linux):

```bash
# One-shot: configure, build, run all tests
bash scripts/run_tests.sh
```

Manual steps:

```bash
mkdir -p build && cd build
cmake ..
cmake --build .
ctest --output-on-failure
```

**Working directory:** ctest runs with `WORKING_DIRECTORY` set to the project root, so paths like `tests/cfg.cpp` resolve correctly.

**Prerequisites:** CMake 3.16+, C++17 compiler (`g++` / `clang++`), `build-essential` on Debian/Ubuntu.

If `run_tests.sh` fails with `bash\r` on WSL, fix line endings once:

```bash
sed -i 's/\r$//' scripts/run_tests.sh
```

---

## Full Test Suite (13 targets)

| # | CTest name | Source | Phase | What it checks |
|---|------------|--------|-------|----------------|
| 1 | `field_sensitive_test` | `tests/field_sensitive_test.cpp` | 6 | Field-sensitive points-to and partial deadness on IR |
| 2 | `lexer_parser_test` | `tests/lexer_parser_test.cpp` | 1–5 | Lexer + parser + CFGBuilder on sample `.cpp` files |
| 3 | `callgraph_test` | `tests/callgraph_test.cpp` | 3 | Context-sensitive call graph, recursion |
| 4 | `cha_virtual_test` | `tests/cha_virtual_test.cpp` | 1–2 | Class hierarchy + virtual call resolution |
| 5 | `cfg_path_test` | `tests/cfg_path_test.cpp` | 5 | Manual diamond CFG: 2 paths, 4 DU chains |
| 6 | **`cfg_phase5_test`** | **`tests/cfg_phase5_test.cpp`** | **5** | **End-to-end AST → CFG → DU → paths → PDE** |
| 7 | `challenge_500` | `tests/challenge_500.cpp` | 4 | PDE stress (500 variables) |
| 8 | `challenge_5000` | `tests/challenge_5000.cpp` | 4 | PDE stress (5,000 variables) |
| 9 | `challenge_50000` | `tests/challenge_50000.cpp` | 4 | PDE stress (50,000 variables) |
| 10 | `master_challenge` | `tests/master_challenge.cpp` | 4 | PDE master challenge |
| 11 | `full_master_challenge` | `tests/full_master_challenge.cpp` | 1–4 | CHA + virtual calls + call graph + PDE |
| 12 | `cjson_pde_test` | `tests/cjson_pde_test.cpp` | 4 | PDE + cJSON (optional; needs `../cJSON-master`) |
| 13 | `tinyexpr_pde_test` | `tests/tinyexpr_pde_test.cpp` | 4 | PDE + tinyexpr (optional; needs `../tinyexpr-master`) |

Run a single test:

```bash
cd build
ctest -R cfg_phase5_test --output-on-failure
```

Or run the binary directly:

```bash
./build/tests/cfg_phase5_test
```

---

## Phase 5 Integration Test (`cfg_phase5_test`)

### Purpose

`cfg_phase5_test` validates the **complete Phase 5 pipeline** driven from real source files:

```
Lexer → Parser → AST → CFGBuilder → CFG
                              ↓
         CFGValidation + CFGStatistics (per function)
                              ↓
              DUChainAnalysis (reaching definitions)
                              ↓
              PathEnumeration (acyclic DFS)
                              ↓
              CFGPDE (path-sensitive classification)
```

Assertions are **structural** — minimum block counts, path counts, validation pass/fail, per-function entry/exit — not hardcoded CFG shapes or edge lists.

### Pipeline exercised per sample

For each input file the test:

1. Tokenizes and parses the file.
2. Builds a CFG with `CFGBuilder`.
3. Checks `cfg.size() >= minBlocks`.
4. Records function regions (`entryBlock`, `exitBlock`).
5. Enumerates paths from the global entry block.
6. Runs `CFGValidation::validate()` with per-function bounds.
7. Verifies each function has valid entry/exit blocks.
8. Computes `CFGStatistics::computePerFunction()`.
9. Builds DU chains via `DUChainAnalysis::buildFromCFG()`.
10. Runs `CFGPDE::analyzeFromCFG()`.

### Sample input files

| File | Control-flow feature | min blocks | min paths | DU chains expected |
|------|----------------------|------------|-----------|-------------------|
| `tests/cfg.cpp` | if / else | 5 | 1 | yes |
| `tests/cfg_while.cpp` | while loop | 5 | 1 | yes |
| `tests/cfg_for.cpp` | for loop | 6 | 1 | yes |
| `tests/cfg_nested_if.cpp` | nested if / else | 8 | 1 | yes |
| `tests/cfg_do_while.cpp` | do-while loop | 5 | 1 | yes |
| `tests/cfg_break_continue.cpp` | break / continue in while | 6 | 1 | yes |
| `tests/cfg_switch.cpp` | switch / case / break | 5 | 1 | yes |
| `tests/cfg_multi_return.cpp` | multiple returns, 2 functions | 5 | 1 | no* |
| `tests/recursion.cpp` | direct recursion | 5 | 1 | no* |
| `tests/mutual_recursive.cpp` | mutual recursion (3 functions) | 5 | 1 | no* |
| `tests/oop.cpp` | OOP sample (class-style source) | 3 | 1 | no* |

\*No local variable declarations in the parsed AST for these samples (parameters are not modeled as `Variable` defs yet), so DU chain count may be zero while the CFG/path pipeline still passes.

### Expected success output

```
================================================
  Phase 5 CFG Pipeline Test
================================================

[ tests/cfg.cpp ]
[ tests/cfg_while.cpp ]
...

================================================
  Total : <N>  Pass  : <N>  Fail  : 0
  RESULT : ALL TESTS PASSED
================================================
```

### Modules linked into `cfg_phase5_test`

Defined in `tests/CMakeLists.txt`:

- `Lexer.cpp`, `Parser.cpp`
- `CFGBuilder.cpp`, `CFG.cpp`
- `DUChain.cpp`, `PathEnumeration.cpp`
- `CFGValidation.cpp`, `CFGStatistics.cpp`
- `CFGPDE.cpp`, `PDE.cpp`

---

## Related Phase 5 Tests

### `cfg_path_test` (unit-level)

Builds a **manual diamond CFG** in code (not from AST):

- 4 basic blocks
- 2 acyclic paths from entry
- 4 DU chains via line-based `addDefinition` / `addUse`

Use this for quick regression on path enumeration and legacy DU API.

### `lexer_parser_test` (smoke)

Runs lexer + parser + CFGBuilder on:

- `tests/dead.cpp`, `tests/cfg.cpp`, `tests/oop.cpp`
- `tests/partial.cpp`, `tests/recursion.cpp`, `tests/template.cpp`
- `tests/mutual_recursive.cpp`

Checks function count and `cfg.size() > 0`.

---

## Proposed New Test Files (implemented)

---

## Troubleshooting

| Problem | Fix |
|---------|-----|
| `Lexer failed to tokenize file` | Run tests from project root, or use `ctest` (sets `WORKING_DIRECTORY`) |
| `bash\r: No such file` on WSL | `sed -i 's/\r$//' scripts/run_tests.sh` |
| cjson / tinyexpr tests missing | Optional; place libraries at `../cJSON-master` and `../tinyexpr-master` |
| CMake not found | `sudo apt install build-essential cmake` |

---

## Adding New Phase 5 Samples

1. Add a `.cpp` file under `tests/` (e.g. `tests/cfg_my_case.cpp`).
2. Add a row to the `cases[]` array in `tests/cfg_phase5_test.cpp` with sensible `minBlocks`, `minPaths`, and `expectDuChains`.
3. Rebuild and run:

   ```bash
   cd build && cmake --build . && ctest -R cfg_phase5_test --output-on-failure
   ```

Do **not** hardcode full CFG output — use structural bounds only.

---

## Test Count Summary

| Category | Count |
|----------|-------|
| Regression (Phase 1–6 unit/smoke) | 5 |
| Phase 5 integration | 1 (`cfg_phase5_test`) |
| PDE challenges | 5 |
| Optional external integration | 2 |
| **Total** | **13** |

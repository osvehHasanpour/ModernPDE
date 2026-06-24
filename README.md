<div align="center">

# ⚙️ ModernPDE

### A full-pipeline C++ static analysis engine — from class hierarchies to dead code, all in one run.

![C++17](https://img.shields.io/badge/C%2B%2B-17-blue?style=flat-square&logo=c%2B%2B)
![CMake](https://img.shields.io/badge/Build-CMake-red?style=flat-square)
![Zero Dependencies](https://img.shields.io/badge/Dependencies-Zero-brightgreen?style=flat-square)

</div>

---

ModernPDE is a modular, zero-dependency C++ framework that implements **seven distinct static analysis techniques** in a single pipeline. It models a game engine's class hierarchy as its subject program, then tears it apart phase by phase — resolving virtual calls, building call graphs, classifying dead code, tracing control flow, and benchmarking the whole thing at 50,000+ variables.

No LLVM. No libclang. Just clean, modern C++17 from scratch.

---

## The Pipeline

ModernPDE runs in sequential phases. Each one feeds context into the next.

```
[Source Program]
      │
      ▼
┌─────────────────────────────┐
│  Phase 1 · Class Hierarchy  │  Who inherits from whom?
└──────────────┬──────────────┘
               ▼
┌─────────────────────────────┐
│  Phase 2 · Virtual Calls    │  Which subclass handles Player::attack()?
└──────────────┬──────────────┘
               ▼
┌─────────────────────────────┐
│  Phase 3 · Call Graph       │  Context-sensitive. Detects recursion.
└──────────────┬──────────────┘
               ▼
┌─────────────────────────────┐
│  Phase 4 · PDE              │  Dead / Partially Dead / Live — per path.
└──────────────┬──────────────┘
               ▼
┌─────────────────────────────┐
│  Phase 5 · CFG + DU + Paths │  Basic blocks, def-use chains, path enum.
└──────────────┬──────────────┘
               ▼
┌─────────────────────────────┐
│  Phase 5.4 · CFG × PDE      │  Path-precise dead code.
└──────────────┬──────────────┘
               ▼
┌─────────────────────────────┐
│  Phase 5.5–5.7 · Eval       │  Accuracy, F1, benchmarks up to 50k vars.
└─────────────────────────────┘
```

---

## Phases in Detail

### 🧬 Phase 1 — Class Hierarchy Analysis

Builds a full inheritance tree from a game engine domain model and answers structural queries:

```
Entity
├── Character
│   ├── Player
│   │   ├── Warrior
│   │   ├── Mage
│   │   └── Archer
│   └── NPC
│       ├── Merchant
│       └── QuestGiver
├── Weapon
│   ├── Sword
│   └── Bow
└── Potion
    ├── HealthPotion
    └── ManaPotion
```

Queries supported: `getParent`, `getAllDescendants`, `getLeafClasses`.

---

### 📞 Phase 2 — Virtual Call Analysis

Resolves virtual dispatch targets using the class hierarchy. Ask `Player::attack()` and it returns every concrete subclass that actually implements it — exactly what a vtable would resolve at runtime.

```
Player::attack  →  Warrior, Mage, Archer
NPC::talk       →  Merchant, QuestGiver
Entity::use     →  Sword, Bow, HealthPotion, ManaPotion
```

---

### 🕸️ Phase 3 — Context-Sensitive Call Graph

Builds a call graph that distinguishes **who called whom and from what context**. Two calls to the same function from different contexts are tracked separately — a core precision upgrade over context-insensitive analysis.

Detects:
- **Reachability** — does `main` reach `uploadGPU` via `CTX_GAME`?
- **Direct recursion** — `factorial` calls itself
- **Mutual recursion** — `even ↔ odd`
- **DFS traversal** from any entry point in any context

Contexts modeled: `CTX_GAME`, `CTX_LEVEL`, `CTX_REC`, `CTX_MR`.

---

### 💀 Phase 4 — Partial Dead Code Elimination (PDE)

The heart of the project. Variables aren't just dead or alive — they exist on a spectrum based on how many execution paths actually use them.

| Classification    | Usage Ratio      | Meaning                              |
|-------------------|------------------|--------------------------------------|
| **DEAD**          | 0%               | Defined, never used anywhere         |
| **MOSTLY DEAD**   | < 25%            | Used on almost no paths              |
| **PARTIALLY DEAD**| 25% – 74%        | Used sometimes, skipped often        |
| **MOSTLY LIVE**   | 75% – 99%        | Used on most paths                   |
| **LIVE**          | 100%             | Used on every execution path         |

This is more nuanced than a typical liveness pass — a variable alive on one branch but dead on nine others is **not** the same as a variable that's fully live.

---

### 🗺️ Phase 5 — CFG · DU Chains · Path Enumeration

Three sub-analyses built on a shared Control Flow Graph:

**Control Flow Graph** — Basic blocks with explicit predecessor/successor edges. Models if/then/else/merge branching.

**DU Chains** — Tracks every Definition → Use relationship across line numbers. Finds the latest reaching definition for each use.

```
Def(x, line 1) -> Use(line 2)
Def(x, line 3) -> Use(line 4)
Def(y, line 5) -> Use(line 6)
Def(z, line 7) -> Use(line 10)
```

**Path Enumeration** — Enumerates all possible paths through the CFG from a given entry block. Used to feed precise path counts into the PDE engine.

---

### 🔗 Phase 5.4 — CFG × PDE Integration

Combines the CFG's path structure with the PDE classifier. Instead of counting uses globally, it asks: *on how many of the CFG's actual paths is this variable used?* This is path-precise dead code detection, not just use/def counting.

```
playerHealth  →  2 paths / 2 used  →  LIVE
bonusDamage   →  2 paths / 1 used  →  PARTIALLY DEAD
unusedTemp    →  2 paths / 0 used  →  DEAD
questReward   →  10 paths / 7 used →  MOSTLY LIVE
```

---

### 📊 Phase 5.5 — Accuracy Dataset

Runs the PDE classifier on a ground-truth dataset of **70 labeled variables** across all categories and compares against expected output.

```
Expected:   DEAD: 20  |  MOSTLY DEAD: 20  |  PARTIALLY DEAD: 15  |  LIVE: 15
```

---

### 📈 Phase 5.6 — Evaluation Metrics

Standard classifier metrics computed from the accuracy run:

```
Accuracy  : 1.00
Precision : 1.00
Recall    : 1.00
F1 Score  : 1.00
```

---

### ⚡ Phase 5.7 — Benchmark

Stress-tests the PDE engine from 100 to 50,000 variables using `std::chrono` high-resolution timing:

```
Variables: 100      Time: X ms
Variables: 1,000    Time: X ms
Variables: 5,000    Time: X ms
Variables: 10,000   Time: X ms
Variables: 50,000   Time: X ms
```

---

## Project Structure

```
Modernp/
├── src/
│   ├── main.cpp                  # Entry point — runs all phases in sequence
│   ├── ClassHierarchy.cpp        # Phase 1  · inheritance tree
│   ├── VirtualCallAnalysis.cpp   # Phase 2  · vtable-style dispatch resolution
│   ├── CallGraph.cpp             # Phase 3  · context-sensitive call graph + DFS
│   ├── PDE.cpp                   # Phase 4  · partial dead code classification
│   ├── CFG.cpp                   # Phase 5  · basic block CFG
│   ├── DUChain.cpp               # Phase 5  · definition-use chains
│   ├── PathEnumeration.cpp       # Phase 5  · CFG path enumeration
│   ├── CFGPDE.cpp                # Phase 5.4 · path-precise dead code
│   ├── Metrics.cpp               # Phase 5.6 · accuracy / precision / recall / F1
│   └── Benchmark.cpp             # Phase 5.7 · scalability timing
├── include/                      # Headers for all modules
├── results/                      # Auto-generated output files
├── build/                        # CMake build artifacts
└── scripts/
    └── run_all.sh                # One-shot: build, run, save all results
```

---

## Build & Run

**Requirements:** CMake 3.10+, any C++17 compiler (GCC, Clang, MSVC).

```bash
# Clone and build
git clone https://github.com/your-username/ModernPDE.git
cd ModernPDE/build
cmake ..
make

# Run the full pipeline
./ModernPDE
```

**Or use the script** to run everything and automatically save per-phase results:

```bash
cd scripts
./run_all.sh
```

Output files saved to `results/`:

| File                     | Contents                           |
|--------------------------|------------------------------------|
| `full_output.txt`        | Complete pipeline output           |
| `pde_result.txt`         | All variable classifications       |
| `cfg_result.txt`         | CFG block structure                |
| `duchain_result.txt`     | DU chain definitions               |
| `path_result.txt`        | Enumerated CFG paths               |
| `metrics.txt`            | Accuracy, Precision, Recall, F1    |
| `benchmark.txt`          | Timing results by variable count   |

---

## Concepts Covered

This project is a practical implementation of techniques taught in compiler design and program analysis courses:

- Class Hierarchy Analysis (CHA)
- Virtual method resolution & vtable simulation
- Context-sensitive call graph construction
- Worklist-based reachability (DFS)
- Partial Dead Code Elimination (PDE) with path-ratio classification
- Control Flow Graph (CFG) construction
- Definition-Use (DU) chain analysis
- Control-flow path enumeration
- CFG-precise dead code classification
- ML-style classifier evaluation (Accuracy, Precision, Recall, F1)

---

## Why ModernPDE?

Most static analysis frameworks are massive — LLVM, Soot, CodeQL. ModernPDE is the opposite: a **single self-contained binary**, no dependencies, no configuration files, no IR format to learn. Every algorithm is written from scratch so you can actually read it.

If you're learning program analysis, this is a working reference. If you're building something bigger, these modules are clean enough to lift directly.

---

## License

MIT — use it, fork it, learn from it.

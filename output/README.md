# Output directory

Generated artifacts only. Source lives in `src/`, `include/`, `tests/`, `scripts/`, and `docs/`.

- `latest/` — symlink to the most recent run
- `runs/` — timestamped analyzer logs (`YYYY-MM-DD_HH-MM-SS`)
- `benchmarks/` — CSV from `scripts/benchmark_compare.py`
- `reports/` — latest metrics JSON copies
- `archive/` — older run files
- `plots/` — figures (fig1–fig6 plus paper-comparison charts)
- `paper-comparison/` — extracted tables and summary vs Muzeel / DIE / AutoJMH

Default Phase 8 report path: `output/phase8_benchmark.txt`.

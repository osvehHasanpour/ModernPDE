#!/usr/bin/env python3

"""
ModernPDE - Real cJSON Source Benchmark

Runs the actual ModernPDE analyzer against every C source file
in the real cJSON source tree.

No expected classifications are hard-coded.

Collected from real executions:
  - source file
  - source LOC
  - wall-clock time
  - CPU usage
  - maximum RSS
  - exit code
  - stdout/stderr
  - success/failure

Results:
  benchmarks/results/cjson_real_raw.csv
  benchmarks/results/cjson_real_summary.csv
"""

from __future__ import annotations

import csv
import os
import re
import subprocess
import sys
import time
from pathlib import Path
from statistics import mean, median, stdev


ROOT = Path(__file__).resolve().parent.parent

# Real cJSON checkout
CJSON_ROOT = ROOT.parent / "cJSON-master"

# Release benchmark executable
EXECUTABLE_CANDIDATES = [
    ROOT / "build-benchmark" / "ModernPDE",
    ROOT / "build" / "ModernPDE",
]

RESULTS_DIR = ROOT / "benchmarks" / "results"
RAW_CSV = RESULTS_DIR / "cjson_real_raw.csv"
SUMMARY_CSV = RESULTS_DIR / "cjson_real_summary.csv"

# Number of measured executions per source file.
RUNS = 3


def find_executable() -> Path:
    for candidate in EXECUTABLE_CANDIDATES:
        if candidate.is_file() and os.access(candidate, os.X_OK):
            return candidate

    print("ERROR: ModernPDE executable not found.")
    print()
    print("Expected one of:")
    for candidate in EXECUTABLE_CANDIDATES:
        print(f"  {candidate}")
    print()
    print("Build first with:")
    print("  cmake -S . -B build-benchmark -DCMAKE_BUILD_TYPE=Release")
    print("  cmake --build build-benchmark -j$(nproc)")
    sys.exit(1)


def discover_c_files() -> list[Path]:
    if not CJSON_ROOT.is_dir():
        print(f"ERROR: cJSON source tree not found:")
        print(f"  {CJSON_ROOT}")
        sys.exit(1)

    files = sorted(
        p for p in CJSON_ROOT.rglob("*.c")
        if p.is_file()
    )

    if not files:
        print("ERROR: no .c files found in cJSON.")
        sys.exit(1)

    return files


def count_loc(path: Path) -> int:
    """
    Simple physical LOC:
    count non-empty lines.
    """
    try:
        text = path.read_text(
            encoding="utf-8",
            errors="replace",
        )
    except OSError:
        return 0

    return sum(
        1
        for line in text.splitlines()
        if line.strip()
    )


def run_one(executable: Path, source: Path) -> dict:
    """
    Execute the REAL ModernPDE binary against the REAL source file.

    /usr/bin/time -v is used so RSS and CPU are obtained from
    the operating system rather than guessed.
    """

    command = [
        "/usr/bin/time",
        "-v",
        str(executable),
        str(source),
    ]

    start = time.perf_counter()

    completed = subprocess.run(
        command,
        cwd=ROOT,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )

    elapsed_ms = (time.perf_counter() - start) * 1000.0

    stderr = completed.stderr

    cpu_percent = parse_cpu(stderr)
    max_rss_kb = parse_rss(stderr)
    user_seconds = parse_time_value(
        stderr,
        "User time (seconds)"
    )
    system_seconds = parse_time_value(
        stderr,
        "System time (seconds)"
    )

    return {
        "exit_code": completed.returncode,
        "wall_ms": elapsed_ms,
        "cpu_percent": cpu_percent,
        "max_rss_kb": max_rss_kb,
        "user_seconds": user_seconds,
        "system_seconds": system_seconds,
        "stdout": completed.stdout,
        "stderr": stderr,
    }


def parse_cpu(text: str) -> float:
    match = re.search(
        r"Percent of CPU this job got:\s*([0-9.]+)%",
        text,
    )

    if not match:
        return 0.0

    return float(match.group(1))


def parse_rss(text: str) -> int:
    match = re.search(
        r"Maximum resident set size \(kbytes\):\s*(\d+)",
        text,
    )

    if not match:
        return 0

    return int(match.group(1))


def parse_time_value(text: str, label: str) -> float:
    match = re.search(
        re.escape(label) + r":\s*([0-9.]+)",
        text,
    )

    if not match:
        return 0.0

    return float(match.group(1))


def relative_source(path: Path) -> str:
    try:
        return str(path.relative_to(CJSON_ROOT))
    except ValueError:
        return str(path)


def write_raw(rows: list[dict]) -> None:
    RESULTS_DIR.mkdir(parents=True, exist_ok=True)

    fields = [
        "run",
        "file",
        "loc",
        "wall_ms",
        "cpu_percent",
        "max_rss_kb",
        "user_seconds",
        "system_seconds",
        "exit_code",
    ]

    with RAW_CSV.open(
        "w",
        newline="",
        encoding="utf-8",
    ) as f:
        writer = csv.DictWriter(
            f,
            fieldnames=fields,
        )

        writer.writeheader()

        for row in rows:
            writer.writerow({
                field: row[field]
                for field in fields
            })


def write_summary(
    executable: Path,
    files: list[Path],
    rows: list[dict],
) -> None:

    grouped: dict[str, list[dict]] = {}

    for row in rows:
        grouped.setdefault(row["file"], []).append(row)

    summary_rows = []

    for file_name, file_rows in grouped.items():

        times = [
            float(r["wall_ms"])
            for r in file_rows
        ]

        cpu = [
            float(r["cpu_percent"])
            for r in file_rows
        ]

        rss = [
            int(r["max_rss_kb"])
            for r in file_rows
        ]

        successes = sum(
            1
            for r in file_rows
            if int(r["exit_code"]) == 0
        )

        loc = int(file_rows[0]["loc"])

        summary_rows.append({
            "file": file_name,
            "loc": loc,
            "runs": len(file_rows),
            "successful_runs": successes,
            "failed_runs": len(file_rows) - successes,
            "median_wall_ms": median(times),
            "mean_wall_ms": mean(times),
            "stddev_wall_ms": (
                stdev(times)
                if len(times) > 1
                else 0.0
            ),
            "min_wall_ms": min(times),
            "max_wall_ms": max(times),
            "mean_cpu_percent": mean(cpu),
            "max_rss_kb": max(rss),
        })

    summary_rows.sort(key=lambda x: x["file"])

    total_loc = sum(
        int(r["loc"])
        for r in summary_rows
    )

    total_runs = sum(
        int(r["runs"])
        for r in summary_rows
    )

    successful_runs = sum(
        int(r["successful_runs"])
        for r in summary_rows
    )

    failed_runs = sum(
        int(r["failed_runs"])
        for r in summary_rows
    )

    all_times = [
        float(r["wall_ms"])
        for r in rows
    ]

    overall = {
        "file": "__TOTAL__",
        "loc": total_loc,
        "runs": total_runs,
        "successful_runs": successful_runs,
        "failed_runs": failed_runs,
        "median_wall_ms": median(all_times)
        if all_times else 0.0,
        "mean_wall_ms": mean(all_times)
        if all_times else 0.0,
        "stddev_wall_ms": stdev(all_times)
        if len(all_times) > 1
        else 0.0,
        "min_wall_ms": min(all_times)
        if all_times else 0.0,
        "max_wall_ms": max(all_times)
        if all_times else 0.0,
        "mean_cpu_percent": mean(
            float(r["cpu_percent"])
            for r in rows
        ) if rows else 0.0,
        "max_rss_kb": max(
            int(r["max_rss_kb"])
            for r in rows
        ) if rows else 0,
    }

    fields = [
        "file",
        "loc",
        "runs",
        "successful_runs",
        "failed_runs",
        "median_wall_ms",
        "mean_wall_ms",
        "stddev_wall_ms",
        "min_wall_ms",
        "max_wall_ms",
        "mean_cpu_percent",
        "max_rss_kb",
    ]

    with SUMMARY_CSV.open(
        "w",
        newline="",
        encoding="utf-8",
    ) as f:

        writer = csv.DictWriter(
            f,
            fieldnames=fields,
        )

        writer.writeheader()

        for row in summary_rows:
            writer.writerow(row)

        writer.writerow(overall)

    print()
    print("============================================================")
    print("  ModernPDE REAL cJSON SOURCE BENCHMARK")
    print("============================================================")
    print()
    print(f"Executable       : {executable}")
    print(f"cJSON root       : {CJSON_ROOT}")
    print(f"C source files   : {len(files)}")
    print(f"Runs / file      : {RUNS}")
    print(f"Total executions : {total_runs}")
    print()
    print(f"Total LOC        : {total_loc}")
    print(f"Successful runs  : {successful_runs}")
    print(f"Failed runs      : {failed_runs}")
    print()
    print(
        f"Median wall time : "
        f"{overall['median_wall_ms']:.3f} ms"
    )
    print(
        f"Mean wall time   : "
        f"{overall['mean_wall_ms']:.3f} ms"
    )
    print(
        f"StdDev           : "
        f"{overall['stddev_wall_ms']:.3f} ms"
    )
    print(
        f"Mean CPU         : "
        f"{overall['mean_cpu_percent']:.1f}%"
    )
    print(
        f"Max RSS          : "
        f"{overall['max_rss_kb']} KB"
    )
    print()
    print("Results:")
    print(f"  {RAW_CSV}")
    print(f"  {SUMMARY_CSV}")
    print("============================================================")


def main() -> int:

    executable = find_executable()
    files = discover_c_files()

    print("ModernPDE Real cJSON Benchmark")
    print("--------------------------------")
    print(f"Executable : {executable}")
    print(f"cJSON      : {CJSON_ROOT}")
    print(f"C files    : {len(files)}")
    print(f"Runs/file  : {RUNS}")
    print()

    rows = []

    for index, source in enumerate(files, start=1):

        loc = count_loc(source)
        rel = relative_source(source)

        print(
            f"[{index:02d}/{len(files):02d}] "
            f"{rel:<70} "
            f"LOC={loc}"
        )

        # First execution is a warm-up.
        warmup = run_one(
            executable,
            source,
        )

        if warmup["exit_code"] != 0:
            print(
                "  WARNING: warm-up returned "
                f"{warmup['exit_code']}"
            )

        for run_number in range(1, RUNS + 1):

            result = run_one(
                executable,
                source,
            )

            rows.append({
                "run": run_number,
                "file": rel,
                "loc": loc,
                "wall_ms": f"{result['wall_ms']:.6f}",
                "cpu_percent": f"{result['cpu_percent']:.2f}",
                "max_rss_kb": result["max_rss_kb"],
                "user_seconds": result["user_seconds"],
                "system_seconds": result["system_seconds"],
                "exit_code": result["exit_code"],
            })

            print(
                f"  run {run_number}: "
                f"{result['wall_ms']:9.3f} ms | "
                f"CPU {result['cpu_percent']:6.1f}% | "
                f"RSS {result['max_rss_kb']:7d} KB | "
                f"exit={result['exit_code']}"
            )

    write_raw(rows)
    write_summary(
        executable,
        files,
        rows,
    )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())

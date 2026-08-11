#!/usr/bin/env python3

import csv
import re
import statistics
import subprocess
import time
from pathlib import Path


# ============================================================
# Configuration
# ============================================================

ROOT = Path(__file__).resolve().parent.parent
BUILD_DIR = ROOT / "build-benchmark"
RESULTS_DIR = ROOT / "benchmarks" / "results"

BENCHMARKS = [
    ("challenge_500", BUILD_DIR / "tests" / "challenge_500", 500),
    ("challenge_5000", BUILD_DIR / "tests" / "challenge_5000", 5000),
    ("challenge_50000", BUILD_DIR / "tests" / "challenge_50000", 50000),
]

WARMUPS = 3
MEASURED_RUNS = 20


# ============================================================
# Run one benchmark
# ============================================================

def run_benchmark(executable):
    """
    Run the benchmark exactly once.

    Measures:
      - wall-clock time using Python perf_counter_ns()
      - CPU percentage using GNU time
      - peak RSS using GNU time
      - process exit code

    The executable is launched only once.
    """

    command = [
        "/usr/bin/time",
        "-f",
        "CPU=%P\nMAX_RSS_KB=%M",
        str(executable),
    ]

    start_ns = time.perf_counter_ns()

    result = subprocess.run(
        command,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )

    end_ns = time.perf_counter_ns()

    wall_ms = (end_ns - start_ns) / 1_000_000.0

    timing = result.stderr

    cpu_match = re.search(
        r"CPU=([0-9]+)%",
        timing
    )

    rss_match = re.search(
        r"MAX_RSS_KB=(\d+)",
        timing
    )

    cpu_percent = (
        float(cpu_match.group(1))
        if cpu_match
        else None
    )

    max_rss_kb = (
        int(rss_match.group(1))
        if rss_match
        else None
    )

    return {
        "exit_code": result.returncode,
        "wall_ms": wall_ms,
        "cpu_percent": cpu_percent,
        "max_rss_kb": max_rss_kb,
        "stdout": result.stdout,
        "stderr": timing,
    }


# ============================================================
# Validate benchmark output
# ============================================================

def validate_output(name, result):
    """
    Make sure the benchmark itself passed.

    All existing challenge tests return:
        0 -> PASS
        non-zero -> FAIL
    """

    if result["exit_code"] != 0:
        print()
        print("=" * 70)
        print(f"ERROR: {name} FAILED")
        print("=" * 70)
        print(result["stdout"])
        print(result["stderr"])
        raise RuntimeError(
            f"{name} returned exit code {result['exit_code']}"
        )

    if "RESULT : FAIL" in result["stdout"]:
        print()
        print(result["stdout"])
        raise RuntimeError(
            f"{name} reported RESULT : FAIL"
        )


# ============================================================
# Statistics
# ============================================================

def calculate_statistics(values):
    return {
        "median": statistics.median(values),
        "mean": statistics.mean(values),
        "stddev": (
            statistics.stdev(values)
            if len(values) > 1
            else 0.0
        ),
        "min": min(values),
        "max": max(values),
    }


# ============================================================
# Main benchmark
# ============================================================

def main():

    RESULTS_DIR.mkdir(
        parents=True,
        exist_ok=True
    )

    print("=" * 70)
    print("ModernPDE Scalability Benchmark")
    print("=" * 70)
    print()
    print(f"Warm-up runs : {WARMUPS}")
    print(f"Measured runs: {MEASURED_RUNS}")
    print()

    # --------------------------------------------------------
    # Check executables
    # --------------------------------------------------------

    print("Checking benchmark executables...")

    for name, executable, variables in BENCHMARKS:

        if not executable.exists():
            raise FileNotFoundError(
                f"\nExecutable not found:\n"
                f"  {executable}\n\n"
                f"Build the project first with:\n"
                f"  cmake --build build-benchmark -j$(nproc)"
            )

        if not executable.is_file():
            raise FileNotFoundError(
                f"Not a regular file: {executable}"
            )

    print("All executables found.")
    print()

    # --------------------------------------------------------
    # Raw results
    # --------------------------------------------------------

    raw_rows = []

    # --------------------------------------------------------
    # Run benchmarks
    # --------------------------------------------------------

    for name, executable, variables in BENCHMARKS:

        print("-" * 70)
        print(f"{name}")
        print(f"Variables: {variables}")
        print("-" * 70)

        # ----------------------------------------------------
        # Warm-up
        # ----------------------------------------------------

        print("Running warm-up...")

        for i in range(WARMUPS):

            result = run_benchmark(executable)

            validate_output(name, result)

            print(
                f"  warm-up {i + 1}: "
                f"{result['wall_ms']:.3f} ms"
            )

        print("Warm-up complete.")
        print()

        # ----------------------------------------------------
        # Measured runs
        # ----------------------------------------------------

        wall_times = []
        cpu_times = []
        rss_values = []

        for run_number in range(
            1,
            MEASURED_RUNS + 1
        ):

            result = run_benchmark(executable)

            validate_output(name, result)

            wall_ms = result["wall_ms"]
            cpu_percent = result["cpu_percent"]
            rss_kb = result["max_rss_kb"]

            wall_times.append(wall_ms)

            if cpu_percent is not None:
                cpu_times.append(cpu_percent)

            if rss_kb is not None:
                rss_values.append(rss_kb)

            raw_rows.append({
                "benchmark": name,
                "variables": variables,
                "run": run_number,
                "wall_ms": wall_ms,
                "cpu_percent": cpu_percent,
                "max_rss_kb": rss_kb,
                "exit_code": result["exit_code"],
            })

            print(
                f"  run {run_number:2d}: "
                f"{wall_ms:8.3f} ms | "
                f"CPU "
                f"{cpu_percent if cpu_percent is not None else 'N/A'}% | "
                f"RSS "
                f"{rss_kb if rss_kb is not None else 'N/A'} KB"
            )

        # ----------------------------------------------------
        # Calculate statistics
        # ----------------------------------------------------

        wall_stats = calculate_statistics(
            wall_times
        )

        if cpu_times:
            mean_cpu = statistics.mean(
                cpu_times
            )
        else:
            mean_cpu = None

        if rss_values:
            max_rss = max(rss_values)
            mean_rss = statistics.mean(
                rss_values
            )
        else:
            max_rss = None
            mean_rss = None

        # Save summary information temporarily
        for row in raw_rows:
            if (
                row["benchmark"] == name
                and row["run"] == MEASURED_RUNS
            ):
                pass

        # ----------------------------------------------------
        # Print per-benchmark summary
        # ----------------------------------------------------

        print()
        print(f"{name} summary:")
        print(
            f"  Median : {wall_stats['median']:.3f} ms"
        )
        print(
            f"  Mean   : {wall_stats['mean']:.3f} ms"
        )
        print(
            f"  StdDev : {wall_stats['stddev']:.3f} ms"
        )
        print(
            f"  Min    : {wall_stats['min']:.3f} ms"
        )
        print(
            f"  Max    : {wall_stats['max']:.3f} ms"
        )

        if mean_cpu is not None:
            print(
                f"  Mean CPU: {mean_cpu:.1f}%"
            )

        if max_rss is not None:
            print(
                f"  Max RSS : {max_rss} KB"
            )

        print()

    # ========================================================
    # Write raw CSV
    # ========================================================

    raw_file = (
        RESULTS_DIR /
        "scalability_raw.csv"
    )

    with raw_file.open(
        "w",
        newline=""
    ) as f:

        writer = csv.DictWriter(
            f,
            fieldnames=[
                "benchmark",
                "variables",
                "run",
                "wall_ms",
                "cpu_percent",
                "max_rss_kb",
                "exit_code",
            ],
        )

        writer.writeheader()
        writer.writerows(raw_rows)

    # ========================================================
    # Build summary CSV
    # ========================================================

    summary_rows = []

    for name, executable, variables in BENCHMARKS:

        rows = [
            row
            for row in raw_rows
            if row["benchmark"] == name
        ]

        wall = [
            row["wall_ms"]
            for row in rows
        ]

        cpu = [
            row["cpu_percent"]
            for row in rows
            if row["cpu_percent"] is not None
        ]

        rss = [
            row["max_rss_kb"]
            for row in rows
            if row["max_rss_kb"] is not None
        ]

        wall_stats = calculate_statistics(wall)

        mean_cpu = (
            statistics.mean(cpu)
            if cpu
            else None
        )

        max_rss = (
            max(rss)
            if rss
            else None
        )

        mean_rss = (
            statistics.mean(rss)
            if rss
            else None
        )

        # Throughput based on median runtime.
        median_ms = wall_stats["median"]

        if median_ms > 0:
            throughput = (
                variables /
                (median_ms / 1000.0)
            )
        else:
            throughput = None

        summary_rows.append({
            "benchmark": name,
            "variables": variables,
            "runs": len(rows),
            "median_wall_ms": wall_stats["median"],
            "mean_wall_ms": wall_stats["mean"],
            "stddev_wall_ms": wall_stats["stddev"],
            "min_wall_ms": wall_stats["min"],
            "max_wall_ms": wall_stats["max"],
            "mean_cpu_percent": mean_cpu,
            "max_rss_kb": max_rss,
            "mean_rss_kb": mean_rss,
            "median_throughput_variables_sec": throughput,
        })

    summary_file = (
        RESULTS_DIR /
        "scalability_summary.csv"
    )

    with summary_file.open(
        "w",
        newline=""
    ) as f:

        writer = csv.DictWriter(
            f,
            fieldnames=[
                "benchmark",
                "variables",
                "runs",
                "median_wall_ms",
                "mean_wall_ms",
                "stddev_wall_ms",
                "min_wall_ms",
                "max_wall_ms",
                "mean_cpu_percent",
                "max_rss_kb",
                "mean_rss_kb",
                "median_throughput_variables_sec",
            ],
        )

        writer.writeheader()
        writer.writerows(summary_rows)

    # ========================================================
    # Final summary
    # ========================================================

    print()
    print("=" * 90)
    print("FINAL SUMMARY")
    print("=" * 90)

    print(
        f"{'Benchmark':<20}"
        f"{'Variables':>12}"
        f"{'Median':>14}"
        f"{'Mean':>14}"
        f"{'StdDev':>14}"
        f"{'Max RSS':>14}"
        f"{'Throughput':>18}"
    )

    print("-" * 90)

    for row in summary_rows:

        throughput = row[
            "median_throughput_variables_sec"
        ]

        throughput_text = (
            f"{throughput:,.0f}"
            if throughput is not None
            else "N/A"
        )

        max_rss = row["max_rss_kb"]

        rss_text = (
            f"{max_rss:,} KB"
            if max_rss is not None
            else "N/A"
        )

        print(
            f"{row['benchmark']:<20}"
            f"{row['variables']:>12,}"
            f"{row['median_wall_ms']:>11.3f} ms"
            f"{row['mean_wall_ms']:>11.3f} ms"
            f"{row['stddev_wall_ms']:>11.3f} ms"
            f"{rss_text:>14}"
            f"{throughput_text:>18} var/s"
        )

    print("=" * 90)
    print()
    print("All measured executions completed successfully.")
    print()
    print(f"Raw results    : {raw_file}")
    print(f"Summary results: {summary_file}")
    print()


# ============================================================
# Entry point
# ============================================================

if __name__ == "__main__":
    main()

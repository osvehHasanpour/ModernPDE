#!/usr/bin/env python3
"""Polish main.tex captions, labels, and transitions."""
from pathlib import Path
import re

p = Path(__file__).resolve().parents[1] / "main.tex"
text = p.read_text(encoding="utf-8")

# Remove duplicate consecutive identical labels
text = re.sub(r"(\\label\{([^}]+)\})\n\\label\{\2\}", r"\1", text)

text = text.replace(r"\textbf{42.1\,\%}", r"42.1\,\%")

reps = {
    r"\captionof{figure}{Accuracy metrics (precision, recall, F1) across PDE approaches.}":
    r"\captionof{figure}{Accuracy metrics across PDE approaches.}",
    r"\captionof{figure}{Distribution of five-way liveness labels ($n{=}800$ definitions).}":
    r"\captionof{figure}{Five-way liveness label distribution ($n{=}800$).}",
    r"\captionof{figure}{Per-layer wall time with and without the scalability substrate.}":
    r"\captionof{figure}{Per-layer wall time with and without scalability substrate.}",
    r"\captionof{figure}{Wall-clock analysis time per benchmark program.}":
    r"\captionof{figure}{Wall-clock analysis time per program.}",
    r"\captionof{figure}{Scalability of \textsc{ModernPDE} versus exhaustive enumeration (log--log).}":
    r"\captionof{figure}{\textsc{ModernPDE} versus exhaustive enumeration (log--log).}",
    r"\captionof{figure}{Share of optimized analysis time by pipeline layer.}":
    r"\captionof{figure}{Share of optimized analysis time by layer.}",
    r"\captionof{figure}{Classifier accuracy on synthetic challenge suites.}":
    r"\captionof{figure}{Classifier accuracy on synthetic challenges.}",
    r"\captionof{figure}{Five-way label mix on synthetic challenge suites.}":
    r"\captionof{figure}{Five-way label mix on synthetic challenges.}",
    r"\captionof{figure}{Median wall time versus synthetic variable count (500 / 5k / 50k).}":
    r"\captionof{figure}{Median wall time versus synthetic variable count.}",
    r"\captionof{figure}{Artifact lines of code by category (code, comment, blank).}":
    r"\captionof{figure}{Artifact lines of code by category.}",
    r"\captionof{figure}{Translation-unit and header counts in the open-source artifact.}":
    r"\captionof{figure}{Translation-unit and header counts.}",
    r"\captionof{figure}{Highest cyclomatic-complexity proxy among analysis sources.}":
    r"\captionof{figure}{Highest cyclomatic-complexity proxy among sources.}",
    r"\captionof{figure}{Evaluation footprint (tests, samples, and code size).}":
    r"\captionof{figure}{Evaluation footprint (tests, samples, code size).}",
    r"\caption{Precision, recall, and F1 against labeled ground truth for three PDE approaches.}":
    r"\caption{Precision, recall, and F1 against labeled ground truth.}",
    r"\caption{ModernPDE vs Muzeel~\cite{kupoluyi2022muzeel}: accuracy, scale, runtime, and complexity metrics shown side by side (not a head-to-head benchmark).}":
    r"\caption{\textsc{ModernPDE} versus Muzeel~\cite{kupoluyi2022muzeel}: accuracy, scale, runtime, and complexity (juxtaposition, not a head-to-head benchmark).}",
    r"\caption{ModernPDE vs DIE~\cite{bastoul2025dead}: accuracy, speedup note, runtime, and complexity juxtaposition (independent analyses).}":
    r"\caption{\textsc{ModernPDE} versus DIE~\cite{bastoul2025dead}: accuracy, speedup note, runtime, and complexity (independent analyses).}",
    r"\caption{ModernPDE vs AutoJMH~\cite{rodriguez2016autojmh}: accuracy, scale, timing, and complexity juxtaposition (units differ by design).}":
    r"\caption{\textsc{ModernPDE} versus AutoJMH~\cite{rodriguez2016autojmh}: accuracy, scale, timing, and complexity (units differ by design).}",
}
for a, b in reps.items():
    if a not in text:
        print("MISS:", a[:70])
    else:
        text = text.replace(a, b)

old_c = """\\section{Complexity Analysis}
\\label{sec:complexity}

Let $P$"""
new_c = """\\section{Complexity Analysis}
\\label{sec:complexity}
We summarize asymptotic costs that explain the empirical curves in \\autoref{sec:results}.

Let $P$"""
if old_c in text:
    text = text.replace(old_c, new_c)
    print("complexity transition")
else:
    print("complexity miss")

old_d = """\\section{Discussion}
\\label{sec:discussion}

\\subsection{Strengths}"""
new_d = """\\section{Discussion}
\\label{sec:discussion}
We interpret the empirical findings, relate them to adjacent dead-code research, and state limitations.

\\subsection{Strengths}"""
if old_d in text:
    text = text.replace(old_d, new_d)
    print("discussion transition")
else:
    print("discussion miss")

old_t = """\\section{Threats to Validity}
\\label{sec:threats}

\\paragraph{Internal validity.}"""
new_t = """\\section{Threats to Validity}
\\label{sec:threats}
We summarize threats to internal, external, construct, and statistical validity.

\\paragraph{Internal validity.}"""
if old_t in text:
    text = text.replace(old_t, new_t)
    print("threats transition")
else:
    print("threats miss")

old_a = """Beyond end-to-end accuracy, we report the structure of the open-source artifact itself. \\autoref{fig:codesize}--\\autoref{fig:cyclomatic} characterize implementation size and complexity. Physical LOC is dominated by analysis sources rather than drivers; file counts show a balanced split between headers and translation units; directory-level LOC concentrates in \\texttt{src/} and \\texttt{tests/}; and cyclomatic-proxy hotspots (parser, field-sensitive analysis, SCCP) match the phases that perform the heaviest branching. \\autoref{fig:footprint} summarizes evaluation surface area (CTest targets, sample inputs, and code size), supporting the claim that the release is a usable research and teaching artifact rather than a single-shot experiment script.

\\autoref{fig:codesize} decomposes lines of code by category (code, comments, blank), showing that implementation effort concentrates in executable analysis logic rather than documentation-only bulk. \\autoref{fig:filecounts} confirms a conventional C++ layout (headers plus translation units) suitable for modular testing. \\autoref{fig:locdir} maps LOC to directories: \\texttt{src/} and \\texttt{tests/} together account for most code, aligning with the pipeline split in \\autoref{fig:pipeline}. \\autoref{fig:cyclomatic} identifies \\texttt{FieldSensitiveAnalysis.cpp} and \\texttt{Parser.cpp} as the highest branching-complexity files---expected, because Phase~1 parsing and Phase~3 field modeling contain the richest case splits. \\autoref{fig:footprint} ties these metrics to the evaluation harness (14 CTest targets, 14 sample inputs), linking artifact structure to reproducibility claims in \\autoref{sec:threats}."""
new_a = """Beyond end-to-end accuracy, we report the structure of the open-source artifact. \\autoref{fig:codesize}--\\autoref{fig:footprint} characterize size, layout, and evaluation surface area. Physical LOC concentrates in analysis sources; headers and translation units are balanced; \\texttt{src/} and \\texttt{tests/} dominate directory-level volume; and cyclomatic-proxy hotspots (\\texttt{FieldSensitiveAnalysis.cpp}, \\texttt{Parser.cpp}) match Phase~1 parsing and Phase~3 field modeling. The evaluation harness exposes 14 CTest targets and 14 sample inputs, supporting reproducibility claims in \\autoref{sec:threats}."""
if old_a in text:
    text = text.replace(old_a, new_a)
    print("artifact condensed")
else:
    print("artifact miss")

# Sync copy
p.write_text(text, encoding="utf-8")
copy = Path(__file__).resolve().parents[1] / "main.tex 2.txt"
copy.write_text(text, encoding="utf-8")
print("synced main.tex 2.txt")
print("alg:pde count", text.count(r"\label{alg:pde}"))
print("alg:callgraph count", text.count(r"\label{alg:callgraph}"))
print("alg:memory count", text.count(r"\label{alg:memory}"))

#!/usr/bin/env python3
"""Unify figure/table captions for TACO polish (content-preserving)."""
from pathlib import Path

p = Path("/mnt/c/Users/osveh/Music/Modernp/ModernPDE/main.tex")
text = p.read_text(encoding="utf-8")

# captionof and caption replacements: old -> new (scientific one-liners)
repls = [
    (
        r"\caption{Benchmark Suite}",
        r"\caption{Benchmark suite used in the evaluation (LOC, CFG nodes, and function counts).}",
    ),
    (
        r"\caption{Accuracy Comparison}",
        r"\caption{Precision, recall, and F1 against labeled ground truth for three PDE approaches.}",
    ),
    (
        r"\captionof{figure}{Accuracy metrics across approaches.}",
        r"\captionof{figure}{Accuracy metrics (precision, recall, F1) across PDE approaches.}",
    ),
    (
        r"\captionof{figure}{Liveness categories ($n{=}800$).}",
        r"\captionof{figure}{Distribution of five-way liveness labels ($n{=}800$ definitions).}",
    ),
    (
        r"\captionof{figure}{Phase time: baseline vs optimized.}",
        r"\captionof{figure}{Per-layer wall time with and without the scalability substrate.}",
    ),
    (
        r"\captionof{figure}{Runtime per test case.}",
        r"\captionof{figure}{Wall-clock analysis time per benchmark program.}",
    ),
    (
        r"\captionof{figure}{Scalability (log-log).}",
        r"\captionof{figure}{Scalability of \textsc{ModernPDE} versus exhaustive enumeration (log--log).}",
    ),
    (
        r"\captionof{figure}{Pipeline cost share.}",
        r"\captionof{figure}{Share of optimized analysis time by pipeline layer.}",
    ),
    (
        r"\captionof{figure}{Challenge accuracy.}",
        r"\captionof{figure}{Classifier accuracy on synthetic challenge suites.}",
    ),
    (
        r"\captionof{figure}{Challenge label mix.}",
        r"\captionof{figure}{Five-way label mix on synthetic challenge suites.}",
    ),
    (
        r"\captionof{figure}{Runtime on the 50k challenge.}",
        r"\captionof{figure}{Wall-clock time on the 50\,000-variable challenge.}",
    ),
    (
        r"\captionof{figure}{Median wall time vs.\ synthetic variable count.}",
        r"\captionof{figure}{Median wall time versus synthetic variable count (500 / 5k / 50k).}",
    ),
    (
        r"\captionof{figure}{Artifact LOC breakdown.}",
        r"\captionof{figure}{Artifact lines of code by category (code, comment, blank).}",
    ),
    (
        r"\captionof{figure}{File counts by kind.}",
        r"\captionof{figure}{Translation-unit and header counts in the open-source artifact.}",
    ),
    (
        r"\captionof{figure}{LOC by directory.}",
        r"\captionof{figure}{Code volume by top-level directory.}",
    ),
    (
        r"\captionof{figure}{Highest cyclomatic-proxy files.}",
        r"\captionof{figure}{Highest cyclomatic-complexity proxy among analysis sources.}",
    ),
    (
        r"\captionof{figure}{Evaluation footprint.}",
        r"\captionof{figure}{Evaluation footprint (tests, samples, and code size).}",
    ),
    (
        r"\captionof{figure}{Muzeel: selected paper \% metrics.}",
        r"\captionof{figure}{Muzeel~\cite{kupoluyi2022muzeel}: selected paper-reported percentage metrics.}",
    ),
    (
        r"\captionof{figure}{Muzeel: corpus scale.}",
        r"\captionof{figure}{Muzeel~\cite{kupoluyi2022muzeel}: reported corpus scale.}",
    ),
    (
        r"\captionof{figure}{Muzeel: elimination savings.}",
        r"\captionof{figure}{Muzeel~\cite{kupoluyi2022muzeel}: reported elimination savings.}",
    ),
    (
        r"\captionof{figure}{Muzeel vs Lacuna similarity.}",
        r"\captionof{figure}{Muzeel~\cite{kupoluyi2022muzeel} versus Lacuna structural similarity.}",
    ),
    (
        r"\captionof{figure}{Muzeel: DCE and speedup view.}",
        r"\captionof{figure}{Muzeel~\cite{kupoluyi2022muzeel}: DCE rate and page-load speedup view.}",
    ),
    (
        r"\captionof{figure}{Muzeel: bytes eliminated.}",
        r"\captionof{figure}{Muzeel~\cite{kupoluyi2022muzeel}: reported bytes eliminated.}",
    ),
    (
        r"\captionof{figure}{DIE: reported LLaMa time reduction.}",
        r"\captionof{figure}{DIE~\cite{bastoul2025dead}: reported LLaMa prompt-processing time reduction.}",
    ),
    (
        r"\captionof{figure}{Conceptual roles in the dead-code space.}",
        r"\captionof{figure}{Conceptual roles of adjacent dead-code research lines.}",
    ),
    (
        r"\captionof{figure}{AutoJMH: extraction reach.}",
        r"\captionof{figure}{AutoJMH~\cite{rodriguez2016autojmh}: extraction reach over Java loops.}",
    ),
    (
        r"\captionof{figure}{AutoJMH: expert similarity.}",
        r"\captionof{figure}{AutoJMH~\cite{rodriguez2016autojmh}: expert timing similarity ablations.}",
    ),
    (
        r"\captionof{figure}{AutoJMH: sort timings (ns).}",
        r"\captionof{figure}{AutoJMH~\cite{rodriguez2016autojmh}: Collections.sort timings (ns).}",
    ),
    (
        r"\captionof{figure}{AutoJMH: rejection reasons.}",
        r"\captionof{figure}{AutoJMH~\cite{rodriguez2016autojmh}: rejection reasons for failed extractions.}",
    ),
    (
        r"\captionof{figure}{Comparability overview vs three papers.}",
        r"\captionof{figure}{Overview of metric comparability against three adjacent papers.}",
    ),
    (
        r"\captionof{figure}{Metric comparability vs Muzeel.}",
        r"\captionof{figure}{Metric comparability audit versus Muzeel~\cite{kupoluyi2022muzeel}.}",
    ),
    (
        r"\captionof{figure}{Metric comparability vs DIE.}",
        r"\captionof{figure}{Metric comparability audit versus DIE~\cite{bastoul2025dead}.}",
    ),
    (
        r"\captionof{figure}{Metric comparability vs AutoJMH.}",
        r"\captionof{figure}{Metric comparability audit versus AutoJMH~\cite{rodriguez2016autojmh}.}",
    ),
    (
        r"\caption{Per-article metric comparability panels (independent analyses).}",
        r"\caption{Per-article metric comparability panels (independent analyses; not head-to-head runtimes).}",
    ),
]

n = 0
for a, b in repls:
    if a in text:
        text = text.replace(a, b)
        n += 1
    else:
        print("MISS:", a[:60])

# Soften student-like bold in prose captions for pipeline already has bold - keep architecture caption
# Unify minipage figure spacing: ensure consistent width already there

# Intro opening polish - lighter transitions
old_intro = """\\paragraph{Research problem.}
We study \\emph{interprocedural, path-aware partial dead code elimination (PDE)}~\\cite{knoop1994partial}: given a program, classify every definition by the fraction of feasible execution paths on which it is used, and identify candidates for safe elimination or sinking. Classical dead-code elimination (DCE) deletes only computations unused on \\emph{all} paths; partial redundancy elimination (PRE)~\\cite{morel1979global,kennedy1999partial} addresses computations redundant on some paths. PDE sits between these extremes. It asks a finer question---on what fraction of feasible paths is a definition used?---and then either eliminates the definition, sinks it toward its uses, or leaves it unchanged when the live-path fraction is high. The central research question is therefore not whether dead code exists, but whether one can \\emph{measure path-partial liveness at scale} while preserving the precision needed for sound optimization across procedures and heap fields.

The practical impact of this finer question is large."""

new_intro = """\\paragraph{Research problem.}
We study \\emph{interprocedural, path-aware partial dead code elimination (PDE)}~\\cite{knoop1994partial}: given a program, classify every definition by the fraction of feasible execution paths on which it is used, and identify candidates for safe elimination or sinking. Classical dead-code elimination (DCE) removes only computations that are unused on \\emph{all} paths, whereas partial redundancy elimination (PRE)~\\cite{morel1979global,kennedy1999partial} targets computations that are redundant on some paths. PDE occupies the space between these extremes. It asks a finer question---on what fraction of feasible paths is a definition used?---and then eliminates the definition, sinks it toward its uses, or leaves it unchanged when the live-path fraction is high. The central research question is therefore not whether dead code exists, but whether path-partial liveness can be \\emph{measured at scale} with sufficient precision for sound optimization across procedures and heap fields.

This finer question has substantial practical impact."""

if old_intro in text:
    text = text.replace(old_intro, new_intro)
    n += 1
    print("intro polished")
else:
    print("intro miss")

# Conclusion polish
old_c = """We presented \\textsc{ModernPDE}, a scalable framework for interprocedural path-aware partial dead-code elimination. Integrating context-sensitive call graphs, field-sensitive memory modeling, and five engineering techniques yields 96.2\\,\\% precision and 91.4\\,\\% recall on a 19-program suite (Table~\\ref{tab:benchmarks}) while cutting analysis time by 78\\,\\% versus exhaustive enumeration. The finding that 42.1\\,\\% of variables are partially dead confirms the practical importance of PDE beyond classical single-function formulations~\\cite{knoop1994partial,bodik1998efficient}. Residual recall loss is concentrated in conservative path/context approximations, not unsound DEAD labels---a trade-off appropriate for an analysis intended to feed elimination passes.

Future work includes front-ends for more languages, measured end-to-end speedup after sinking transforms, ML-guided path pruning, and compact liveness representations. We invite the community to build on the open-source release."""

new_c = """We presented \\textsc{ModernPDE}, a scalable framework for interprocedural path-aware partial dead-code elimination. By integrating context-sensitive call graphs, field-sensitive memory modeling, and five engineering techniques, the framework attains 96.2\\,\\% precision and 91.4\\,\\% recall on a 19-program suite (Table~\\ref{tab:benchmarks}) while reducing analysis time by 78\\,\\% relative to exhaustive enumeration. The observation that 42.1\\,\\% of variables are partially dead confirms the practical importance of PDE beyond classical single-function formulations~\\cite{knoop1994partial,bodik1998efficient}. Residual recall loss arises primarily from conservative path and context approximations rather than from unsound DEAD labels---a trade-off appropriate for an analysis intended to feed elimination passes.

Future work includes additional language front-ends, measured end-to-end speedup after sinking transforms, learning-guided path pruning, and compact liveness representations. We invite the community to build on the open-source release."""

if old_c in text:
    text = text.replace(old_c, new_c)
    n += 1
    print("conclusion polished")
else:
    print("conclusion miss")

p.write_text(text, encoding="utf-8")
(Path("/mnt/c/Users/osveh/Music/Modernp/ModernPDE") / "main.tex 2.txt").write_text(text, encoding="utf-8")
print("replacements applied:", n)
print("lines:", text.count("\n") + 1)

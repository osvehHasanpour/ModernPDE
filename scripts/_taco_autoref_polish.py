#!/usr/bin/env python3
"""Final ACM TACO polish: autoref, palette unity, light prose fixes. Technical content unchanged."""
from pathlib import Path
import re

p = Path("/mnt/c/Users/osveh/Music/Modernp/ModernPDE")
tex = (p / "main.tex").read_text(encoding="utf-8")

# --- Palette: remove off-palette algoFrame; use chartDarkBlue + codeBg ---
tex = tex.replace(
    r"""\definecolor{algoFrame}{RGB}{52,73,94}
\definecolor{algoFill}{RGB}{250,251,252}
""",
    "",
)
tex = tex.replace("colback=algoFill, colframe=algoFrame,", "colback=codeBg, colframe=chartDarkBlue,")

# --- Autoref names (hyperref) ---
autoref_block = r"""
% ---- Autoref names (ACM TACO style) ----
\providecommand{\figureautorefname}{Figure}
\providecommand{\tableautorefname}{Table}
\providecommand{\algorithmautorefname}{Algorithm}
\providecommand{\sectionautorefname}{Section}
\providecommand{\subsectionautorefname}{Section}
\providecommand{\equationautorefname}{Equation}
"""
if "figureautorefname" not in tex:
    tex = tex.replace(
        r"\captionsetup[subfigure]{font=scriptsize,labelfont=bf,justification=centering}",
        r"\captionsetup[subfigure]{font=scriptsize,labelfont=bf,justification=centering}"
        + "\n"
        + autoref_block,
    )

# Convert prefixed refs to autoref (avoid double words)
repl_map = [
    (r"Figures~\\ref\{([^}]+)\}--\\ref\{([^}]+)\}", r"\\autoref{\1}--\\autoref{\2}"),
    (r"Figures~\\ref\{([^}]+)\} and~\\ref\{([^}]+)\}", r"\\autoref{\1} and~\\autoref{\2}"),
    (r"Figures~\\ref\{([^}]+)\} and\\ref\{([^}]+)\}", r"\\autoref{\1} and~\\autoref{\2}"),
    (r"Figures~\\ref\{([^}]+)\}--\\ref\{([^}]+)\}", r"\\autoref{\1}--\\autoref{\2}"),
    (r"Figure~\\ref\{", r"\\autoref{"),
    (r"Table~\\ref\{", r"\\autoref{"),
    (r"Algorithm~\\ref\{", r"\\autoref{"),
    (r"Algorithms~\\ref\{([^}]+)\}--\\ref\{([^}]+)\}", r"\\autoref{\1}--\\autoref{\2}"),
    (r"Section~\\ref\{", r"\\autoref{"),
    (r"Sections~\\ref\{([^}]+)\}--\\ref\{([^}]+)\}", r"\\autoref{\1}--\\autoref{\2}"),
    (r"Equation~\(([0-9]+)\)", r"Equation~(\\ref{eq:\1})"),  # skip if no labels
]
# Simpler sequential string replaces for common patterns
simple = [
    ("Figure~\\ref{", "\\autoref{"),
    ("Figures~\\ref{", "\\autoref{"),
    ("Table~\\ref{", "\\autoref{"),
    ("Algorithm~\\ref{", "\\autoref{"),
    ("Algorithms~\\ref{", "\\autoref{"),
    ("Section~\\ref{", "\\autoref{"),
    ("Sections~\\ref{", "\\autoref{"),
]
for a, b in simple:
    tex = tex.replace(a, b)

# Fix accidental "\\autoref{fig:x}--\\ref{" remaining mid patterns
tex = re.sub(r"\\autoref\{([^}]+)\}--\\ref\{", r"\\autoref{\1}--\\autoref{", tex)
tex = re.sub(r"\\autoref\{([^}]+)\} and~\\ref\{", r"\\autoref{\1} and~\\autoref{", tex)
tex = re.sub(r"and~\\ref\{", r"and~\\autoref{", tex)
tex = re.sub(r"and \\ref\{", r"and~\\autoref{", tex)

# Keywords: sentence case consistency for TACO
tex = tex.replace(
    r"\keywords{Static Program Analysis, Compiler Optimization, Partial Dead Code Elimination, Interprocedural Analysis, Field-Sensitive Analysis, Context-Sensitive Analysis, Path-Sensitive Analysis.}",
    r"\keywords{Static program analysis, compiler optimization, partial dead code elimination, interprocedural analysis, field-sensitive analysis, context-sensitive analysis, path-sensitive analysis}",
)

# Light prose: front-end spelling consistency
tex = tex.replace("frontend IR", "front-end IR")
tex = tex.replace("a source frontend", "a source front-end")
tex = tex.replace("same frontend", "same front-end")

# Include palette comment block for maintainers
palette_doc = r"""
% Unified ModernPDE / ACM TACO visual palette (use ONLY these):
%   chartBlue / colorLive      RGB 41,128,185
%   chartGreen / colorMostlyLive RGB 39,174,96
%   chartOrange                RGB 243,156,18
%   chartRed / colorDead       RGB 192,57,43
%   chartPurple                RGB 142,68,173
%   chartTeal                  RGB 26,188,156
%   chartDarkBlue              RGB 31,78,121
%   colorMostlyDead            RGB 230,126,34
%   colorPartiallyDead         RGB 241,196,15
%   codeBg                     RGB 248,249,250
"""
if "Unified ModernPDE / ACM TACO visual palette" not in tex:
    tex = tex.replace("% ---- Colors (unified TACO palette) ----", palette_doc + "% ---- Colors (unified TACO palette) ----")

(p / "main.tex").write_text(tex, encoding="utf-8")
(p / "main.tex 2.txt").write_text(tex, encoding="utf-8")
print("lines", tex.count("\n") + 1)
print("autoref count", tex.count("\\autoref{"))
print("remaining Figure~\\ref", tex.count("Figure~\\ref{"))
print("algoFrame gone", "algoFrame" not in tex)

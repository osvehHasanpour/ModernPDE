from pathlib import Path
p = Path("/mnt/c/Users/osveh/Music/Modernp/ModernPDE")
t = (p / "main.tex").read_text(encoding="utf-8")

t = t.replace(
    "caption={Motivating example: partial dead computation of metadata fields.}",
    "caption={Motivating example illustrating partially dead metadata-field computations.}",
)
t = t.replace(
    "Each baseline reuses the same frontend and IR (Phases~1--2) so differences isolate analysis choices:",
    "Each baseline reuses the same front-end and IR (L1--L2) so that differences isolate analysis choices:",
)
t = t.replace(
    "Each baseline reuses the same frontend and IR (Phases~1--2) so differences isolate analysis choices",
    "Each baseline reuses the same front-end and IR (L1--L2) so that differences isolate analysis choices",
)

# TikZ: slightly tighter professional fonts
t = t.replace(
    "font=\\scriptsize\\sffamily\\bfseries,",
    "font=\\scriptsize\\bfseries,",
)
t = t.replace(
    "font=\\scriptsize\\sffamily,",
    "font=\\scriptsize,",
)
t = t.replace(
    "font=\\scriptsize\\sffamily\\bfseries, align=center,",
    "font=\\scriptsize\\bfseries, align=center,",
)
t = t.replace("font=\\sffamily,\n  node distance", "node distance")
t = t.replace("font=\\scriptsize\\sffamily\\itshape,", "font=\\scriptsize\\itshape,")
t = t.replace("font=\\scriptsize\\sffamily\\bfseries, text=chartRed", "font=\\scriptsize\\bfseries, text=chartRed")
t = t.replace("font=\\tiny\\sffamily,", "font=\\tiny,")
t = t.replace("font=\\scriptsize\\sffamily\\bfseries, text=chartPurple", "font=\\scriptsize\\bfseries, text=chartPurple")
t = t.replace("font=\\scriptsize\\sffamily\\bfseries, anchor=west]", "font=\\scriptsize\\bfseries, anchor=west]")

(p / "main.tex").write_text(t, encoding="utf-8")
(p / "main.tex 2.txt").write_text(t, encoding="utf-8")
print("synced lines", t.count("\n") + 1)
print("ForEach defined", "SetKwFor{ForEach}" in t)
print("ML gone", "Machine learning" not in t)
print("motivating caption", "illustrating partially dead" in t)

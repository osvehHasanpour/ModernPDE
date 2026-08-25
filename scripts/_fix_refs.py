from pathlib import Path
import re

root = Path("/mnt/c/Users/osveh/Music/Modernp/ModernPDE")
t = (root / "main.tex").read_text(encoding="utf-8")
t = t.replace(", \\ref{fig:scalability}", ", \\autoref{fig:scalability}")
# Convert any remaining labeled object \ref to \autoref
t = re.sub(r"(?<!auto)\\ref\{((?:fig|tab|alg|sec):[^}]+)\}", r"\\autoref{\1}", t)
leftovers = re.findall(r"(?<!auto)\\ref\{((?:fig|tab|alg|sec):[^}]+)\}", t)
(root / "main.tex").write_text(t, encoding="utf-8")
(root / "main.tex 2.txt").write_text(t, encoding="utf-8")
print("leftovers", leftovers)
print("lines", t.count("\n") + 1)
print("autoref", t.count("\\autoref{"))

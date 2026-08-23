from pathlib import Path
import re
text = Path("/mnt/c/Users/osveh/Music/Modernp/ModernPDE/main.tex").read_text(encoding="utf-8")
secs = re.findall(r"\\section\*?\{([^}]*)\}", text)
print("sections:", secs)
figs = re.findall(r"\\includegraphics\[[^\]]*\]\{([^}]*)\}", text)
print("figures:", len(figs))
for f in figs:
    p = Path("/mnt/c/Users/osveh/Music/Modernp/ModernPDE") / f
    # plots may be under plots/ symlink
    exists = p.exists() or (Path("/mnt/c/Users/osveh/Music/Modernp/ModernPDE") / f.replace("plots/", "output/plots/")).exists()
    print(("OK" if exists else "MISSING"), f)

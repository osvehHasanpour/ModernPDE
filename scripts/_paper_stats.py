from pathlib import Path
p = Path("/mnt/c/Users/osveh/Music/Modernp/ModernPDE")
src = next(p.glob("main.tex*"))
text = src.read_text(encoding="utf-8", errors="replace")
print("source:", repr(src.name))
print("lines:", text.count("\n") + 1)
print("chars:", len(text))
dst = p / "main.tex"
dst.write_text(text, encoding="utf-8")
print("wrote main.tex", dst.stat().st_size)
# verify no new usepackage beyond original set by checking documentclass still first content packages
pkgs = [ln for ln in text.splitlines() if ln.strip().startswith("\\usepackage")]
print("usepackage count:", len(pkgs))

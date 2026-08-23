from pathlib import Path
import re

p = Path("/mnt/c/Users/osveh/Music/Modernp/ModernPDE")
src = p / "main.tex"
if not src.exists():
    src = next(p.glob("main.tex*"))
text = src.read_text(encoding="utf-8", errors="replace")
lines = text.splitlines()
print("FILE:", src.name)
print("LINES:", len(lines), "CHARS:", len(text))

secs = []
for i, ln in enumerate(lines, 1):
    m = re.match(r"\\(section|subsection|subsubsection)\*?\{([^}]*)\}", ln)
    if m:
        secs.append((i, m.group(1), m.group(2)))

print("\n=== STRUCTURE ===")
for i, (ln, kind, title) in enumerate(secs):
    end = secs[i + 1][0] - 1 if i + 1 < len(secs) else len(lines)
    print(f"{ln:4d}-{end:4d} ({end - ln + 1:4d} lines) {kind}: {title}")

print("\n=== FIGURES ===")
used = []
for m in re.finditer(r"\\begin\{figure\*?\}.*?\\end\{figure\*?\}", text, re.S):
    block = m.group(0)
    caps = re.findall(r"\\caption\{((?:[^{}]|\{[^{}]*\})*)\}", block)
    labs = re.findall(r"\\label\{([^}]*)\}", block)
    imgs = re.findall(r"\\includegraphics(?:\[[^\]]*\])?\{([^}]*)\}", block)
    used.extend(imgs)
    print("LAB", labs, "NIMG", len(imgs))
    for im in imgs:
        print("  ", im)
    if caps:
        print("  CAP:", caps[0][:160].replace("\n", " "))

print("\n=== TABLES ===")
for m in re.finditer(r"\\begin\{table\*?\}.*?\\end\{table\*?\}", text, re.S):
    block = m.group(0)
    caps = re.findall(r"\\caption\{((?:[^{}]|\{[^{}]*\})*)\}", block)
    labs = re.findall(r"\\label\{([^}]*)\}", block)
    print("TAB", labs, (caps[0][:120] if caps else ""))

print("\n=== ALGORITHMS ===")
for m in re.finditer(r"\\begin\{algorithm\*?\}.*?\\end\{algorithm\*?\}", text, re.S):
    block = m.group(0)
    caps = re.findall(r"\\caption\{((?:[^{}]|\{[^{}]*\})*)\}", block)
    labs = re.findall(r"\\label\{([^}]*)\}", block)
    print("ALG", labs, (caps[0][:120] if caps else ""))

print("\n=== EQUATIONS / MATH ENV ===")
for env in ["equation", "align", "gather", "eqnarray"]:
    n = len(re.findall(rf"\\begin\{{{env}\*?\}}", text))
    if n:
        print(env, n)

print("\n=== CITE KEYS ===")
keys = set()
for c in re.findall(r"\\cite[tp]?\{([^}]*)\}", text):
    for k in c.split(","):
        keys.add(k.strip())
print(sorted(keys))
print("n_unique", len(keys), "n_cite_cmds", len(re.findall(r"\\cite", text)))

# claims / numbers
print("\n=== NUMERIC CLAIMS (sample) ===")
for pat in [r"96\.2", r"91\.4", r"42\.1", r"78", r"precision", r"recall", r"F1", r"partially dead"]:
    hits = [(i + 1, lines[i].strip()[:140]) for i, ln in enumerate(lines) if re.search(pat, ln, re.I)]
    print(pat, "->", len(hits), "hits")
    for h in hits[:3]:
        print(" ", h)

# plot inventory
plot_dirs = [p / "output" / "plots", p / "plots"]
all_plots = set()
for d in plot_dirs:
    if d.exists() or d.is_symlink():
        try:
            for f in d.rglob("*.png"):
                all_plots.add(f.name)
            for f in d.rglob("*.pdf"):
                all_plots.add(f.name)
        except Exception as e:
            print("plot scan err", d, e)

used_names = {Path(u).name for u in used}
print("\n=== PLOT USAGE ===")
print("used", len(used_names))
print("available", len(all_plots))
unused = sorted(all_plots - used_names)
print("unused count", len(unused))
for u in unused:
    print(" UNUSED", u)

# packages
pkgs = [ln.strip() for ln in lines if ln.strip().startswith("\\usepackage")]
print("\n=== PACKAGES", len(pkgs), "===")
for pk in pkgs:
    print(pk)

# documentclass
for ln in lines[:30]:
    if "documentclass" in ln or "acm" in ln.lower():
        print(ln)

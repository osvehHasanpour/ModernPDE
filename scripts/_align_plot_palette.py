#!/usr/bin/env python3
from pathlib import Path
import re

PAL = {
    "BLUE": "#2980B9",
    "GREEN": "#27AE60",
    "ORANGE": "#F39C12",
    "RED": "#C0392B",
    "PURPLE": "#8E44AD",
    "TEAL": "#1ABC9C",
    "DARKBLUE": "#1F4E79",
    "MOSTLY_DEAD": "#E67E22",
    "PARTIAL": "#F1C40F",
}

root = Path("/mnt/c/Users/osveh/Music/Modernp/ModernPDE/scripts")

mp = root / "make_plots.py"
t = mp.read_text(encoding="utf-8")
t = t.replace('color="#3b6ea5"', f'color="{PAL["BLUE"]}"')
t = t.replace(
    'colors = ["#b23a48", "#d98c5f", "#e8c46a", "#8fb996", "#3b6ea5"]',
    f'colors = ["{PAL["RED"]}", "{PAL["MOSTLY_DEAD"]}", "{PAL["PARTIAL"]}", "{PAL["GREEN"]}", "{PAL["BLUE"]}"]',
)
mp.write_text(t, encoding="utf-8")
print("updated make_plots.py")

ep = root / "export_paper_comparison.py"
t = ep.read_text(encoding="utf-8")
t = t.replace('BLUE = "#2F5D8A"', f'BLUE = "{PAL["BLUE"]}"')
t = t.replace('OCHRE = "#C4783A"', f'OCHRE = "{PAL["ORANGE"]}"')
t = t.replace('GREEN = "#3D7A5A"', f'GREEN = "{PAL["GREEN"]}"')
t = t.replace('SLATE = "#4A5560"', f'SLATE = "{PAL["DARKBLUE"]}"')
t = t.replace('RED = "#A33B3B"', f'RED = "{PAL["RED"]}"')
ep.write_text(t, encoding="utf-8")
print("updated export_paper_comparison.py")

gp = root / "generate_modernpde_plots.py"
t = gp.read_text(encoding="utf-8")
# Replace house palette block
old = '''C_DEAD          = "#b23a48"
C_MOSTLY_DEAD   = "#d98c5f"
C_PARTIAL_DEAD  = "#e8c46a"
C_MOSTLY_LIVE   = "#8fb996"
C_LIVE          = "#3b6ea5"
C_PHASE7        = "#8a8d91"
C_PHASE8        = "#3b6ea5"
C_ACCENT        = "#b23a48"
C_NEUTRAL_DARK  = "#2b2b2b"'''
new = f'''C_DEAD          = "{PAL["RED"]}"
C_MOSTLY_DEAD   = "{PAL["MOSTLY_DEAD"]}"
C_PARTIAL_DEAD  = "{PAL["PARTIAL"]}"
C_MOSTLY_LIVE   = "{PAL["GREEN"]}"
C_LIVE          = "{PAL["BLUE"]}"
C_PHASE7        = "{PAL["DARKBLUE"]}"
C_PHASE8        = "{PAL["BLUE"]}"
C_ACCENT        = "{PAL["RED"]}"
C_NEUTRAL_DARK  = "{PAL["DARKBLUE"]}"'''
if old in t:
    t = t.replace(old, new)
t = t.replace('metric_colors = ["#3b6ea5", "#8fb996", "#d98c5f", "#b23a48"]',
              f'metric_colors = ["{PAL["BLUE"]}", "{PAL["GREEN"]}", "{PAL["ORANGE"]}", "{PAL["RED"]}"]')
t = t.replace(
    'colors = ["#6b7f9e", "#8a8d91", "#7ea6c4", "#8fb996",\n              "#e8c46a", "#b23a48", "#3b6ea5"]',
    f'colors = ["{PAL["DARKBLUE"]}", "{PAL["TEAL"]}", "{PAL["BLUE"]}", "{PAL["GREEN"]}",\n'
    f'              "{PAL["PARTIAL"]}", "{PAL["RED"]}", "{PAL["PURPLE"]}"]',
)
t = t.replace('"axes.edgecolor": "#333333"', f'"axes.edgecolor": "{PAL["DARKBLUE"]}"')
t = t.replace('color="#555555"', f'color="{PAL["DARKBLUE"]}"')
t = t.replace('color="#444444"', f'color="{PAL["DARKBLUE"]}"')
gp.write_text(t, encoding="utf-8")
print("updated generate_modernpde_plots.py")

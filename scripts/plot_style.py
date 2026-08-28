"""
Uniform figure dimensions for ModernPDE / ACM TACO publication plots.

Every PNG in output/plots/ is saved at exactly:
  7.0 in × 4.5 in @ 300 dpi  →  2100 × 1350 pixels
"""

from __future__ import annotations

from pathlib import Path

import matplotlib.pyplot as plt

FIG_W_IN = 7.0
FIG_H_IN = 4.5
FIG_DPI = 300
FIG_SIZE = (FIG_W_IN, FIG_H_IN)
FIG_SIZE_PX = (int(FIG_W_IN * FIG_DPI), int(FIG_H_IN * FIG_DPI))


def new_figure(*, nrows: int = 1, ncols: int = 1, **kwargs):
    """Create a figure on the standard journal canvas."""
    return plt.subplots(nrows, ncols, figsize=FIG_SIZE, dpi=FIG_DPI, **kwargs)


def layout_single(fig) -> None:
    fig.subplots_adjust(left=0.12, right=0.96, top=0.88, bottom=0.15)


def layout_wide(fig) -> None:
    """Two side-by-side panels (e.g. fig6)."""
    fig.subplots_adjust(left=0.08, right=0.98, top=0.88, bottom=0.18, wspace=0.32)


def layout_grid2x2(fig) -> None:
    fig.subplots_adjust(left=0.08, right=0.98, top=0.90, bottom=0.10, hspace=0.52, wspace=0.38)


def layout_tall_barh(fig) -> None:
    """Dense horizontal bar charts (fig5, cyclomatic)."""
    fig.subplots_adjust(left=0.28, right=0.96, top=0.90, bottom=0.10)


def journal_save(fig, path: str | Path, *, layout: str | None = "single") -> None:
    """Save PNG/PDF at fixed physical size."""
    path = Path(path)
    fig.set_size_inches(FIG_W_IN, FIG_H_IN, forward=False)
    if layout == "single":
        layout_single(fig)
    elif layout == "wide":
        layout_wide(fig)
    elif layout == "grid2x2":
        layout_grid2x2(fig)
    elif layout == "tall_barh":
        layout_tall_barh(fig)
    elif layout is not None:
        raise ValueError(f"unknown layout: {layout}")

    kwargs = dict(facecolor="white", edgecolor="none", pad_inches=0)
    fig.savefig(path, **kwargs)
    if path.suffix.lower() == ".png":
        fig.savefig(path.with_suffix(".pdf"), **kwargs)


def verify_png_sizes(plots_dir: Path, names: list[str] | None = None) -> None:
    """Raise if any named PNG deviates from FIG_SIZE_PX."""
    try:
        from PIL import Image
    except ImportError:
        print("  (skip PNG size verify: Pillow not installed)")
        return

    targets = names or [p.name for p in sorted(plots_dir.glob("*.png"))]
    bad = []
    for name in targets:
        png = plots_dir / name
        if not png.exists():
            bad.append(f"{name}: missing")
            continue
        with Image.open(png) as im:
            if im.size != FIG_SIZE_PX:
                bad.append(f"{name}: {im.size} != {FIG_SIZE_PX}")
    if bad:
        raise SystemExit("PNG size mismatch:\n  " + "\n  ".join(bad))
    print(f"  verified {len(targets)} PNGs at {FIG_SIZE_PX[0]}×{FIG_SIZE_PX[1]} px")

#!/usr/bin/env python3
"""Export every PNG in output/plots/ to PDF (per-file + one combined booklet)."""

from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PLOTS = ROOT / "output" / "plots"


def main() -> int:
    try:
        from PIL import Image
    except ImportError as exc:
        raise SystemExit("Pillow required: pip install pillow") from exc

    PLOTS.mkdir(parents=True, exist_ok=True)
    pngs = sorted(PLOTS.glob("*.png"))
    if not pngs:
        print(f"No PNG files in {PLOTS}")
        return 1

    rgb_pages: list[Image.Image] = []
    for png in pngs:
        img = Image.open(png)
        if img.mode in ("RGBA", "LA", "P"):
            background = Image.new("RGB", img.size, (255, 255, 255))
            if img.mode == "P":
                img = img.convert("RGBA")
            background.paste(img, mask=img.split()[-1] if img.mode == "RGBA" else None)
            img = background
        elif img.mode != "RGB":
            img = img.convert("RGB")

        pdf_path = png.with_suffix(".pdf")
        img.save(pdf_path, "PDF", resolution=300.0)
        print(f"  {pdf_path.relative_to(ROOT)}")
        rgb_pages.append(img.copy())

    combined = PLOTS / "all_plots_combined.pdf"
    first, *rest = rgb_pages
    first.save(combined, "PDF", resolution=300.0, save_all=True, append_images=rest)
    print(f"\nCombined: {combined.relative_to(ROOT)} ({len(rgb_pages)} pages)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

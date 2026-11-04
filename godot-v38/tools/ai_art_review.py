#!/usr/bin/env python3
"""Moonfall AI art production board: 55 guardians + Americas wildlife, offline."""
import argparse
from collections import Counter
from html import escape
import json
import os
from pathlib import Path
import sys
from urllib.parse import quote

import ai_art_studio as studio


def all_items() -> list[dict]:
    guardians = [studio.guardian_profile(i) for i in range(55)]
    animals = [studio.animal_profile(name) for name in sorted(studio.available_animals())]
    return guardians + animals


def review_status(item: dict) -> dict:
    staged, approved = studio.dest_paths(item)
    prompt = studio.prompt_for(item)
    chosen = approved if approved.exists() else staged
    state = "approved" if approved.exists() else "staged" if staged.exists() else "missing"
    valid = False
    issue = ""
    if state != "missing":
        try:
            payload = chosen.read_bytes()
            studio.validate_png(payload)
            record = chosen.with_suffix(".json")
            meta = json.loads(record.read_text(encoding="utf-8"))
            import hashlib
            valid = (
                meta.get("asset_id") == item["asset_id"] and
                meta.get("sha256") == hashlib.sha256(payload).hexdigest() and
                meta.get("reviewed") is (state == "approved")
            )
            if not valid:
                issue = "Metadata or checksum does not match PNG"
        except (ValueError, OSError, json.JSONDecodeError) as exc:
            issue = str(exc)
    return {
        "id": item["asset_id"], "subject": item["subject"],
        "category": item["category"], "state": state, "valid": valid,
        "issue": issue, "prompt": prompt,
        "image_path": str(chosen) if state != "missing" else "",
    }


def plan_rows(category: str = "all") -> list[dict]:
    return [
        review_status(item)
        for item in all_items()
        if category == "all" or item["category"] == category
    ]


def local_image_src(path: str, gallery_dir: Path) -> str:
    if not path:
        return ""
    relative = Path(os.path.relpath(path, gallery_dir))
    # Paths are constructed only from validated catalog names/IDs.
    return quote(relative.as_posix(), safe="/.-_")


def make_gallery(rows: list[dict], destination: Path) -> None:
    destination.parent.mkdir(parents=True, exist_ok=True)
    cards = []
    for row in rows:
        title = escape(row["subject"])
        state = escape(row["state"])
        category = escape(row["category"])
        aid = escape(row["id"])
        image = ""
        if row["image_path"] and row["valid"]:
            src = escape(local_image_src(row["image_path"], destination.parent), quote=True)
            image = f'<img src="{src}" alt="Staged artwork for {title}" loading="lazy">'
        elif row["image_path"]:
            image = '<span class="invalid">File failed validation</span>'
        else:
            image = '<span class="placeholder" aria-hidden="true">◌</span>'
        issue = f'<p class="issue">{escape(row["issue"])}</p>' if row["issue"] else ""
        cards.append(
            f'<article class="card"><div class="image">{image}</div>'
            f'<h2>{title}</h2><p>{category} / {aid}</p>'
            f'<p>Status: <strong>{state}</strong></p>{issue}'
            f'<details><summary>Generation prompt</summary>'
            f'<p class="prompt">{escape(row["prompt"])}</p></details></article>'
        )
    counts = Counter(x["state"] for x in rows)
    summary = " · ".join(f"{count} {name}" for name, count in sorted(counts.items()))
    document = f"""<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Moonfall AI Art Review Board</title>
<style>
:root {{font-family:system-ui,-apple-system,Segoe UI,sans-serif;color-scheme:dark;background:#151420;color:#f5eafa}}
body {{max-width:1300px;margin:0 auto;padding:2rem 1rem}}
header {{margin-bottom:2rem}} .grid {{display:grid;grid-template-columns:repeat(auto-fill,minmax(210px,1fr));gap:1rem}}
.card {{background:#252236;border:1px solid #524662;border-radius:12px;padding:.75rem;overflow-wrap:anywhere}}
.image {{height:190px;display:grid;place-items:center;background:#343047;border-radius:8px}}
.image img {{max-height:100%;max-width:100%;object-fit:contain}}
.placeholder {{font-size:4rem;color:#b6a4c8}} .invalid,.issue {{color:#ffb5aa}}
h2 {{font-size:1rem;margin:.65rem 0}} .card p {{font-size:.83rem;margin:.4rem 0}}
details {{font-size:.85rem;cursor:pointer}} .prompt {{white-space:pre-wrap}}
</style></head><body><header><h1>Moonfall AI Art Review Board</h1>
<p>{escape(summary)} · {len(rows)} planned creature artworks</p>
<p>Offline preview only. No images are generated or approved by this page.
Review staged images, then approve each image separately using the command line.
Original Z and Nyra artwork is never used.</p></header>
<main class="grid">{''.join(cards)}</main></body></html>"""
    destination.write_text(document, encoding="utf-8")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--category", choices=("all", "guardians", "animals"), default="all")
    parser.add_argument("--gallery", action="store_true", help="Create offline review HTML and JSON plan")
    parser.add_argument("--audit", action="store_true", help="Return failure if any present artwork is invalid")
    args = parser.parse_args(argv)
    rows = plan_rows(args.category)
    counts = Counter(row["state"] for row in rows)
    invalid = [row for row in rows if row["state"] != "missing" and not row["valid"]]
    print(f"Moonfall art plan: {len(rows)} entries, {counts['approved']} approved, "
          f"{counts['staged']} staged, {counts['missing']} missing, {len(invalid)} invalid")
    if args.gallery:
        directory = studio.STAGING / "review"
        directory.mkdir(parents=True, exist_ok=True)
        (directory / "art_plan.json").write_text(json.dumps(rows, indent=2) + "\n", encoding="utf-8")
        make_gallery(rows, directory / "index.html")
        print(f"Review gallery: {directory / 'index.html'}")
    for row in invalid:
        print(f"INVALID {row['id']}: {row['issue']}", file=sys.stderr)
    return 1 if args.audit and invalid else 0


if __name__ == "__main__":
    raise SystemExit(main())

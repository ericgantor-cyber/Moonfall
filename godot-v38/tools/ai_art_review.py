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
import ai_art_3d as model
import hashlib
from ai_art_game_catalog import all_game_items, SLOTS


def all_items() -> list[dict]:
    guardians = [studio.guardian_profile(i) for i in range(55)]
    animals = [studio.animal_profile(name) for name in sorted(studio.available_animals())]
    return guardians + animals + all_game_items()


def review_status(item: dict) -> dict:
    staged, approved = studio.dest_paths(item)
    prompt = studio.prompt_for(item)
    chosen = approved if approved.exists() else staged
    state = "approved" if approved.exists() else "staged" if staged.exists() else "missing"
    valid = False
    issue = ""
    image_checksum = ""
    if state != "missing":
        try:
            payload = chosen.read_bytes()
            studio.validate_png(payload)
            record = chosen.with_suffix(".json")
            meta = json.loads(record.read_text(encoding="utf-8"))
            if not isinstance(meta, dict):
                raise ValueError("PNG review manifest must be a JSON object.")
            valid = (
                meta.get("asset_id") == item["asset_id"] and
                meta.get("sha256") == hashlib.sha256(payload).hexdigest() and
                meta.get("reviewed") is (state == "approved")
            )
            if valid:
                image_checksum = meta["sha256"]
            else:
                issue = "Metadata or checksum does not match PNG"
        except (ValueError, OSError, json.JSONDecodeError) as exc:
            issue = str(exc)
    model_state = "not_applicable"
    model_valid = False
    model_issue = ""
    model_checksum = ""
    model_details = {}
    model_file = ""
    if item["category"] in model.ALLOWED_MODEL_CATEGORIES:
        model_staged, model_approved, _ = model.paths(item)
        selected = model_approved if model_approved.exists() else model_staged
        model_state = ("approved" if model_approved.exists()
                       else "staged" if model_staged.exists() else "missing")
        if model_state != "missing":
            model_file = str(selected)
            try:
                payload = selected.read_bytes()
                model_details = model.glb_info(payload)
                record = json.loads(selected.with_suffix(".json").read_text(encoding="utf-8"))
                if not isinstance(record, dict):
                    raise ValueError("3D review manifest must be a JSON object.")
                model_valid = (
                    record.get("asset_id") == item["asset_id"]
                    and record.get("reviewed") is (model_state == "approved")
                    and record.get("sha256") == hashlib.sha256(payload).hexdigest()
                )
                if model_valid:
                    model_checksum = record["sha256"]
                else:
                    model_issue = "Model review manifest/checksum mismatch"
            except (ValueError, OSError, json.JSONDecodeError) as error:
                model_issue = str(error)
    # A model awaiting review matters more than a sibling format already
    # approved: keep mixed PNG-approved/GLB-staged cards visible in the
    # gallery's Ready to review filter.
    display_state = ("staged" if state == "staged" or model_state == "staged"
                     else "approved" if state == "approved" or model_state == "approved"
                     else "missing")
    return {
        "id": item["asset_id"], "subject": item["subject"],
        "category": item["category"], "state": state, "valid": valid,
        "issue": issue, "prompt": prompt,
        "image_path": str(chosen) if state != "missing" else "",
        "image_checksum": image_checksum,
        "model_checksum": model_checksum,
        "display_state": display_state, "model_state": model_state,
        "model_valid": model_valid, "model_issue": model_issue,
        "model_geometry": model_details, "model_path": model_file,
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
        state = escape(row.get("display_state", row["state"]))
        png_state = escape(row["state"])
        glb_state = escape(row.get("model_state", "not_applicable"))
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
        model_issue = row.get("model_issue", "")
        if model_issue:
            issue += f'<p class="issue">3D: {escape(model_issue)}</p>'
        geometry = row.get("model_geometry", {})
        model_summary = ""
        if row.get("model_state") != "not_applicable":
            maps = geometry.get("max_texture_size", 0)
            pbr = geometry.get("pbr_material_count", 0)
            faces = geometry.get("triangles", 0)
            texture = ("Embedded 4K+ maps" if geometry.get("has_4k_texture", False)
                       else f"Max embedded map: {maps}px" if maps else "No measured texture maps")
            model_summary = (
                f'<p>PNG: {png_state} | 3D: <strong>{glb_state}</strong></p>'
                f'<p>{faces:,} triangles · {escape(texture)} · {pbr} PBR materials</p>'
                if geometry else f'<p>PNG: {png_state} | 3D: <strong>{glb_state}</strong></p>'
            )
        choices = ""
        if row["state"] == "staged" and row["valid"] and row.get("image_checksum"):
            choices += (
                f'<label class="review-choice"><input class="approval-select" type="checkbox" '
                f'data-id="{aid}" data-format="png" data-sha256="{escape(row["image_checksum"])}">'
                f'Approve reviewed PNG</label>'
            )
        if row.get("model_state") == "staged" and row.get("model_valid") and row.get("model_checksum"):
            choices += (
                f'<label class="review-choice"><input class="approval-select" type="checkbox" '
                f'data-id="{aid}" data-format="glb" data-sha256="{escape(row["model_checksum"])}">'
                f'Approve reviewed 3D model</label>'
            )
        if row.get("model_path") and row.get("model_valid"):
            url = escape(local_image_src(row["model_path"], destination.parent), quote=True)
            choices += f'<a href="{url}">Open 3D GLB in an external model viewer</a>'
        cards.append(
            f'<article class="card" data-category="{category}" data-status="{state}"><div class="image">{image}</div>'
            f'<h2>{title}</h2><p>{category} / {aid}</p>'
            f'<p>Status: <strong>{state}</strong></p>{model_summary}{issue}{choices}'
            f'<details><summary>Generation prompt</summary>'
            f'<p class="prompt">{escape(row["prompt"])}</p></details></article>'
        )
    counts = Counter(x.get("display_state", x["state"]) for x in rows)
    summary = " · ".join(f"{count} {name}" for name, count in sorted(counts.items()))
    controls = """<section class="filters" aria-label="Artwork filters">
<label for="art-search">Search creatures <input id="art-search" type="search" placeholder="Guardian or wildlife name"></label>
<label for="art-status">Status <select id="art-status"><option value="all">All</option>
<option value="missing">Missing</option><option value="staged">Ready to review</option>
<option value="approved">Approved</option></select></label>
<label for="art-category">Category <select id="art-category"><option value="all">All</option>
<option value="guardians">Guardians</option><option value="animals">Wildlife</option>
<option value="regions">Regions</option><option value="chambers">Dungeon chambers</option>
<option value="glyphs">HUD & glyphs</option><option value="spells">Spells</option>
<option value="gear">Gear</option><option value="scenery">Scenery</option></select></label>
<output id="art-count" aria-live="polite"></output></section>
<section class="approve-toolbar" aria-label="Review approvals">
<button type="button" id="export-art-selection">Export checked artwork for approval</button>
<output id="selected-art-count" aria-live="polite">0 files selected</output>
<p>Only select art after reviewing the image or 3D model. Export downloads a JSON selection for the separate offline approval command. No artwork is approved by the gallery.</p>
</section>"""
    filter_script = """<script>
(() => {
  const search = document.getElementById('art-search');
  const status = document.getElementById('art-status');
  const category = document.getElementById('art-category');
  const count = document.getElementById('art-count');
  const cards = Array.from(document.querySelectorAll('.card'));
  const approvals = Array.from(document.querySelectorAll('.approval-select'));
  const selectedCount = document.getElementById('selected-art-count');
  function selected() {
    return approvals.filter(input => input.checked).map(input => ({
      asset_id: input.dataset.id, format: input.dataset.format, sha256: input.dataset.sha256
    }));
  }
  function selectionChanged() {
    selectedCount.textContent = selected().length + ' reviewed files selected';
  }
  approvals.forEach(input => input.addEventListener('change', selectionChanged));
  document.getElementById('export-art-selection').addEventListener('click', () => {
    const choices = selected();
    if (!choices.length) {
      selectedCount.textContent = 'Select at least one reviewed file first.';
      return;
    }
    const payload = JSON.stringify({version: 1, approved_assets: choices}, null, 2);
    const blob = new Blob([payload], {type: 'application/json'});
    const url = URL.createObjectURL(blob);
    const link = document.createElement('a');
    link.href = url;
    link.download = 'moonfall_art_approval.json';
    document.body.appendChild(link);
    link.click();
    link.remove();
    setTimeout(() => URL.revokeObjectURL(url), 1000);
  });
  function update() {
    const query = search.value.trim().toLowerCase();
    let shown = 0;
    for (const card of cards) {
      const match = (status.value === 'all' || card.dataset.status === status.value)
        && (category.value === 'all' || card.dataset.category === category.value)
        && card.textContent.toLowerCase().includes(query);
      card.hidden = !match;
      if (match) shown++;
    }
    count.textContent = shown + ' of ' + cards.length + ' artworks shown';
  }
  for (const field of [search, status, category]) {
    field.addEventListener('input', update);
    field.addEventListener('change', update);
  }
  update();
})();
</script>"""
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
.filters {{display:flex;align-items:end;gap:.8rem;flex-wrap:wrap;margin:1.5rem 0}}
.filters label {{display:flex;flex-direction:column;gap:.35rem;font-weight:600}}
.filters input,.filters select {{font:inherit;background:#312b40;color:#fff;border:1px solid #76668e;padding:.55rem;border-radius:6px}}
.filters output {{font-size:.88rem;color:#d5c2ef}}
 .card[hidden] {{display:none}}
.review-choice {{display:flex;align-items:center;gap:.4rem;padding:.4rem 0;cursor:pointer;font-weight:600}}
.review-choice input {{width:1.15rem;height:1.15rem;accent-color:#c4abff}}
.approve-toolbar {{padding:1rem;margin-bottom:1.5rem;border:1px solid #76668e;border-radius:12px}}
.approve-toolbar button {{font:inherit;background:#a48ed6;color:#1e152b;border:0;border-radius:8px;padding:.65rem 1rem;font-weight:700;cursor:pointer}}
.approve-toolbar p {{font-size:.85rem;color:#e2d6f0}}
.approve-toolbar output {{margin-left:.7rem}}
.card a {{font-size:.85rem;color:#d5b8ff;display:block;margin:.3rem 0}}
:focus-visible {{outline:3px solid #f0ca96;outline-offset:3px}}
h2 {{font-size:1rem;margin:.65rem 0}} .card p {{font-size:.83rem;margin:.4rem 0}}
details {{font-size:.85rem;cursor:pointer}} .prompt {{white-space:pre-wrap}}
</style></head><body><header><h1>Moonfall AI Art Review Board</h1>
<p>{escape(summary)} · {len(rows)} planned creature artworks</p>
<p>Offline preview only. No images are generated or approved by this page.
Review each staged PNG or GLB, check only the art you accept, and export its
checksum-protected approval selection. Then explicitly approve it with the
offline command-line tool. This gallery never approves art or makes API calls.
Original Z and Nyra artwork is never used.</p></header>
{controls}
<main class="grid">{''.join(cards)}</main>{filter_script}</body></html>"""
    destination.write_text(document, encoding="utf-8")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--category", choices=("all", "guardians", "animals", *SLOTS), default="all")
    parser.add_argument("--gallery", action="store_true", help="Create offline review HTML and JSON plan")
    parser.add_argument("--audit", action="store_true", help="Return failure if any present artwork is invalid")
    args = parser.parse_args(argv)
    rows = plan_rows(args.category)
    counts = Counter(row.get("display_state", row["state"]) for row in rows)
    invalid = [row for row in rows if (row["state"] != "missing" and not row["valid"])
               or (row.get("model_state") not in ("missing", "not_applicable") and not row.get("model_valid"))]
    print(f"Moonfall art plan: {len(rows)} entries, {counts['approved']} approved, "
          f"{counts['staged']} staged, {counts['missing']} missing, {len(invalid)} invalid")
    if args.gallery:
        directory = studio.STAGING / "review"
        directory.mkdir(parents=True, exist_ok=True)
        (directory / "art_plan.json").write_text(json.dumps(rows, indent=2) + "\n", encoding="utf-8")
        make_gallery(rows, directory / "index.html")
        print(f"Review gallery: {directory / 'index.html'}")
    for row in invalid:
        print(f"INVALID {row['id']}: {row['issue'] or row.get('model_issue', '')}", file=sys.stderr)
    return 1 if args.audit and invalid else 0


if __name__ == "__main__":
    raise SystemExit(main())

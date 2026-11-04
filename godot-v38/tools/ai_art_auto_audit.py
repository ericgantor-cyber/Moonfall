#!/usr/bin/env python3
"""Read-only Moonfall AI art auto-audit: approved, staged and optional inbox.

Generates HTML/JSON/Markdown reports. No internet, model generation, approval,
billing or mutation of the art itself. Missing slots keep native art fallback.
"""
import argparse
from collections import Counter
from datetime import datetime, timezone
import hashlib
from html import escape
import json
from pathlib import Path
import struct

import ai_art_studio as studio
import ai_art_3d as mesh3d
import ai_art_autorig as autorig
import ai_art_rig as rig
import ai_art_intake as intake
import ai_art_release_audit as release
import ai_art_review as review

DEFAULT_REPORT_DIR = studio.STAGING / "review"


def check_file(item: dict, fmt: str, path: Path, state: str, variant: str = "combined") -> dict:
    record = {
        "asset_id": item["asset_id"], "category": item["category"],
        "format": fmt, "state": state, "variant": variant,
        "path": release.relative_asset(path), "valid": False, "bytes": 0,
        "sha256": "", "triangles": None, "bones": None, "clips": None,
        "texture_max": None, "verified_4k": None, "pbr_materials": None,
        "dimensions": None, "estimated_uncompressed_mipmap_bytes": 0,
        "errors": [], "warnings": [],
    }
    try:
        if path.is_symlink() or not path.is_file():
            raise ValueError("Art must be a real regular file, not a symlink.")
        raw = path.read_bytes()
        record["bytes"] = len(raw)
        digest = hashlib.sha256(raw).hexdigest()
        record["sha256"] = digest
        manifest_path = path.with_suffix(".json")
        if not manifest_path.is_file() or manifest_path.is_symlink():
            raise ValueError("Missing or linked review manifest.")
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        if (not isinstance(manifest, dict) or manifest.get("asset_id") != item["asset_id"]
                or manifest.get("sha256") != digest
                or manifest.get("reviewed") is not (state == "approved")):
            raise ValueError("Review manifest, checksum or approval status mismatch.")
        if fmt == "png":
            studio.validate_png(raw)
            record["dimensions"] = list(struct.unpack_from(">II", raw, 16))
            width, height = record["dimensions"]
            record["estimated_uncompressed_mipmap_bytes"] = width * height * 4 * 4 // 3
        else:
            mesh = mesh3d.glb_info(raw)
            animation = rig.rig_report(raw, require_rig=(fmt == "rigged"))
            record.update({
                "triangles": mesh["triangles"], "bones": animation["bone_count"],
                "clips": animation["animation_count"], "texture_max": mesh["max_texture_size"],
                "verified_4k": mesh["has_4k_texture"],
                "pbr_materials": mesh["pbr_material_count"],
            })
            record["estimated_uncompressed_mipmap_bytes"] = sum(
                width * height * 4 * 4 // 3
                for size in mesh.get("texture_dimensions", []) if size
                for width, height in [size]
            )
            if fmt == "rigged" and (not animation["rigged"] or not animation["mapped_clips"]):
                raise ValueError("Animated GLB requires an actual bound skeleton and named clips.")
            if animation["rigged"]:
                record["warnings"].extend(animation["warnings"])
            elif fmt == "glb" and item["category"] in ("animals", "guardians"):
                record["warnings"].append("Static creature model: original visual fallback remains available.")
            if not mesh["has_4k_texture"]:
                record["warnings"].append("4K texture not verified; this is optional, not a release failure.")
            if not mesh["pbr_material_count"]:
                record["warnings"].append("No complete normal + metallic/roughness PBR map set detected.")
            if mesh["triangles"] > release.WARNING_MODEL_TRIANGLES:
                record["warnings"].append("Large mesh: consider LOD when repeated many times.")
        if fmt == "rigged" and variant != "combined":
            record["warnings"].append("Separate animation variant; combine for the active gameplay rig.")
        record["valid"] = True
    except (OSError, ValueError, UnicodeError, TypeError, KeyError, OverflowError) as error:
        record["errors"].append(str(error))
    return record


def expected_paths(catalog: list[dict]) -> tuple[list[dict], set[Path], set[Path]]:
    records = []
    approved_paths: set[Path] = set()
    staged_paths: set[Path] = set()
    for item in catalog:
        formats = ["png"]
        if item["category"] in mesh3d.ALLOWED_MODEL_CATEGORIES:
            formats.append("glb")
        if item["category"] in ("animals", "guardians"):
            formats.append("rigged")
        for fmt in formats:
            if fmt == "png":
                stage, approved = studio.dest_paths(item)
                variations = [(stage, "combined")]
            elif fmt == "glb":
                stage, approved, _ = mesh3d.paths(item)
                variations = [(stage, "combined")]
            else:
                stage, approved, _ = autorig.paths(item)
                variations = [(autorig.paths(item, v)[0], v) for v in autorig.OPTIONS]
            approved_paths.add(approved)
            if approved.exists() or approved.is_symlink():
                records.append(check_file(item, fmt, approved, "approved"))
            for staged, variant in variations:
                staged_paths.add(staged)
                if staged.exists() or staged.is_symlink():
                    records.append(check_file(item, fmt, staged, "staged", variant))
    return records, approved_paths, staged_paths


def find_unregistered(root: Path, allowed: set[Path], kind: str) -> list[dict]:
    if not root.exists():
        return []
    if root.is_symlink() or not root.is_dir():
        return [{"area": kind, "path": str(root), "error": "Invalid or linked art root."}]
    found = []
    for path in sorted(root.rglob("*")):
        if path.suffix.lower() in (".png", ".glb") and path not in allowed:
            found.append({"area": kind, "path": release.relative_asset(path),
                          "error": "Unregistered art file bypasses the review catalog."})
    return found


def inbox_check(directory: Path) -> tuple[list[dict], list[dict]]:
    if not directory.exists():
        return [], []
    try:
        entries = intake.list_inbox(directory)
    except (OSError, ValueError) as error:
        return [], [{"area": "inbox", "path": str(directory), "error": str(error)}]
    records = []
    for path, item, extension in entries:
        result = {"asset_id": item["asset_id"], "path": str(path),
                  "format": extension.lstrip("."), "valid": False}
        try:
            raw = path.read_bytes()
            if extension == ".png":
                studio.validate_png(raw)
                result["dimensions"] = list(struct.unpack_from(">II", raw, 16))
            else:
                geometry = mesh3d.glb_info(raw)
                animation = rig.rig_report(raw, require_rig=extension == ".rigged")
                result["triangles"] = geometry["triangles"]
                result["bones"] = animation["bone_count"]
                result["clips"] = animation["animation_count"]
            result["valid"] = True
            result["sha256"] = hashlib.sha256(raw).hexdigest()
        except (OSError, ValueError, TypeError, KeyError) as error:
            result["error"] = str(error)
        records.append(result)
    return records, []


def audit(include_inbox: bool = False, inbox_path: Path | None = None) -> dict:
    catalog = review.all_items()
    records, known_approved, known_staged = expected_paths(catalog)
    errors = find_unregistered(studio.ROOT, known_approved, "approved")
    errors.extend(find_unregistered(studio.STAGING, known_staged, "staged"))
    inbox, inbox_errors = inbox_check(inbox_path or studio.PROJECT / "ai_art_inbox") if include_inbox else ([], [])
    errors.extend(inbox_errors)
    for item in records:
        for msg in item["errors"]:
            errors.append({"area": item["state"], "asset_id": item["asset_id"],
                           "path": item["path"], "error": msg})
    for item in inbox:
        if not item["valid"]:
            errors.append({"area": "inbox", "asset_id": item["asset_id"],
                           "path": item["path"], "error": item["error"]})
    warnings = [{"asset_id": r["asset_id"], "path": r["path"], "message": msg}
                for r in records for msg in r["warnings"]]
    approved = [r for r in records if r["state"] == "approved" and r["valid"]]
    # Duplicate approved artworks are legal, but unnecessary duplicates
    # often inflate the installer and hide accidental asset reuse.
    hashes: dict[str, list[str]] = {}
    for item in approved:
        hashes.setdefault(item["sha256"], []).append(item["asset_id"] + "." + item["format"])
    for ids in hashes.values():
        if len(ids) > 1:
            warnings.append({"asset_id": ids[0], "path": "",
                             "message": "Identical bytes used in " + str(len(ids)) +
                             " approved slots. Check intentional reuse."})
    staged = [r for r in records if r["state"] == "staged" and r["valid"]]
    counts = Counter((r["state"], r["format"]) for r in records)
    return {
        "schema": "moonfall.ai_art.auto_audit.v1",
        "timestamp_utc": datetime.now(timezone.utc).isoformat(),
        "read_only": True, "paid_api_calls": 0,
        "catalog_slots": len(catalog),
        "approved_slots": len({r["asset_id"] for r in approved}),
        "approved_files": len(approved), "staged_files": len(staged),
        "inbox_files": len(inbox),
        "verified_4k_glbs": sum(r["verified_4k"] is True for r in approved),
        "animated_rigs": sum(r["format"] == "rigged" and bool(r["clips"]) for r in approved),
        "approved_source_bytes": sum(r["bytes"] for r in approved),
        "estimated_uncompressed_mipmap_bytes": sum(
            r["estimated_uncompressed_mipmap_bytes"] for r in approved),
        "format_counts": {f"{a}_{b}": n for (a, b), n in counts.items()},
        "assets": records, "inbox": inbox, "errors": errors, "warnings": warnings,
        "summary": "FAIL" if errors else "REVIEW WARNINGS" if warnings else "PASS",
    }


def html_dashboard(report: dict) -> str:
    rows = []
    for r in sorted(report["assets"], key=lambda v: (v["state"], v["category"], v["asset_id"])):
        flag = "ERROR" if r["errors"] else "REVIEW" if r["warnings"] else "OK"
        features = []
        if r["triangles"] is not None:
            features.append(str(r["triangles"]) + " triangles")
            features.append(str(r["bones"]) + " bones")
            features.append(str(r["clips"]) + " clips")
            features.append("4K verified" if r["verified_4k"] else "4K unverified")
        if r["dimensions"]:
            features.append(str(r["dimensions"][0]) + "x" + str(r["dimensions"][1]))
        cells = [r["asset_id"], r["category"], r["format"], r["state"],
                 " · ".join(features), flag, "; ".join(r["errors"] or r["warnings"])]
        rows.append('<tr data-flag="' + escape(flag.lower()) + '" data-format="' +
                    escape(r["format"]) + '">' + "".join(
                        "<td>" + escape(str(value)) + "</td>" for value in cells
                    ) + "</tr>")
    failures = "".join("<li>" + escape(e.get("path", "") + ": " + e["error"]) + "</li>"
                       for e in report["errors"])
    # CSS is static; untrusted asset data is escaped before insertion.
    return """<!doctype html><html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Moonfall Art Auto Audit</title>
<style>
:root{color-scheme:dark}body{background:#15121c;color:#f6f0ff;font:16px system-ui,sans-serif;margin:0;padding:1.2rem}
main{max-width:1360px;margin:auto}h1{font-size:1.7rem}a{color:#dec7ff}
.summary{display:flex;gap:.7rem;flex-wrap:wrap;margin:1rem 0}
.stat{border:1px solid #74618a;background:#282132;padding:.8rem;border-radius:.65rem;min-width:8rem}
.stat strong{display:block;font-size:1.4rem}
.controls{display:flex;flex-wrap:wrap;gap:.7rem;margin:1rem 0;align-items:center}
input,select{background:#30263c;color:#fff;font:inherit;padding:.5rem;border:1px solid #b29ad0;border-radius:.35rem}
input{flex:1;min-width:12rem}table{border-collapse:collapse;width:100%;font-size:.88rem}
th,td{text-align:left;padding:.6rem;border-bottom:1px solid #51435c;vertical-align:top}
th{position:sticky;top:0;background:#292333}tr[hidden]{display:none}
tbody tr:nth-child(even){background:#211b2a} :focus-visible{outline:3px solid #ffdc8a}
.scroll{overflow:auto;max-height:70vh}.error{color:#ffc3c3}
</style></head><body><main>
<h1>Moonfall AI Art automatic audit</h1>
<p>Generated: """ + escape(report["timestamp_utc"]) + """ · Status: <strong>""" + escape(report["summary"]) + """</strong></p>
<div class="summary" aria-label="Art quality metrics">
""" + "".join('<div class="stat"><strong>' + str(number) + '</strong>' + escape(label) + '</div>'
               for label, number in [
                   ("Approved slots", str(report["approved_slots"]) + "/" + str(report["catalog_slots"])),
                   ("Staged art", report["staged_files"]),
                   ("4K-verified GLBs", report["verified_4k_glbs"]),
                   ("Animated rigs", report["animated_rigs"]),
                   ("Raw source MiB", round(report["approved_source_bytes"] / 1048576, 1)),
                   ("Texture estimate MiB", round(report["estimated_uncompressed_mipmap_bytes"] / 1048576, 1)),
                   ("Integrity errors", len(report["errors"])),
                   ("Quality warnings", len(report["warnings"])),
               ]) + """</div>
<p>Read-only and offline. Missing art keeps the game's original visuals.
Unverified 4K, missing optional clips and high polycounts are advisory warnings, not bans.</p>
<h2>Integrity findings</h2>""" + (
    '<ul class="error">' + failures + '</ul>' if failures else "<p>No integrity errors.</p>"
) + """<h2>Asset details</h2>
<div class="controls">
<label for="search">Search</label><input id="search" type="search" placeholder="Find a model, warning or category">
<label for="flag">Findings</label><select id="flag"><option value="all">All</option>
<option value="error">Errors</option><option value="review">Warnings</option><option value="ok">OK</option></select>
<label for="format">Format</label><select id="format"><option value="all">All</option>
<option value="png">PNG</option><option value="glb">GLB</option><option value="rigged">Rigged</option></select>
</div><output id="count" aria-live="polite"></output><div class="scroll" tabindex="0" aria-label="Artwork table">
<table><thead><tr><th scope="col">Asset</th><th scope="col">Category</th><th scope="col">Format</th>
<th scope="col">Stage</th><th scope="col">Model details</th><th scope="col">Status</th><th scope="col">Findings</th>
</tr></thead><tbody>""" + "".join(rows) + """</tbody></table></div>
<p>Texture memory, animation appearance and real FPS require on-device Godot testing.
This metadata report never makes paid provider requests.</p></main><script>
(() => {
  const rows = [...document.querySelectorAll('tbody tr')];
  const q = document.querySelector('#search');
  const flag = document.querySelector('#flag');
  const format = document.querySelector('#format');
  const output = document.querySelector('#count');
  function update() {
    let visible = 0;
    for (const row of rows) {
      const show = row.textContent.toLowerCase().includes(q.value.toLowerCase())
        && (flag.value === 'all' || row.dataset.flag === flag.value)
        && (format.value === 'all' || row.dataset.format === format.value);
      row.hidden = !show;
      if (show) visible++;
    }
    output.textContent = visible + ' assets visible';
  }
  [q,flag,format].forEach(el => el.addEventListener('input', update));
  update();
})();
</script></body></html>"""


def markdown_summary(report: dict) -> str:
    lines = ["## Moonfall AI Art automatic audit",
             "**" + report["summary"] + "** | " + str(report["approved_files"]) +
             " approved | " + str(report["staged_files"]) + " staged | " +
             str(len(report["errors"])) + " errors | " + str(len(report["warnings"])) + " warnings",
             "Raw RGBA mip estimate (not actual GPU VRAM): " +
             str(round(report["estimated_uncompressed_mipmap_bytes"] / 1048576, 1)) + " MiB",
             "Verified 4K models: " + str(report["verified_4k_glbs"]) +
             " | animated rigs: " + str(report["animated_rigs"]),
             "Offline and read-only; no paid requests."]
    for issue in report["errors"][:20]:
        lines.append("- ERROR " + issue.get("path", "") + ": " + issue["error"])
    for issue in report["warnings"][:10]:
        lines.append("- REVIEW " + issue["asset_id"] + ": " + issue["message"])
    return "\n".join(lines) + "\n"


def save_reports(report: dict, directory: Path | None = None) -> dict[str, Path]:
    folder = directory or DEFAULT_REPORT_DIR
    folder.mkdir(parents=True, exist_ok=True)
    paths = {"json": folder / "auto_audit.json",
             "html": folder / "auto_audit.html",
             "markdown": folder / "auto_audit.md"}
    paths["json"].write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    paths["html"].write_text(html_dashboard(report), encoding="utf-8")
    paths["markdown"].write_text(markdown_summary(report), encoding="utf-8")
    return paths


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--strict", action="store_true", help="Fail on corrupt, unregistered or unreviewed art")
    parser.add_argument("--inbox", action="store_true", help="Also validate original incoming files")
    parser.add_argument("--inbox-path", type=Path)
    parser.add_argument("--output-dir", type=Path)
    parser.add_argument("--json", action="store_true", help="Print compact JSON summary")
    args = parser.parse_args(argv)
    report = audit(args.inbox, args.inbox_path)
    paths = save_reports(report, args.output_dir)
    if args.json:
        print(json.dumps({"status": report["summary"], "errors": len(report["errors"]),
                          "warnings": len(report["warnings"]),
                          "reports": {k: str(v) for k, v in paths.items()}}))
    else:
        print(markdown_summary(report))
        print("Local dashboard:", paths["html"])
    return 1 if args.strict and report["errors"] else 0


if __name__ == "__main__":
    raise SystemExit(main())

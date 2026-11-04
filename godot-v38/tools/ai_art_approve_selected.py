#!/usr/bin/env python3
"""Explicit review-selected bulk approval of Moonfall art, fully offline.

Gallery checkboxes create a signed-by-content SHA-256 selection list. This
script validates all selected items first, then imports ONLY those reviewed
PNG/GLB assets into Godot's approved asset folders. Never submits paid work.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import sys

import ai_art_review as review
import ai_art_studio as png
import ai_art_3d as glb
import ai_art_autorig as autorig
from ai_art_rig import rig_report


MAX_SELECTION_BYTES = 2 * 1024 * 1024  # Prevent corrupted/oversized JSON; not an artwork count cap.
SHA256_RE = re.compile(r"^[a-f0-9]{64}$")


def read_selection(path: Path) -> list[dict]:
    if not path.is_file() or path.is_symlink() or path.stat().st_size > MAX_SELECTION_BYTES:
        raise ValueError("Provide a normal JSON approval file under 2 MiB.")
    try:
        selection = json.loads(path.read_text(encoding="utf-8"))
    except (UnicodeError, json.JSONDecodeError) as error:
        raise ValueError("Approval file is not valid UTF-8 JSON.") from error
    if not isinstance(selection, dict) or selection.get("version") != 1:
        raise ValueError("Approval selection must be a version-1 manifest.")
    entries = selection.get("approved_assets")
    if not isinstance(entries, list) or not entries:
        raise ValueError("Select at least one known artwork file in the gallery.")
    seen: set[tuple[str, str]] = set()
    available = {item["asset_id"]: item for item in review.all_items()}
    if len(entries) > 3 * len(available):
        raise ValueError("Approval manifest contains more entries than possible known PNG/GLB/rigged assets.")
    result = []
    for raw in entries:
        if not isinstance(raw, dict) or set(raw) != {"asset_id", "format", "sha256"}:
            raise ValueError("Each selected entry needs asset_id, format and sha256.")
        asset_id, fmt, digest = raw["asset_id"], raw["format"], raw["sha256"]
        if not isinstance(asset_id, str) or asset_id not in available:
            raise ValueError("Unknown or forbidden art asset. Hero artwork is never eligible.")
        if fmt not in ("png", "glb", "rigged") or not isinstance(digest, str) or not SHA256_RE.fullmatch(digest):
            raise ValueError("Invalid art format or checksum.")
        if fmt == "rigged" and available[asset_id]["category"] not in ("animals", "guardians"):
            raise ValueError("Animated rig overlays are supported only for approved creature categories.")
        if fmt == "glb" and available[asset_id]["category"] not in glb.ALLOWED_MODEL_CATEGORIES:
            raise ValueError("UI, spell and region art cannot be approved as 3D meshes.")
        key = (asset_id, fmt)
        if key in seen:
            raise ValueError("Repeated approval entry for one asset.")
        seen.add(key)
        result.append({"item": available[asset_id], "format": fmt, "sha256": digest})
    return result


def verify_all(entries: list[dict]) -> list[dict]:
    """Preflight every file before a single item is approved."""
    for entry in entries:
        item, fmt = entry["item"], entry["format"]
        staged, approved = (png.dest_paths(item) if fmt == "png" else
                            autorig.paths(item)[:2] if fmt == "rigged" else glb.paths(item)[:2])
        if approved.exists() or approved.is_symlink():
            raise FileExistsError(f"Reviewed art already approved: {item['asset_id']}.{fmt}")
        if not staged.is_file() or staged.is_symlink():
            raise FileNotFoundError(f"Missing staged {fmt}: {item['asset_id']}")
        payload = staged.read_bytes()
        if fmt == "png":
            png.validate_png(payload)
        else:
            glb.glb_info(payload)
            if fmt == "rigged":rig_report(payload, require_rig=True)
        actual_sha = hashlib.sha256(payload).hexdigest()
        if actual_sha != entry["sha256"]:
            raise ValueError(f"Artwork changed after gallery review: {item['asset_id']}.{fmt}")
        record = staged.with_suffix(".json")
        if not record.is_file() or record.is_symlink():
            raise ValueError(f"Missing provenance record: {item['asset_id']}.{fmt}")
        metadata = json.loads(record.read_text(encoding="utf-8"))
        if (not isinstance(metadata, dict) or metadata.get("asset_id") != item["asset_id"]
                or metadata.get("sha256") != actual_sha or metadata.get("reviewed") is not False):
            raise ValueError(f"Artwork manifest does not match: {item['asset_id']}.{fmt}")
    return entries


def apply(entries: list[dict]) -> list[Path]:
    verify_all(entries)
    # After full validation, each approved file is placed in its fixed native
    # category. Godot picks it up on editor import / subsequent game build.
    destinations = []
    for entry in entries:
        item = entry["item"]
        if entry["format"] == "png":
            destinations.append(png.approve(item))
        elif entry["format"] == "rigged":
            destinations.append(autorig.approve(item, "combined"))
        else:
            destinations.append(glb.approve_model(item))
    return destinations


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, required=True, help="Review gallery's downloaded selection JSON")
    parser.add_argument("--apply", action="store_true", help="Explicitly approve validated selections; default is preview")
    args = parser.parse_args(argv)
    try:
        selected = verify_all(read_selection(args.manifest))
        if not args.apply:
            print(f"{len(selected)} reviewed artworks eligible. Preview only, nothing changed.")
            print("Run again with --apply after confirming the selection and source files.")
            return 0
        output = apply(selected)
        print(f"Approved {len(output)} reviewed files for Godot's next asset import.")
        for destination in output:
            print(destination)
        return 0
    except (ValueError, OSError, FileNotFoundError, FileExistsError) as error:
        print(f"Moonfall art approval rejected: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())

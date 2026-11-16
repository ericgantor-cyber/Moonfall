#!/usr/bin/env python3
"""Moonfall approved-art release audit, entirely offline and read-only.

Checks every present reviewed PNG and GLB against its original checksum and
format; reports actual 4K texture dimensions and an estimated *uncompressed*
texture memory cost. Never creates art or modifies existing assets.
"""
import argparse
from collections import Counter, defaultdict
import hashlib
import json
import os
from pathlib import Path
import struct
import sys

import ai_art_studio as png
import ai_art_3d as glb
import ai_art_rig as rig
import ai_art_autorig as autorig
import ai_art_review as review


WARNING_TEXTURE_MB = 384
WARNING_MODEL_TRIANGLES = 25000
WARNING_APPROVED_SIZE_MB = 1024


def relative_asset(path: Path) -> str:
    return os.path.relpath(path, png.PROJECT).replace("\\", "/")


def verify_asset(item: dict, fmt: str, destination: Path) -> dict:
    """Inspect approved bytes, matching review manifest and current structure."""
    payload = destination.read_bytes()
    digest = hashlib.sha256(payload).hexdigest()
    record_file = destination.with_suffix(".json")
    if not record_file.is_file() or record_file.is_symlink():
        raise ValueError("Missing or linked provenance metadata.")
    record = json.loads(record_file.read_text(encoding="utf-8"))
    if not isinstance(record, dict) or record.get("asset_id") != item["asset_id"]:
        raise ValueError("Asset identifier does not match the known registry slot.")
    if record.get("reviewed") is not True or record.get("sha256") != digest:
        raise ValueError("Asset checksum or review approval is invalid.")
    result = {
        "asset_id": item["asset_id"],
        "category": item["category"],
        "format": fmt, "path": relative_asset(destination),
        "sha256": digest, "bytes": len(payload),
        "estimated_rgba_mipmap_bytes": 0,
        "warnings": [],
    }
    if fmt == "png":
        png.validate_png(payload)
        width, height = struct.unpack_from(">II", payload, 16)
        result["width"] = width
        result["height"] = height
        # Estimate raw, uncompressed RGBA8 with all mip levels (4/3 pixels).
        result["estimated_rgba_mipmap_bytes"] = int(width * height * 4 * 4 / 3)
    elif fmt in ("glb","rigged"):
        info = glb.glb_info(payload)
        result["mesh"] = info
        rig_data = rig.rig_report(payload, require_rig=fmt=="rigged")
        result["rig"] = rig_data
        if item["category"] in ("guardians", "animals"):
            if not rig_data["rigged"]:
                result["warnings"].append("Static creature mesh: no bound skeleton; animation uses native fallback.")
            else:
                result["warnings"].extend(rig_data["warnings"])
        for dims in info.get("texture_dimensions", []):
            if not dims or len(dims) != 2:
                continue
            width, height = dims
            result["estimated_rgba_mipmap_bytes"] += int(width * height * 4 * 4 / 3)
        if info["triangles"] > WARNING_MODEL_TRIANGLES:
            result["warnings"].append(
                f"High mesh complexity: {info['triangles']} triangles; prefer <{WARNING_MODEL_TRIANGLES:,} for repeated NPCs."
            )
        if not info.get("has_4k_texture"):
            result["warnings"].append(
                "No observed embedded 4096x4096 texture map (4K was not verified)."
            )
        if not info.get("pbr_material_count"):
            result["warnings"].append(
                "No complete normal + metallic/roughness material mapping was detected."
            )
    else:
        raise ValueError("Unsupported reviewed artwork format.")
    return result


def audit() -> dict:
    approved: list[dict] = []
    failures: list[dict] = []
    selected_assets: set[str] = set()
    hashes: dict[str, list[str]] = defaultdict(list)
    for item in review.all_items():
        present = []
        image = png.dest_paths(item)[1]
        if image.exists() or image.is_symlink():
            present.append(("png", image))
        if item["category"] in glb.ALLOWED_MODEL_CATEGORIES:
            model = glb.paths(item)[1]
            if model.exists() or model.is_symlink():
                present.append(("glb", model))
        if item["category"] in ("guardians","animals"):
            rigged = autorig.paths(item)[1]
            if rigged.exists() or rigged.is_symlink():
                present.append(("rigged", rigged))
        if present:
            selected_assets.add(item["asset_id"])
        for fmt, target in present:
            try:
                if target.is_symlink() or not target.is_file():
                    raise ValueError("Approved artwork must be a real regular file.")
                result = verify_asset(item, fmt, target)
                approved.append(result)
                hashes[result["sha256"]].append(result["asset_id"] + "." + fmt)
            except (ValueError, OSError, json.JSONDecodeError, TypeError, KeyError) as error:
                failures.append({
                    "asset_id": item["asset_id"], "format": fmt,
                    "path": relative_asset(target), "error": str(error),
                })
    total_source_bytes = sum(item["bytes"] for item in approved)
    uncompressed_texture_bytes = sum(item["estimated_rgba_mipmap_bytes"] for item in approved)
    report = {
        "catalog_slots": len(review.all_items()),
        "occupied_slots": len(selected_assets),
        "approved_files": len(approved),
        "source_bytes": total_source_bytes,
        "estimated_uncompressed_rgba_mipmap_bytes": uncompressed_texture_bytes,
        "format_counts": dict(Counter(item["format"] for item in approved)),
        "assets": approved,
        "errors": failures,
        "warnings": [],
    }
    # This estimate is NOT measured VRAM: actual compression/sharing, sampling,
    # import settings, GPU residency and mipmaps vary by Godot project/build.
    if uncompressed_texture_bytes > WARNING_TEXTURE_MB * 1024 * 1024:
        report["warnings"].append(
            f"Combined source map footprint exceeds {WARNING_TEXTURE_MB} MiB uncompressed. "
            "Use Godot texture import compression, mipmaps and near-player loading."
        )
    if total_source_bytes > WARNING_APPROVED_SIZE_MB * 1024 * 1024:
        report["warnings"].append(
            "Approved artwork source bundle exceeds 1 GiB; check Windows installer size."
        )
    for digest, ids in hashes.items():
        if len(ids) > 1:
            report["warnings"].append(
                f"Identical approved art reused for {len(ids)} slots: {', '.join(ids[:4])}."
            )
    return report


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--json", action="store_true", help="Print complete machine-readable report")
    parser.add_argument("--strict", action="store_true", help="Fail if any present approved file is corrupt or unreviewed")
    parser.add_argument("--require-animated-creatures", action="store_true", help="Opt-in gate: fail if approved creature GLBs lack a usable skinned animation rig")
    parser.add_argument("--report", type=Path, help="Optional path for a JSON report (never edits actual art)")
    args = parser.parse_args(argv)
    report = audit()
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    if args.json:
        print(json.dumps(report, indent=2))
    else:
        print(f"Moonfall approved art: {report['approved_files']} files over "
              f"{report['occupied_slots']} of {report['catalog_slots']} slots")
        print(f"Source assets: {report['source_bytes'] / 1048576:.1f} MiB; "
              f"estimated raw RGBA mip footprint: "
              f"{report['estimated_uncompressed_rgba_mipmap_bytes'] / 1048576:.1f} MiB")
        for warning in report["warnings"]:
            print("WARNING:", warning)
        for asset in report["assets"]:
            for warning in asset["warnings"]:
                print(f"ART WARNING {asset['asset_id']}.{asset['format']}: {warning}")
        for error in report["errors"]:
            print(f"INVALID: {error['asset_id']}.{error['format']}: {error['error']}",
                  file=sys.stderr)
    if args.require_animated_creatures:
        # A valid separately approved rig satisfies the corresponding static
        # creature's readiness without forcing deletion of the original art.
        rigged_ids = {asset["asset_id"] for asset in report["assets"]
                      if asset["format"] == "rigged"}
        for asset in report["assets"]:
            if asset["category"] not in ("animals","guardians") or asset["format"] == "png":
                continue
            if asset["format"] == "glb" and asset["asset_id"] in rigged_ids:
                continue
            animated = asset.get("rig", {})
            if (not animated.get("rigged") or not animated.get("mapped_clips")
                    or animated.get("missing_preferred")):
                print("MISSING RIG:", asset["asset_id"], file=sys.stderr)
                return 1
    return 1 if args.strict and report["errors"] else 0


if __name__ == "__main__":
    raise SystemExit(main())

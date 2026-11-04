#!/usr/bin/env python3
"""Offline, read-only optimization priorities from Moonfall AI art audit metadata."""
import argparse
import csv
import json
from pathlib import Path
import sys

MIB = 1048576
FIELDS = ("priority", "asset_id", "category", "format", "state", "source_mib",
          "estimated_texture_mib", "triangles", "bones", "clips", "action")


def make_plan(report, texture_warn_mib=64, triangle_warn=100000):
    if report.get("schema") != "moonfall.ai_art.auto_audit.v1" or not isinstance(report.get("assets"), list):
        raise ValueError("Expected Moonfall AI Art auto-audit v1 JSON")
    if texture_warn_mib <= 0 or triangle_warn <= 0:
        raise ValueError("Thresholds must be positive")
    rows = []
    for asset in report["assets"]:
        if not isinstance(asset, dict):
            raise ValueError("Invalid asset record")
        if asset.get("state") != "approved":
            continue  # Never count unreviewed art as shipped content.
        actions = []
        if asset.get("errors") or not asset.get("valid", False):
            priority = 0
            actions.append("Repair integrity and provenance before shipping")
        else:
            texture = asset.get("estimated_uncompressed_mipmap_bytes") or 0
            triangles = asset.get("triangles") or 0
            if texture >= texture_warn_mib * MIB:
                actions.append("Review texture downscaling/compression and device GPU memory")
            if triangles >= triangle_warn:
                actions.append("Profile repeated instances; consider distant LOD meshes")
            if asset.get("format") == "rigged":
                if not asset.get("clips"):
                    actions.append("Check animation clips and native fallback")
                if (asset.get("bones") or 0) >= 256:
                    actions.append("Profile skeleton cost with many visible actors")
            priority = 1 if actions else 3
        if not actions:
            actions.append("No metadata flags; verify runtime frame times on Windows")
        rows.append({
            "priority": priority, "asset_id": str(asset.get("asset_id", "")),
            "category": str(asset.get("category", "")),
            "format": str(asset.get("format", "")), "state": "approved",
            "source_mib": round((asset.get("bytes") or 0) / MIB, 2),
            "estimated_texture_mib": round((asset.get("estimated_uncompressed_mipmap_bytes") or 0) / MIB, 2),
            "triangles": asset.get("triangles") or 0, "bones": asset.get("bones") or 0,
            "clips": asset.get("clips") or 0, "action": "; ".join(actions),
        })
    rows.sort(key=lambda a: (a["priority"], -a["estimated_texture_mib"],
                             -a["triangles"], a["asset_id"], a["format"]))
    return {"schema": "moonfall.ai_art.budget.v1", "source_schema": report["schema"],
            "thresholds": {"texture_warn_mib": texture_warn_mib, "triangle_warn": triangle_warn},
            "approved_assets": len(rows),
            "needs_repair": sum(a["priority"] == 0 for a in rows),
            "needs_profiling": sum(a["priority"] == 1 for a in rows),
            "note": "Metadata estimates, not measured VRAM, FPS or visual quality. No artwork modified.",
            "assets": rows}


def write_reports(plan, output):
    output.mkdir(parents=True, exist_ok=True)
    (output / "art_budget.json").write_text(json.dumps(plan, indent=2) + "\n", encoding="utf-8")
    with (output / "art_budget.csv").open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=FIELDS)
        writer.writeheader()
        writer.writerows(plan["assets"])
    lines = ["# Moonfall AI Art performance budget", "",
             f"Approved: {plan['approved_assets']} | Repair: {plan['needs_repair']} | Profile: {plan['needs_profiling']}",
             "", plan["note"], ""]
    lines += [f"- P{a['priority']} `{a['asset_id']}` ({a['format']}): {a['action']}"
              for a in plan["assets"] if a["priority"] < 3]
    if not any(a["priority"] < 3 for a in plan["assets"]):
        lines.append("No metadata budget flags. Test frame times on target hardware.")
    (output / "art_budget.md").write_text("\n".join(lines) + "\n", encoding="utf-8")


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--audit", type=Path, default=Path("ai_art_staging/review/auto_audit.json"))
    parser.add_argument("--output-dir", type=Path, default=Path("ai_art_staging/review"))
    parser.add_argument("--texture-warn-mib", type=int, default=64)
    parser.add_argument("--triangle-warn", type=int, default=100000)
    args = parser.parse_args(argv)
    try:
        report = json.loads(args.audit.read_text(encoding="utf-8"))
        plan = make_plan(report, args.texture_warn_mib, args.triangle_warn)
        write_reports(plan, args.output_dir)
    except (OSError, ValueError, TypeError, KeyError) as error:
        print(f"Moonfall art budget: {error}", file=sys.stderr)
        return 1
    print(f"Moonfall art budget: {plan['approved_assets']} approved, "
          f"{plan['needs_repair']} repair, {plan['needs_profiling']} profile")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

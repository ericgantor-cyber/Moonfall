#!/usr/bin/env python3
"""Read-only Moonfall original-art manifest integrity and coverage check."""
import argparse
import hashlib
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "engine-comparison" / "ART_IMPORT_MANIFEST.json"
IMAGES = {".png", ".webp", ".jpg", ".jpeg", ".bmp", ".tga", ".gif", ".svg", ".tif", ".tiff", ".exr", ".hdr"}
HEX64 = re.compile(r"^[a-f0-9]{64}$")


def check():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check-exports", action="store_true",
                        help="Also compare declared Unity/Unreal prototype converted images.")
    args = parser.parse_args()
    errors = []
    try:
        manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    except (OSError, ValueError) as exc:
        print(f"FAIL: cannot read {MANIFEST}: {exc}", file=sys.stderr)
        return 1
    assets = manifest.get("assets")
    if not isinstance(assets, list):
        print("FAIL: assets must be a list", file=sys.stderr)
        return 1
    seen = set()
    source_root = ROOT / "godot-v38" / "assets"

    def verify_file(path_text, expected, label):
        if not isinstance(path_text, str) or not isinstance(expected, str) or not HEX64.fullmatch(expected):
            errors.append(f"{label}: invalid path or SHA-256")
            return
        relative = Path(path_text)
        path = ROOT / relative
        if relative.is_absolute() or ".." in relative.parts or not path.is_file():
            errors.append(f"{label}: missing or invalid path: {path_text}")
            return
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        if digest != expected:
            errors.append(f"{label}: hash mismatch: {path_text}")

    for item in assets:
        if not isinstance(item, dict):
            errors.append("invalid asset entry")
            continue
        source = item.get("source")
        if not isinstance(source, str) or not source.startswith("godot-v38/assets/"):
            errors.append(f"invalid source: {source!r}")
            continue
        if source in seen:
            errors.append(f"duplicate source: {source}")
        seen.add(source)
        verify_file(source, item.get("source_sha256"), "source")
        if args.check_exports:
            for output in item.get("outputs", []):
                if not isinstance(output, dict):
                    errors.append(f"invalid export for {source}")
                    continue
                verify_file(output.get("path"), output.get("sha256"), "export")

    if manifest.get("source_count") != len(assets):
        errors.append("source_count does not match manifest entries")
    if args.check_exports and manifest.get("imported_files") != sum(len(a.get("outputs", [])) for a in assets if isinstance(a, dict)):
        errors.append("imported_files does not match manifest outputs")
    for path in source_root.rglob("*"):
        if path.is_file() and path.suffix.lower() in IMAGES:
            name = path.relative_to(ROOT).as_posix()
            if name not in seen:
                errors.append(f"unmanifested source image: {name}")

    if errors:
        for message in errors:
            print("FAIL:", message)
        print(f"FAIL: {len(errors)} issue(s) among {len(assets)} declared original images")
        return 1
    print(f"PASS: {len(assets)} declared original images verified; no additional image files found")
    if args.check_exports:
        print("PASS: all declared conversion exports verified")
    return 0


if __name__ == "__main__":
    raise SystemExit(check())

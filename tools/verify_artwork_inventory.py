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
# Independent original-art SHA-256 values, never read from the import manifest.
ORIGINAL_HERO_SHA256 = {
    "godot-v38/assets/heroes/Z_Original.webp": "81b8ac6bae0830ab8d8bf53d2cdcbc8bae07256a8b7a9d4398ce4591bafd0666",
    "godot-v38/assets/heroes/Nyra_Original.png": "64aa23e2de7ea0e07347171c3ebe4beae13166359b08855d1076db9dd25871a9",
}


def check():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check-exports", action="store_true",
                        help="Also compare declared Unity/Unreal prototype converted images.")
    parser.add_argument("--require-original-heroes", action="store_true",
                        help="Enforce fixed Z and Nyra source-art hashes.")
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
    seen_exports = set()
    source_root = ROOT / "godot-v38" / "assets"

    def verify_file(path_text, expected, label):
        if not isinstance(path_text, str) or not isinstance(expected, str) or not HEX64.fullmatch(expected):
            errors.append(f"{label}: invalid path or SHA-256")
            return
        relative = Path(path_text)
        path = ROOT / relative
        if (relative.is_absolute() or ".." in relative.parts or not path.is_file()
                or not path.resolve().is_relative_to(ROOT.resolve())):
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
        outputs = item.get("outputs")
        if not isinstance(outputs, list) or len(outputs) != 2:
            errors.append(f"expected two conversion exports for {source}")
            continue
        engines = set()
        for output in outputs:
            if not isinstance(output, dict):
                errors.append(f"invalid export for {source}")
                continue
            engine = output.get("engine")
            dest = output.get("path")
            if engine not in ("unity", "unreal_source_art"):
                errors.append(f"unexpected export engine for {source}: {engine!r}")
            if engine in engines:
                errors.append(f"duplicate export engine for {source}: {engine}")
            engines.add(engine)
            if not isinstance(dest, str):
                errors.append(f"invalid export destination for {source}")
                continue
            if dest in seen_exports:
                errors.append(f"duplicate export destination: {dest}")
            seen_exports.add(dest)
            expected_prefix = {
                "unity": "engine-comparison/unity/Assets/Resources/MoonfallArt/",
                "unreal_source_art": "engine-comparison/unreal/MoonfallPrototype/SourceArt/MoonfallArt/",
            }.get(engine)
            if expected_prefix and not dest.startswith(expected_prefix):
                errors.append(f"unexpected export location for {source}: {dest}")
            if args.check_exports:
                verify_file(dest, output.get("sha256"), "export")
        if engines != {"unity", "unreal_source_art"}:
            errors.append(f"missing required export engine for {source}")

    if args.require_original_heroes:
        known = {item.get("source"): item.get("source_sha256")
                 for item in assets if isinstance(item, dict)
                 and isinstance(item.get("source"), str)}
        for hero, golden_hash in ORIGINAL_HERO_SHA256.items():
            if known.get(hero) != golden_hash:
                errors.append(f"original hero baseline mismatch or omitted: {hero}")
            verify_file(hero, golden_hash, "protected original hero")
    if manifest.get("source_count") != len(assets):
        errors.append("source_count does not match manifest entries")
    if manifest.get("imported_files") != len(seen_exports):
        errors.append("imported_files does not match manifest outputs")
    if not source_root.is_dir():
        errors.append("missing source artwork directory")
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

#!/usr/bin/env python3
"""Import all tracked Godot visual assets into the isolated Unity/Unreal experiments.

Run from repository root: python engine-comparison/tools/import_godot_art.py
Requires Pillow for WebP -> PNG (lossless pixel conversion). Never alters sources.
Outputs deterministic SHA-256 manifest; no manual path allowlist or asset redesign.
"""
from __future__ import annotations
import hashlib
import json
from pathlib import Path
import shutil

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "godot-v38" / "assets"
UNITY = ROOT / "engine-comparison" / "unity" / "Assets" / "Resources" / "MoonfallArt"
UNREAL = ROOT / "engine-comparison" / "unreal" / "MoonfallPrototype" / "SourceArt" / "MoonfallArt"
MANIFEST = ROOT / "engine-comparison" / "ART_IMPORT_MANIFEST.json"
EXTS = {".png", ".webp", ".jpg", ".jpeg", ".bmp", ".tga", ".tif", ".tiff",
        ".gif", ".hdr", ".exr", ".dds", ".svg", ".glb", ".gltf", ".fbx", ".obj"}
CONVERT = {".webp", ".gif", ".tif", ".tiff"}


def digest(path: Path) -> str:
    block_hash = hashlib.sha256()
    with path.open("rb") as file:
        for part in iter(lambda: file.read(1024 * 1024), b""):
            block_hash.update(part)
    return block_hash.hexdigest()


def import_all() -> dict:
    if not SOURCE.is_dir():
        raise SystemExit("Missing Godot artwork source directory")
    files = sorted(p for p in SOURCE.rglob("*") if p.is_file() and
                   p.suffix.lower() in EXTS and not p.is_symlink())
    if not files:
        raise SystemExit("No source artwork found; refusing to write empty manifest")
    try:
        from PIL import Image
    except ImportError as exc:
        if any(p.suffix.lower() in CONVERT for p in files):
            raise SystemExit("Pillow required for lossless image conversion") from exc
        Image = None

    artifacts = []
    seen_destinations = set()
    for src in files:
        rel = src.relative_to(SOURCE)
        if src.suffix.lower() in CONVERT:
            dest_rel = rel.with_suffix(".png")
        else:
            dest_rel = rel
        dest_key = dest_rel.as_posix().casefold()
        if dest_key in seen_destinations:
            raise SystemExit("Case-insensitive output collision: " + str(dest_rel))
        seen_destinations.add(dest_key)
        outputs = []
        for engine, root in (("unity", UNITY), ("unreal_source_art", UNREAL)):
            destination = root / dest_rel
            destination.parent.mkdir(parents=True, exist_ok=True)
            if src.suffix.lower() in CONVERT:
                assert Image is not None
                with Image.open(src) as opened:
                    # Copy decoded pixels without visual redesign or resampling.
                    opened.convert("RGBA").save(destination, format="PNG", optimize=True)
            else:
                shutil.copyfile(src, destination)
                if digest(src) != digest(destination):
                    raise RuntimeError("Source art bytes changed: " + str(src))
            outputs.append({
                "engine": engine,
                "path": destination.relative_to(ROOT).as_posix(),
                "sha256": digest(destination),
            })
        artifacts.append({
            "source": src.relative_to(ROOT).as_posix(),
            "source_sha256": digest(src),
            "format": src.suffix.lower(),
            "outputs": outputs,
        })
    payload = {
        "schema_version": 1,
        "source_root": SOURCE.relative_to(ROOT).as_posix(),
        "source_count": len(artifacts),
        "imported_files": len(artifacts) * 2,
        "notes": "Godot artwork is untouched. Unreal SourceArt must be imported in the editor; model assets may require manual setup.",
        "assets": artifacts,
    }
    MANIFEST.write_text(json.dumps(payload, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    return payload


if __name__ == "__main__":
    data = import_all()
    print(f"Imported {data['source_count']} Godot artworks into {data['imported_files']} Unity/Unreal source-art files.")

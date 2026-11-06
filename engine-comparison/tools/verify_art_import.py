#!/usr/bin/env python3
"""Verify imported art is complete, immutable, and SHA-matched to Godot sources."""
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "godot-v38" / "assets"
MANIFEST = ROOT / "engine-comparison" / "ART_IMPORT_MANIFEST.json"
EXTS = {".png", ".webp", ".jpg", ".jpeg", ".bmp", ".tga", ".tif", ".tiff",
        ".gif", ".hdr", ".exr", ".dds", ".svg", ".glb", ".gltf", ".fbx", ".obj"}

def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()

def verify() -> int:
    data = json.loads(MANIFEST.read_text(encoding="utf-8"))
    expected = sorted(p.relative_to(ROOT).as_posix() for p in SOURCE.rglob("*")
                      if p.is_file() and p.suffix.lower() in EXTS and not p.is_symlink())
    actual = sorted(entry["source"] for entry in data["assets"])
    if expected != actual:
        raise RuntimeError("Manifest does not cover all original Godot artwork")
    if data["source_count"] != len(expected) or data["imported_files"] != len(expected) * 2:
        raise RuntimeError("Incorrect import count")
    for entry in data["assets"]:
        source = ROOT / entry["source"]
        if not source.is_file() or sha256(source) != entry["source_sha256"]:
            raise RuntimeError("Original source SHA mismatch: " + entry["source"])
        if {o["engine"] for o in entry["outputs"]} != {"unity", "unreal_source_art"}:
            raise RuntimeError("Engine destination missing: " + entry["source"])
        for output in entry["outputs"]:
            target = ROOT / output["path"]
            if not target.is_file() or sha256(target) != output["sha256"]:
                raise RuntimeError("Imported artwork mismatch: " + output["path"])
    heroes = [e["source"] for e in data["assets"] if e["source"].endswith(("Z_Original.webp", "Nyra_Original.png"))]
    if len(heroes) != 2:
        raise RuntimeError("Original Z and Nyra hero art not included")
    print(f"Verified {len(expected)} original source artworks, {len(expected) * 2} import files; Z/Nyra unchanged")
    return len(expected)

if __name__ == "__main__":
    verify()

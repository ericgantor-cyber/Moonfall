#!/usr/bin/env python3
"""Verify original generated environment JPGs installed in both engine projects.

CI allows all four files to be missing until the art ZIP is committed, but never
accepts a partial import or incorrect bytes. The --require-installed option
requires four checked-in images, for use before engine release packaging.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MANIFEST = ROOT / "engine-comparison" / "GENERATED_ART_SHA256.json"
FOLDERS = (
    ROOT / "engine-comparison/unity/Assets/Resources/MoonfallGeneratedArt",
    ROOT / "engine-comparison/unreal/MoonfallPrototype/SourceArt/MoonfallGeneratedArt",
)


def verify(require_installed: bool = False) -> int:
    data = json.loads(MANIFEST.read_text(encoding="utf-8"))
    entries = data["generated_art"]
    if len(entries) != 2 or {x["filename"] for x in entries} != {
        "moonfall_ai_moonlit_gate.jpg", "moonfall_ai_waterfall_ruins.jpg"
    }:
        raise RuntimeError("Generated art manifest must describe exactly two images")
    expected = [(folder / entry["filename"], entry) for folder in FOLDERS for entry in entries]
    present = [path.is_file() for path, _ in expected]
    if not any(present):
        if require_installed:
            raise RuntimeError("AI environment JPGs not yet installed in both engines")
        print("AI art is not yet committed. Manifest validated; upload pending.")
        return 0
    if not all(present):
        raise RuntimeError("Partial AI art import: both images required in both engines")
    for path, entry in expected:
        raw = path.read_bytes()
        if len(raw) != entry["bytes"] or raw[:3] != b"\xff\xd8\xff":
            raise RuntimeError("Invalid image bytes: " + str(path))
        if hashlib.sha256(raw).hexdigest() != entry["sha256"]:
            raise RuntimeError("Generated art checksum mismatch: " + str(path))
    print("All four generated environment JPGs verified.")
    return len(expected)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--require-installed", action="store_true")
    args = parser.parse_args()
    verify(args.require_installed)

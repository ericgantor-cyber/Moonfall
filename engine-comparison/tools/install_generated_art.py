#!/usr/bin/env python3
"""Safely install Moonfall's generated scenery pack into the experimental engines.

Usage from repo root:
  python engine-comparison/tools/install_generated_art.py Moonfall_AI_Art_Ready_To_Import.zip

Checks both copies of both JPGs against the manifest before writing any files.
Does not modify the original Godot project or Z and Nyra's source art.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import zipfile

NAMES = ("moonfall_ai_moonlit_gate.jpg", "moonfall_ai_waterfall_ruins.jpg")
DESTINATIONS = (
    "engine-comparison/unity/Assets/Resources/MoonfallGeneratedArt",
    "engine-comparison/unreal/MoonfallPrototype/SourceArt/MoonfallGeneratedArt",
)

def install(zip_path: Path, repo_root: Path) -> None:
    if not (repo_root / "engine-comparison").is_dir():
        raise ValueError("Expected Moonfall repository root with engine-comparison directory")
    staged: list[tuple[Path, bytes]] = []
    with zipfile.ZipFile(zip_path) as archive:
        record = json.loads(archive.read("AI_ART_SHA256.json").decode("utf-8"))
        entries = record.get("generated_art", [])
        by_name = {item["filename"]: item for item in entries}
        if len(entries) != len(NAMES) or set(by_name) != set(NAMES):
            raise ValueError("Generated artwork manifest is missing or has extra images")
        for name in NAMES:
            sha = by_name[name]["sha256"].lower()
            expected_size = by_name[name]["bytes"]
            if len(sha) != 64 or any(c not in "0123456789abcdef" for c in sha):
                raise ValueError("Invalid SHA256: " + name)
            for folder in DESTINATIONS:
                entry_name = f"{folder}/{name}"
                data = archive.read(entry_name)
                if len(data) != expected_size:
                    raise ValueError("Unexpected size: " + entry_name)
                if hashlib.sha256(data).hexdigest() != sha:
                    raise ValueError("SHA256 mismatch: " + entry_name)
                if data[:3] != b"\xff\xd8\xff":
                    raise ValueError("Not a JPEG: " + entry_name)
                staged.append((repo_root / folder / name, data))
    # Only write once every image has been checked successfully.
    for target, data in staged:
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(data)
        print(f"Installed {target.relative_to(repo_root)}")
    print("All 4 AI environment image copies verified. Unreal Editor import is still required.")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("zip_path", type=Path)
    parser.add_argument("--repo-root", type=Path,
                        default=Path(__file__).resolve().parents[2])
    args = parser.parse_args()
    install(args.zip_path.resolve(), args.repo_root.resolve())

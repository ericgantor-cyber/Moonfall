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
import subprocess
from pathlib import Path
import zipfile

NAMES = ("moonfall_ai_moonlit_gate.jpg", "moonfall_ai_waterfall_ruins.jpg")
DESTINATIONS = (
    "engine-comparison/unity/Assets/Resources/MoonfallGeneratedArt",
    "engine-comparison/unreal/MoonfallPrototype/SourceArt/MoonfallGeneratedArt",
)

def install(zip_path: Path, repo_root: Path) -> list[Path]:
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
    return [target for target, _ in staged]

def commit_and_push(repo_root: Path, targets: list[Path]) -> None:
    """Push only the four new artwork files to the existing isolated PR branch.

    This action is never automatic: it requires the --commit-and-push flag.
    Uses the user's configured Git credentials and aborts on branch mismatch.
    """
    branch = "prototype/unreal-unity-comparison-20261104"

    def git(*args: str) -> str:
        return subprocess.check_output(
            ["git", "-C", str(repo_root), *args], text=True).strip()

    current_branch = git("branch", "--show-current")
    if current_branch != branch:
        raise RuntimeError(
            f"Expected experimental branch {branch!r}, found {current_branch!r}")
    # No writes to the original Godot tree or any other tracked files.
    relative = [str(p.relative_to(repo_root)).replace("\\", "/") for p in targets]
    preexisting = git("status", "--porcelain", "--", *relative)
    if preexisting:
        print("Only the four target art files will be staged and committed.")
    subprocess.run(["git", "-C", str(repo_root), "add", "--", *relative],
                   check=True)
    # --only prevents previously staged changes elsewhere from entering the commit.
    subprocess.run([
        "git", "-C", str(repo_root), "commit", "--only",
        "-m", "Add two new AI-generated Moonfall environment art assets",
        "--", *relative
    ], check=True)
    subprocess.run([
        "git", "-C", str(repo_root), "push", "origin", f"HEAD:{branch}"
    ], check=True)
    print("Pushed all 4 generated art files to the experimental Moonfall PR branch.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("zip_path", type=Path)
    parser.add_argument("--repo-root", type=Path,
                        default=Path(__file__).resolve().parents[2])
    parser.add_argument("--commit-and-push", action="store_true",
                        help="Explicitly commit the 4 JPGs and push to the existing experimental branch")
    args = parser.parse_args()
    root = args.repo_root.resolve()
    # Validate destination branch before writing when a GitHub push is requested.
    if args.commit_and_push:
        branch = subprocess.check_output(
            ["git", "-C", str(root), "branch", "--show-current"],
            text=True).strip()
        if branch != "prototype/unreal-unity-comparison-20261104":
            parser.error("Checkout the experimental Moonfall PR branch before publishing")
    installed = install(args.zip_path.resolve(), root)
    if args.commit_and_push:
        commit_and_push(root, installed)

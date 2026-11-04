#!/usr/bin/env python3
"""Moonfall offline bulk art intake: drop files, auto-stage, review, then approve.

Only exact asset IDs from the known 350-slot catalog are accepted. No API call.
Never edits original Z/Nyra art. Nothing automatically approves or overwrites art.
"""
import argparse
from collections import Counter
import json
from pathlib import Path
import sys

import ai_art_3d as model
import ai_art_autorig as autorig
import ai_art_review as review
import ai_art_studio as png


# File counts are no longer capped. Each file is still checked by the actual
# PNG/GLB validator, and staging processes one file at a time.
CATEGORIES = frozenset(("guardians", "animals", "regions", "chambers",
                        "glyphs", "spells", "gear", "scenery", "rigged"))


def candidate_index() -> dict[tuple[str, str], dict]:
    return {(item["category"], item["asset_id"]): item for item in review.all_items()}


def list_inbox(inbox: Path) -> list[tuple[Path, dict, str]]:
    """Return exact known art assets, refusing links and unknown categories."""
    if not inbox.exists():
        raise FileNotFoundError(f"Inbox folder missing: {inbox}")
    if not inbox.is_dir() or inbox.is_symlink():
        raise ValueError("Inbox must be a real directory, not a symlink.")
    index = candidate_index()
    result = []
    for folder in sorted(inbox.iterdir()):
        if folder.is_symlink():
            raise ValueError(f"Symlinked inbox category is forbidden: {folder.name}")
        if not folder.is_dir():
            # A readme is fine; accepted artwork always lives in a category.
            continue
        if folder.name not in CATEGORIES:
            raise ValueError(f"Unknown art category folder: {folder.name}")
        for path in sorted(folder.iterdir()):
            if path.is_symlink():
                raise ValueError(f"Symlinked input file forbidden: {path.name}")
            if not path.is_file():
                continue
            # No arbitrary 900-file or 512-MiB batch ceilings; the import
            # itself is sequential and per-file format limits are retained.
            ext = path.suffix.lower()
            if ext not in {".png", ".glb"}:
                raise ValueError(f"Unsupported file in art inbox: {path.name}")
            if folder.name == "rigged":
                if ext != ".glb":
                    raise ValueError("A rigged model must be an animated GLB.")
                item = index.get(("guardians", path.stem)) or index.get(("animals", path.stem))
                if item is None:
                    raise ValueError(f"Unknown animated creature slot: {path.name}")
                result.append((path, item, ".rigged"))
                continue
            key = (folder.name, path.stem)
            if key not in index:
                raise ValueError(f"Unknown or malformed game-art ID: {folder.name}/{path.name}")
            item = index[key]
            if ext == ".glb" and folder.name not in model.ALLOWED_MODEL_CATEGORIES:
                raise ValueError("Only creatures, physical scenery, gear and chambers support GLB.")
            result.append((path, item, ext))
    return result


def plan(inbox: Path) -> list[dict]:
    results = []
    for path, item, ext in list_inbox(inbox):
        staged = (autorig.paths(item)[0] if ext == ".rigged" else
                  model.paths(item)[0] if ext == ".glb" else png.dest_paths(item)[0])
        approved = (autorig.paths(item)[1] if ext == ".rigged" else
                    model.paths(item)[1] if ext == ".glb" else png.dest_paths(item)[1])
        state = "approved" if approved.exists() else "staged" if staged.exists() else "ready"
        results.append({"file": str(path), "asset_id": item["asset_id"],
                        "category": item["category"], "format": ext[1:], "status": state})
    return results


def stage_all(inbox: Path) -> list[dict]:
    result = []
    for path, item, ext in list_inbox(inbox):
        staged = (autorig.paths(item)[0] if ext == ".rigged" else
                  model.paths(item)[0] if ext == ".glb" else png.dest_paths(item)[0])
        approved = (autorig.paths(item)[1] if ext == ".rigged" else
                    model.paths(item)[1] if ext == ".glb" else png.dest_paths(item)[1])
        if approved.exists() or staged.exists():
            result.append({"asset_id": item["asset_id"], "format": ext[1:], "status": "skipped_existing"})
            continue
        # This validates structural PNG / self-contained GLB and produces a
        # review manifest, but NEVER approves or converts the file.
        try:
            if ext == ".rigged":
                target = autorig.import_local(item, path)
            elif ext == ".glb":
                target = model.import_local(item, path)
            else:
                target = png.import_local_image(item, png.prompt_for(item), path)
        except (ValueError, OSError, RuntimeError) as error:
            result.append({"asset_id": item["asset_id"], "format": ext[1:],
                           "status": "rejected", "reason": str(error)})
            continue
        result.append({"asset_id": item["asset_id"], "format": ext[1:],
                       "status": "staged_for_review", "file": str(target)})
    return result


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--inbox", type=Path, default=png.PROJECT / "ai_art_inbox",
                        help="Directory with category subfolders named guardians, animals, etc.")
    parser.add_argument("--stage", action="store_true",
                        help="Validate and stage all recognized files (never approves or sends to an API)")
    parser.add_argument("--json", action="store_true", help="Print a machine-readable report")
    args = parser.parse_args(argv)
    try:
        result = stage_all(args.inbox) if args.stage else plan(args.inbox)
        if args.json:
            print(json.dumps(result, indent=2))
        else:
            counts = Counter(x["status"] for x in result)
            print(f"Moonfall art inbox: {len(result)} files | " +
                  ", ".join(f"{count} {kind}" for kind, count in sorted(counts.items())))
            if args.stage:
                print("Validated art is staged. Inspect the images/models in the review folder before approving.")
        return 1 if any(x["status"] == "rejected" for x in result) else 0
    except (ValueError, FileNotFoundError, OSError) as error:
        print("Moonfall art inbox:", error, file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())

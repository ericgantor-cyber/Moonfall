#!/usr/bin/env python3
"""Validate Moonfall's five-category, cross-engine feature roadmap."""
from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "config" / "moonfall_game_categories.json"
EXPECTED = ("characters", "combat", "world", "progression", "interface")
STAGES = ("foundation", "integration", "polish")


def validate(data: dict, root: Path = ROOT) -> None:
    if data.get("schema_version") != 2:
        raise ValueError("Unsupported categories schema_version")
    milestones = data.get("milestones")
    if not isinstance(milestones, list) or tuple(m.get("id") for m in milestones) != STAGES:
        raise ValueError("Three stable milestones are required")
    if any(not isinstance(m.get("title"), str) or not m["title"].strip() for m in milestones):
        raise ValueError("Milestones must have labels")
    categories = data.get("categories")
    if not isinstance(categories, list) or len(categories) != 5:
        raise ValueError("Moonfall must have exactly five game categories")
    ids = tuple(cat.get("id") for cat in categories)
    if ids != EXPECTED:
        raise ValueError(f"Expected five stable category IDs in order: {EXPECTED}")

    owners: set[str] = set()
    tracks: dict[str, dict] = {}
    category_for: dict[str, str] = {}
    for cat in categories:
        for field in ("title", "purpose"):
            if not isinstance(cat.get(field), str) or not cat[field].strip():
                raise ValueError(f"Missing {field} for {cat['id']}")
        examples = cat.get("features")
        if not isinstance(examples, list) or not examples or not all(
            isinstance(v, str) and v.strip() for v in examples
        ):
            raise ValueError(f"Invalid feature examples for {cat['id']}")
        owner_paths = cat.get("owners")
        if not isinstance(owner_paths, list) or not owner_paths:
            raise ValueError(f"Category must have existing source ownership: {cat['id']}")
        for owner in owner_paths:
            if not isinstance(owner, str) or not owner:
                raise ValueError("Owner path must be a nonempty string")
            relative = Path(owner)
            if relative.is_absolute() or ".." in relative.parts or "\\" in owner:
                raise ValueError(f"Unsafe owner path: {owner}")
            if owner in owners:
                raise ValueError(f"Duplicate file owner: {owner}")
            owners.add(owner)
            if not (root / relative).is_file():
                raise ValueError(f"Missing owner source file: {owner}")
        roadmap = cat.get("roadmap")
        if not isinstance(roadmap, list) or len(roadmap) != 5:
            raise ValueError(f"Exactly five roadmap tracks required for {cat['id']}")
        for track in roadmap:
            key = track.get("id")
            if not isinstance(key, str) or not key or not key.replace("_", "").isalnum():
                raise ValueError(f"Invalid track ID: {key}")
            if key in tracks:
                raise ValueError(f"Duplicate roadmap track: {key}")
            if track.get("milestone") not in STAGES:
                raise ValueError(f"Invalid milestone: {key}")
            if track.get("verification") != "not_verified":
                raise ValueError(f"Do not claim unverified features as complete: {key}")
            if not isinstance(track.get("title"), str) or not track["title"].strip():
                raise ValueError(f"Missing roadmap title: {key}")
            if not isinstance(track.get("depends_on"), list) or len(set(track["depends_on"])) != len(track["depends_on"]):
                raise ValueError(f"Malformed dependencies: {key}")
            tracks[key] = track
            category_for[key] = cat["id"]

    for key, track in tracks.items():
        for dep in track["depends_on"]:
            if dep not in tracks or dep == key:
                raise ValueError(f"Unknown or self dependency: {key} -> {dep}")
            if STAGES.index(tracks[dep]["milestone"]) > STAGES.index(track["milestone"]):
                raise ValueError(f"Later milestone dependency: {key} -> {dep}")

    visiting: set[str] = set()
    visited: set[str] = set()

    def visit(key: str) -> None:
        if key in visiting:
            raise ValueError(f"Cyclic roadmap dependency: {key}")
        if key in visited:
            return
        visiting.add(key)
        for dep in tracks[key]["depends_on"]:
            visit(dep)
        visiting.remove(key)
        visited.add(key)

    for key in tracks:
        visit(key)


if __name__ == "__main__":
    validate(json.loads(MANIFEST.read_text(encoding="utf-8")))
    print("Moonfall: 5 categories, 25 roadmap tracks, valid owners and acyclic dependencies")

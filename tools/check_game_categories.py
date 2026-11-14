#!/usr/bin/env python3
"""Validate the five Moonfall category boundaries without moving game code."""
from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "config" / "moonfall_game_categories.json"
EXPECTED = ("characters", "combat", "world", "progression", "interface")


def validate(data: dict, root: Path = ROOT) -> None:
    if data.get("schema_version") != 1:
        raise ValueError("Unsupported categories schema_version")
    categories = data.get("categories")
    if not isinstance(categories, list) or len(categories) != 5:
        raise ValueError("Moonfall must have exactly five game categories")
    ids = tuple(category.get("id") for category in categories)
    if ids != EXPECTED:
        raise ValueError(f"Expected five stable category IDs in order: {EXPECTED}")
    owners: set[str] = set()
    for category in categories:
        if not isinstance(category.get("title"), str) or not category["title"].strip():
            raise ValueError(f"Missing title for {category['id']}")
        if not isinstance(category.get("purpose"), str) or not category["purpose"].strip():
            raise ValueError(f"Missing purpose for {category['id']}")
        features = category.get("features")
        if not isinstance(features, list) or not features or not all(
            isinstance(value, str) and value.strip() for value in features
        ):
            raise ValueError(f"Missing feature examples for {category['id']}")
        examples = category.get("owners")
        if not isinstance(examples, list):
            raise ValueError(f"Invalid owners for {category['id']}")
        for owner in examples:
            if not isinstance(owner, str) or not owner:
                raise ValueError("Owner paths must be nonempty strings")
            relative = Path(owner)
            if relative.is_absolute() or ".." in relative.parts:
                raise ValueError(f"Unsafe owner path: {owner}")
            if owner in owners:
                raise ValueError(f"Source file has more than one category owner: {owner}")
            owners.add(owner)
            if not (root / relative).is_file():
                raise ValueError(f"Listed owner file does not exist: {owner}")
    if not owners:
        raise ValueError("Expected some concrete source-file ownership")


if __name__ == "__main__":
    validate(json.loads(MANIFEST.read_text(encoding="utf-8")))
    print("Moonfall: 5 categories, valid ownership and existing source paths")

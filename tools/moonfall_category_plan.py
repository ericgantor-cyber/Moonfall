#!/usr/bin/env python3
"""Build a deterministic, dependency-safe work plan from Moonfall's five domains.

Examples:
    python tools/moonfall_category_plan.py
    python tools/moonfall_category_plan.py --format json
    python tools/moonfall_category_plan.py --category combat
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path

from check_game_categories import EXPECTED, MANIFEST, ROOT, STAGES, validate


def ordered_tracks(data: dict, category: str | None = None) -> list[dict]:
    """Return dependency closure in stable milestone/catalog order.

    A category filter includes prerequisites from other categories, clearly
    marked by their owning category. It cannot silently omit dependencies.
    """
    categories = data["categories"]
    by_id: dict[str, dict] = {}
    for c in categories:
        for track in c["roadmap"]:
            by_id[track["id"]] = {**track, "category": c["id"]}
    if category is not None and category not in EXPECTED:
        raise ValueError(f"Unknown category: {category}")

    requested = {key for key, value in by_id.items()
                 if category is None or value["category"] == category}
    required: set[str] = set()

    def add_prereqs(track_id: str) -> None:
        if track_id in required:
            return
        required.add(track_id)
        for dep in by_id[track_id]["depends_on"]:
            add_prereqs(dep)

    for key in requested:
        add_prereqs(key)

    catalog_index = {key: index for index, key in enumerate(by_id)}
    stage_index = {stage: index for index, stage in enumerate(STAGES)}
    remaining = set(required)
    result: list[dict] = []
    finished: set[str] = set()
    while remaining:
        ready = [key for key in remaining
                 if all(dep in finished for dep in by_id[key]["depends_on"])]
        if not ready:
            raise ValueError("Cannot schedule cyclic or missing dependencies")
        ready.sort(key=lambda key: (stage_index[by_id[key]["milestone"]],
                                    catalog_index[key]))
        chosen = ready[0]
        item = {**by_id[chosen], "prerequisite_only": chosen not in requested}
        result.append(item)
        finished.add(chosen)
        remaining.remove(chosen)
    return result


def render_markdown(tracks: list[dict], category: str | None) -> str:
    title = "Moonfall: five-category development plan"
    if category:
        title += f" / {category}"
    lines = [f"# {title}", "",
             "Planning order only. **No gameplay completion is implied.**",
             "Every track below is marked `not_verified` until actual engine tests exist.", ""]
    for stage in STAGES:
        items = [track for track in tracks if track["milestone"] == stage]
        if not items:
            continue
        lines.extend([f"## {stage.title()}", "",
                      "| Track | Category | Dependencies | Scope |",
                      "| --- | --- | --- | --- |"])
        for track in items:
            deps = ", ".join(f"`{dep}`" for dep in track["depends_on"]) or "None"
            scope = "Prerequisite" if track["prerequisite_only"] else "Requested category"
            if category is None:
                scope = "All categories"
            lines.append(f"| `{track['id']}`: {track['title']} | {track['category']} | {deps} | {scope} |")
        lines.append("")
    lines.append(f"**Tracks in this plan:** {len(tracks)}")
    lines.append("")
    return "\n".join(lines)


def build_plan(data: dict, root: Path = ROOT, category: str | None = None) -> dict:
    validate(data, root)
    tracks = ordered_tracks(data, category)
    return {
        "schema_version": 1,
        "category": category or "all",
        "total": len(tracks),
        "tracks": tracks,
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--category", choices=EXPECTED,
                        help="Include one category and all external prerequisites")
    parser.add_argument("--format", choices=("markdown", "json"), default="markdown")
    parser.add_argument("--output", type=Path,
                        help="Optional report file (otherwise stdout)")
    args = parser.parse_args()
    data = json.loads(MANIFEST.read_text(encoding="utf-8"))
    plan = build_plan(data, ROOT, args.category)
    output = (json.dumps(plan, indent=2) + "\n" if args.format == "json"
              else render_markdown(plan["tracks"], args.category))
    if args.output:
        args.output.write_text(output, encoding="utf-8")
    else:
        print(output, end="")


if __name__ == "__main__":
    main()

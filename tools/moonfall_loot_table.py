#!/usr/bin/env python3
"""Validate Moonfall UE5 loot CSVs and preview exact effective rarity weights.

Uses only Python's standard library. Source/editor validation is not a native
UE5 build or evidence that a given Unreal DataTable asset imported correctly.
"""
from __future__ import annotations

import argparse
import csv
from pathlib import Path

DEFAULT_CSV = Path(__file__).resolve().parents[1] / "MoonfallUnreal" / "Data" / "Moonfall_DefaultLootTable.csv"
FIELDS = ("Name", "Source", "ZoneIndex", "MinimumLevel", "Rarity",
          "Weight", "SlotMask", "ForcedTrait")
SOURCES = {"Regular": 0, "Elite": 1, "Boss": 2, "Treasure": 3}
RARITIES = ("Common", "Uncommon", "Rare", "Epic", "Emerald", "Legendary")
RARITY_FLOORS = {"Regular": 0, "Elite": 2, "Boss": 3, "Treasure": 2}
MAX_TOTAL_WEIGHT = 1_000_000


def validate_rows(rows: list[dict]) -> list[dict]:
    """Reject any table that the C++ loader would reject, in sorted row order."""
    if not 1 <= len(rows) <= 2048:
        raise ValueError("Loot tables require 1 to 2048 rows.")
    names: set[str] = set()
    coverage: set[str] = set()
    validated: list[dict] = []
    for row in rows:
        missing = [key for key in FIELDS if key not in row]
        if missing:
            raise ValueError(f"Missing columns: {', '.join(missing)}")
        name = str(row["Name"]).strip()
        if not name or name in names:
            raise ValueError(f"Missing or duplicate loot row name: {name!r}")
        names.add(name)
        source = str(row["Source"]).strip()
        if source not in SOURCES:
            raise ValueError(f"{name}: invalid source {source!r}")
        try:
            values = {key: int(str(row[key]).strip()) for key in FIELDS[2:]}
        except (ValueError, TypeError) as exc:
            raise ValueError(f"{name}: all tuning fields must be integers") from exc
        zone = values["ZoneIndex"]
        level = values["MinimumLevel"]
        rarity = values["Rarity"]
        weight = values["Weight"]
        slots = values["SlotMask"]
        trait = values["ForcedTrait"]
        if not (-1 <= zone <= 5 and 1 <= level <= 100
                and RARITY_FLOORS[source] <= rarity <= 5
                and 0 <= weight <= 10000 and 1 <= slots <= 7
                and 0 <= trait <= 3 and (rarity != 0 or trait == 0)):
            raise ValueError(f"{name}: invalid rarity/zone/level/weight/slot/trait")
        if zone == -1 and level == 1 and weight > 0:
            coverage.add(source)
        validated.append({"Name": name, "Source": source, **values})
    if coverage != set(SOURCES):
        raise ValueError("Each source needs an enabled, generic minimum-level-1 row; "
                         f"missing {', '.join(sorted(set(SOURCES) - coverage))}")
    return sorted(validated, key=lambda row: row["Name"])


def load_rows(path: Path) -> list[dict]:
    with path.open("r", encoding="utf-8-sig", newline="") as stream:
        reader = csv.DictReader(stream)
        if reader.fieldnames is None or set(reader.fieldnames) != set(FIELDS):
            raise ValueError("CSV header must match: " + ",".join(FIELDS))
        return validate_rows(list(reader))


def rarity_weights(rows: list[dict], source: str, zone: int, level: int) -> tuple[int, list[int]]:
    """Match UE's sorted order, level/region filters and one-million weight cap."""
    if source not in SOURCES:
        raise ValueError("Source must be Regular, Elite, Boss or Treasure")
    zone = zone if 0 <= zone < 6 else -1
    level = min(100, max(1, level))
    total = 0
    weights = [0] * len(RARITIES)
    for row in sorted(rows, key=lambda item: item["Name"]):
        weight = row["Weight"]
        if (row["Source"] != source or row["ZoneIndex"] not in (-1, zone)
                or row["MinimumLevel"] > level or weight <= 0
                or total > MAX_TOTAL_WEIGHT - weight):
            continue
        total += weight
        weights[row["Rarity"]] += weight
    return total, weights


def main() -> int:
    parser = argparse.ArgumentParser(description="Validate and preview Moonfall weighted loot tables")
    parser.add_argument("--file", type=Path, default=DEFAULT_CSV,
                        help="CSV to validate (defaults to the example 41-row table)")
    parser.add_argument("--source", choices=list(SOURCES), default="Boss")
    parser.add_argument("--zone", type=int, default=0,
                        help="0-5 for dungeon region, -1 for generic overworld")
    parser.add_argument("--level", type=int, default=60)
    args = parser.parse_args()
    try:
        rows = load_rows(args.file)
    except (OSError, ValueError) as exc:
        parser.exit(1, f"Invalid Moonfall loot table: {exc}\n")
    total, chances = rarity_weights(rows, args.source, args.zone, args.level)
    print(f"Valid: {len(rows)} editable loot rows, all four generic sources covered.")
    print(f"{args.source} | zone {args.zone} | level {args.level} | total weight {total}")
    for name, weight in zip(RARITIES, chances):
        percent = 100.0 * weight / total if total else 0.0
        print(f"  {name:10} {percent:6.2f}%  (weight {weight})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

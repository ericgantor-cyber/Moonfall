#!/usr/bin/env python3
"""Pure WoW Forever-inspired secondary stat calculations for Moonfall.

Provisional Moonfall numbers, NOT copied WoW formulas or wired to gameplay.
"""
from __future__ import annotations

import json
import math
from pathlib import Path

CONFIG = Path(__file__).resolve().parents[1] / "config/moonfall_combat_ratings.json"
# Explicit design contract: retired Hit/Dodge/Block/Parry ratings cannot
# reappear by editing the JSON file or passing a custom config.
RATINGS = frozenset(("critical_strike", "haste"))
FLAT_STATS = frozenset(("armor", "attack_power", "spell_power", "health_regen",
                        "mana_regen", "fire_resistance", "frost_resistance",
                        "shock_resistance"))


def load() -> dict:
    data = json.loads(CONFIG.read_text(encoding="utf-8"))
    validate(data)
    return data


def validate(data: dict) -> None:
    if data.get("schema_version") != 1:
        raise ValueError("Unsupported combat rating schema")
    ratings = data.get("ratings")
    flat = data.get("flat_stats")
    if not isinstance(ratings, dict) or set(ratings) != RATINGS:
        raise ValueError("Combat ratings must be exactly Critical Strike and Haste")
    if not isinstance(flat, dict) or set(flat) != FLAT_STATS:
        raise ValueError("Invalid or missing supported flat combat stats")
    if set(ratings) & set(flat):
        raise ValueError("Duplicate combat stat ID")
    for key, values in ratings.items():
        if not isinstance(key, str) or not key or not isinstance(values, dict):
            raise ValueError("Invalid rating")
        if not isinstance(values.get("label"), str) or not values["label"]:
            raise ValueError("Missing rating label")
        for prop in ("rating_per_percent", "base_percent", "maximum_percent"):
            num = values.get(prop)
            if type(num) not in (int, float) or not math.isfinite(num):
                raise ValueError("Invalid conversion rule")
        if (values["rating_per_percent"] <= 0 or values["base_percent"] < 0
                or not values["base_percent"] <= values["maximum_percent"] <= 100):
            raise ValueError("Invalid rating cap")
    for key, values in flat.items():
        if not isinstance(values.get("label"), str) or not values["label"]:
            raise ValueError("Missing flat stat label")
        lo, hi = values.get("min"), values.get("max")
        if any(type(x) not in (int, float) or not math.isfinite(x) for x in (lo, hi)):
            raise ValueError("Invalid flat stat bound")
        if lo < 0 or hi < lo:
            raise ValueError("Invalid flat stat range")
    limits = data.get("limits", {})
    for key in ("maximum_rating", "maximum_total_rating"):
        value = limits.get(key)
        if type(value) is not int or value <= 0:
            raise ValueError("Invalid rating limit")
    if limits["maximum_total_rating"] < limits["maximum_rating"]:
        raise ValueError("Total rating limit is smaller than one stat's limit")


def calculate(ratings: dict[str, int] | None = None,
              flat_bonuses: dict[str, float] | None = None,
              config: dict | None = None) -> dict:
    data = config if config is not None else load()
    validate(data)
    rating_in = {} if ratings is None else ratings
    bonus_in = {} if flat_bonuses is None else flat_bonuses
    if not isinstance(rating_in, dict) or set(rating_in) - set(data["ratings"]):
        raise ValueError("Unknown combat rating")
    if not isinstance(bonus_in, dict) or set(bonus_in) - set(data["flat_stats"]):
        raise ValueError("Unknown flat combat stat")
    for key, value in rating_in.items():
        if type(value) is not int or not 0 <= value <= data["limits"]["maximum_rating"]:
            raise ValueError(f"Invalid rating amount: {key}")
    if sum(rating_in.values()) > data["limits"]["maximum_total_rating"]:
        raise ValueError("Total combat rating over limit")
    for key, value in bonus_in.items():
        if type(value) not in (int, float) or not math.isfinite(value):
            raise ValueError(f"Invalid flat combat bonus: {key}")
    percentages = {}
    for key, rule in data["ratings"].items():
        percentages[key] = min(rule["maximum_percent"],
                               rule["base_percent"] +
                               rating_in.get(key, 0) / rule["rating_per_percent"])
    flat = {key: min(rule["max"], max(rule["min"], bonus_in.get(key, 0.0)))
            for key, rule in data["flat_stats"].items()}
    return {"percentages": percentages, "flat": flat,
            "ratings": {key: rating_in.get(key, 0) for key in data["ratings"]},
            "ruleset": "moonfall_classless_ratings"}


if __name__ == "__main__":
    print(json.dumps(calculate({"critical_strike": 120, "haste": 75},
                               {"armor": 300, "attack_power": 40}), indent=2))

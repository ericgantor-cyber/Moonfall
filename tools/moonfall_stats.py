#!/usr/bin/env python3
"""Deterministic reference calculations for Moonfall's classless stats.

This reference model is NOT yet hooked into Godot or Unreal gameplay.
"""
from __future__ import annotations
import json
import math
from pathlib import Path

from moonfall_combat_ratings import calculate as calculate_combat_ratings
from moonfall_talents import evaluate as evaluate_talents

ROOT = Path(__file__).resolve().parents[1]
CONFIG = ROOT / "config/moonfall_stats.json"
KEYS = ("might", "agility", "arcana", "vitality", "resolve", "endurance")
VITALS = ("health", "mana", "stamina")
DAMAGE = ("melee_damage", "ranged_damage", "magic_damage")


def load_config() -> dict:
    data = json.loads(CONFIG.read_text(encoding="utf-8"))
    validate_config(data)
    return data


def validate_config(data: dict) -> None:
    if not isinstance(data, dict) or data.get("schema_version") != 1:
        raise ValueError("Unsupported stat schema")
    attrs = data.get("attributes", {})
    growth = data.get("auto_growth_per_level", {})
    if not isinstance(attrs, dict) or not isinstance(growth, dict) or set(attrs) != set(KEYS) or set(growth) != set(KEYS):
        raise ValueError("Exactly six classless attributes required")
    if not isinstance(data.get("base"), dict) or set(data["base"]) != set(VITALS + DAMAGE):
        raise ValueError("Base stats must include three resources and three damage types")
    seen_targets = set()
    for key in KEYS:
        rule = attrs[key]
        if not isinstance(rule, dict):
            raise ValueError("Invalid attribute rule")
        target = rule.get("stat")
        kind = rule.get("kind")
        if target not in VITALS + DAMAGE or target in seen_targets:
            raise ValueError("Each attribute must have a distinct valid stat target")
        seen_targets.add(target)
        if kind != ("flat" if target in VITALS else "damage_multiplier"):
            raise ValueError("Attribute kind does not match its target stat")
        rate = rule.get("bonus_per_point")
        if type(rate) not in (int, float) or not math.isfinite(rate) or rate < 0:
            raise ValueError("Attribute bonuses must be finite and nonnegative")
        if type(growth[key]) is not int or growth[key] < 0:
            raise ValueError("Automatic growth must be a nonnegative integer")
    for key, val in data["base"].items():
        if type(val) not in (int, float) or not math.isfinite(val) or val <= 0:
            raise ValueError(f"Invalid base stat: {key}")
    power = data.get("power")
    if not isinstance(power, dict) or power.get("rounding") != "floor":
        raise ValueError("Only deterministic floor power scoring is supported")
    for name in ("level_weight", "gear_score_weight"):
        value = power.get(name)
        if type(value) not in (int, float) or not math.isfinite(value) or value < 0:
            raise ValueError("Power weights must be finite and nonnegative")
    limits = data.get("limits")
    if not isinstance(limits, dict):
        raise ValueError("Invalid stat range limits")
    for low, high in (("min_level", "max_level"),
                      ("min_gear_score", "max_gear_score")):
        if type(limits.get(low)) is not int or type(limits.get(high)) is not int:
            raise ValueError("Stat range limits must be integers")
        if limits[low] < 0 or limits[low] > limits[high]:
            raise ValueError("Invalid stat range")


def calculate(level: int, gear_score: int = 0,
              gear_bonuses: dict[str, float] | None = None,
              config: dict | None = None,
              combat_ratings: dict[str, int] | None = None,
              combat_bonuses: dict[str, float] | None = None,
              talent_allocations: dict[str, int] | None = None) -> dict:
    data = config if config is not None else load_config()
    validate_config(data)
    limits = data["limits"]
    if type(level) is not int or not limits["min_level"] <= level <= limits["max_level"]:
        raise ValueError("Level outside allowed range")
    if type(gear_score) is not int or not limits["min_gear_score"] <= gear_score <= limits["max_gear_score"]:
        raise ValueError("Gear score outside allowed range")
    # Preserve invalid-but-empty inputs for validation instead of silently
    # converting [] or "" into an empty dictionary.
    bonuses = {} if gear_bonuses is None else gear_bonuses
    if not isinstance(bonuses, dict) or set(bonuses) - set(VITALS + DAMAGE):
        raise ValueError("Unknown gear bonus stat")
    for key, val in bonuses.items():
        if type(val) not in (int, float) or not math.isfinite(val):
            raise ValueError(f"Invalid gear modifier: {key}")
    attrs = {key: (level - 1) * data["auto_growth_per_level"][key]
             for key in KEYS}
    stats = {key: float(val) for key, val in data["base"].items()}
    for name, rule in data["attributes"].items():
        target = rule["stat"]
        rate = rule["bonus_per_point"]
        if rule["kind"] == "flat":
            stats[target] += attrs[name] * rate
        else:
            stats[target] *= 1 + attrs[name] * rate
    for key, bonus in bonuses.items():
        stats[key] = max(0.0, stats[key] + bonus)
    # Talent choices are evaluated independently from automatic level growth.
    # Apply flat talents first, then percentages on the combined base + gear
    # value. A 0.03 talent bonus means +3%, not 3 percentage points.
    talents = evaluate_talents(level, talent_allocations)
    for key, effect in talents["bonuses"].items():
        stats[key] = max(0.0, (stats[key] + effect["flat"]) *
                         (1.0 + effect["percent"]))
    power = math.floor(level * data["power"]["level_weight"] +
                       gear_score * data["power"]["gear_score_weight"])
    return {"level": level, "attributes": attrs, "stats": stats,
            "gear_score": gear_score, "power": power,
            "combat": calculate_combat_ratings(combat_ratings, combat_bonuses),
            "talents": talents, "ruleset": "classless_no_cards"}


if __name__ == "__main__":
    sample = calculate(10, 80, {"health": 25, "melee_damage": 2})
    print(json.dumps(sample, indent=2))

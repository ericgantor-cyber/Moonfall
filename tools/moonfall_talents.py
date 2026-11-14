#!/usr/bin/env python3
"""Moonfall classless talent-tree prototype. Not integrated with either engine."""
from __future__ import annotations
import json
import math
from pathlib import Path

CONFIG = Path(__file__).resolve().parents[1] / "config" / "moonfall_talents.json"
STATS = ("health", "mana", "stamina", "melee_damage", "ranged_damage", "magic_damage")


def load() -> dict:
    data = json.loads(CONFIG.read_text(encoding="utf-8"))
    validate(data)
    return data


def validate(data: dict) -> None:
    if data.get("schema_version") != 1:
        raise ValueError("Unsupported talent schema")
    branches = data.get("branches")
    if not isinstance(branches, list) or len(branches) != 3:
        raise ValueError("Expected three talent branches")
    branch_ids = [b["id"] for b in branches]
    if len(set(branch_ids)) != 3:
        raise ValueError("Duplicate branch")
    nodes = data.get("nodes")
    if not isinstance(nodes, list) or not nodes:
        raise ValueError("Missing talent nodes")
    ids = [node.get("id") for node in nodes]
    if len(ids) != len(set(ids)):
        raise ValueError("Duplicate talent node")
    index = {node["id"]: node for node in nodes}
    max_level = data.get("maximum_level")
    points = data.get("points_per_level")
    start = data.get("starting_points")
    if type(max_level) is not int or max_level < 1 or type(points) is not int or points < 0:
        raise ValueError("Invalid level or point budget")
    if type(start) is not int or start < 0:
        raise ValueError("Invalid starting budget")
    for node in nodes:
        if node.get("branch") not in branch_ids:
            raise ValueError("Unknown talent branch")
        if type(node.get("tier")) is not int or node["tier"] < 1:
            raise ValueError("Invalid talent tier")
        if type(node.get("min_level")) is not int or not 1 <= node["min_level"] <= max_level:
            raise ValueError("Invalid level gate")
        if type(node.get("max_rank")) is not int or node["max_rank"] < 1:
            raise ValueError("Invalid rank limit")
        if type(node.get("cost_per_rank")) is not int or node["cost_per_rank"] < 1:
            raise ValueError("Invalid point cost")
        effect = node.get("effect", {})
        if effect.get("stat") not in STATS or effect.get("kind") not in ("flat", "percent"):
            raise ValueError("Invalid talent effect")
        amount = effect.get("amount_per_rank")
        if type(amount) not in (int, float) or not math.isfinite(amount) or amount < 0:
            raise ValueError("Invalid effect value")
        deps = node.get("requires")
        if not isinstance(deps, list) or len(deps) != len(set(deps)):
            raise ValueError("Invalid prerequisites")
        for dep in deps:
            if dep not in index or dep == node["id"]:
                raise ValueError("Unknown or self talent prerequisite")
            if index[dep]["tier"] >= node["tier"]:
                raise ValueError("Prerequisite must be in earlier tier")
    visiting, visited = set(), set()
    def visit(node_id: str) -> None:
        if node_id in visiting:
            raise ValueError("Cyclic talents")
        if node_id in visited:
            return
        visiting.add(node_id)
        for dep in index[node_id]["requires"]:
            visit(dep)
        visiting.remove(node_id)
        visited.add(node_id)
    for node_id in index:
        visit(node_id)


def evaluate(level: int, allocations: dict[str, int] | None = None,
             data: dict | None = None) -> dict:
    tree = data if data is not None else load()
    validate(tree)
    if type(level) is not int or not 1 <= level <= tree["maximum_level"]:
        raise ValueError("Invalid character level")
    chosen = allocations if allocations is not None else {}
    if not isinstance(chosen, dict):
        raise ValueError("Allocations must be a map")
    index = {node["id"]: node for node in tree["nodes"]}
    if set(chosen) - set(index):
        raise ValueError("Unknown talent choice")
    ranks = {}
    spent = 0
    for key, rank in chosen.items():
        node = index[key]
        if type(rank) is not int or not 0 <= rank <= node["max_rank"]:
            raise ValueError("Invalid allocated rank")
        if rank > 0 and level < node["min_level"]:
            raise ValueError("Talent level gate not reached")
        ranks[key] = rank
        spent += rank * node["cost_per_rank"]
    for key, rank in ranks.items():
        if rank and any(ranks.get(dep, 0) < 1 for dep in index[key]["requires"]):
            raise ValueError("Missing talent prerequisite")
    earned = tree["starting_points"] + (level - 1) * tree["points_per_level"]
    if spent > earned:
        raise ValueError("Not enough talent points")
    bonuses = {stat: {"flat": 0.0, "percent": 0.0} for stat in STATS}
    for key, rank in ranks.items():
        if rank:
            effect = index[key]["effect"]
            bonuses[effect["stat"]][effect["kind"]] += rank * effect["amount_per_rank"]
    return {"level": level, "earned": earned, "spent": spent,
            "available": earned - spent, "allocations": {k: v for k, v in ranks.items() if v},
            "bonuses": bonuses, "ruleset": "classless_talent_tree"}


def spend(level: int, current: dict[str, int], talent_id: str, data: dict | None = None) -> dict:
    tree = data if data is not None else load()
    # Never accept an invalid or malformed saved state simply because the
    # requested new node would otherwise satisfy the final dependencies.
    # Unlike evaluate(), spend() requires an explicit saved allocation map.
    if not isinstance(current, dict):
        raise ValueError("Allocations must be a map")
    prior = evaluate(level, current, tree)
    index = {node["id"]: node for node in tree["nodes"]}
    if talent_id not in index:
        raise ValueError("Unknown talent choice")
    chosen = dict(prior["allocations"])
    chosen[talent_id] = chosen.get(talent_id, 0) + 1
    return evaluate(level, chosen, tree)



def respec(level: int, data: dict | None = None) -> dict:
    return evaluate(level, {}, data)


if __name__ == "__main__":
    print(json.dumps(spend(5, {"iron_will": 1}, "breaker"), indent=2))

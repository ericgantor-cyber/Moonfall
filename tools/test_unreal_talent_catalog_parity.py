"""Verify the UE5 talent catalog matches Moonfall's canonical Godot JSON exactly."""
import json
import math
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
source = (ROOT / "MoonfallUnreal/Source/MoonfallUnreal/MoonfallTalentRules.h").read_text(encoding="utf-8")
canonical = json.loads((ROOT / "godot-v38/data/moonfall_talents.json").read_text(encoding="utf-8"))
pattern = re.compile(
    r'\{"([^"]+)","([^"]+)",Branch::(\w+),(\d+),(\d+),(-?\d+),(-?\d+),(\d+),'
    r'Stat::(\w+),([\d.]+),(true|false)\}'
)
entries = pattern.findall(source)
nodes = canonical["nodes"]
assert canonical["schema_version"] == 1
assert canonical["points_per_level"] == 2
assert canonical["maximum_level"] == 100
assert len(nodes) == len(entries) == 141, (len(nodes), len(entries))
ids = [node["id"] for node in nodes]
assert len(set(ids)) == 141, "Duplicate stable talent IDs"
branches = {"might": "Might", "agility": "Agility", "arcana": "Arcana"}
stats = {"health": "Health", "mana": "Mana", "stamina": "Stamina",
         "melee_damage": "MeleeDamage", "ranged_damage": "RangedDamage",
         "magic_damage": "MagicDamage"}
for index, (node, entry) in enumerate(zip(nodes, entries)):
    (node_id, title, branch, tier, level, req_a, req_b,
     max_rank, stat, amount, percent) = entry
    requirements = [ids.index(req) for req in node["requires"]]
    requirements += [-1] * (2 - len(requirements))
    assert len(requirements) == 2, (index, node_id)
    expected = (
        node["id"], node["title"], branches[node["branch"]],
        node["tier"], node["min_level"], requirements[0], requirements[1],
        node["max_rank"], stats[node["effect"]["stat"]],
        node["effect"]["kind"] == "percent"
    )
    actual = (node_id, title, branch, int(tier), int(level), int(req_a),
              int(req_b), int(max_rank), stat, percent == "true")
    assert actual == expected, (index, expected, actual)
    assert node["cost_per_rank"] == 1, (index, node_id)
    assert math.isclose(float(amount), node["effect"]["amount_per_rank"],
                        rel_tol=0, abs_tol=1e-10), (index, node_id)
    assert all(parent < index for parent in requirements if parent >= 0), (index, node_id)
assert all(sum(n["branch"] == b for n in nodes) == 47 for b in branches)
print("Exact UE5/Godot parity confirmed for all 141 talent nodes.")

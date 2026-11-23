"""Guard exact UE5/Godot 141-node parity and the versioned talent save contract.

This runs portable C++17 talent rules. It does NOT compile or play Unreal Engine.
"""
from __future__ import annotations

import json
import re
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "godot-v38/data/moonfall_talents.json"
HEADER = ROOT / "MoonfallUnreal/Source/MoonfallUnreal/MoonfallTalentRules.h"
HERO_H = ROOT / "MoonfallUnreal/Source/MoonfallUnreal/MoonfallHeroPawn.h"
HERO_CPP = ROOT / "MoonfallUnreal/Source/MoonfallUnreal/MoonfallHeroPawn.cpp"
SAVE = ROOT / "MoonfallUnreal/Source/MoonfallUnreal/MoonfallPersistentSave.h"
TEST_CPP = ROOT / "tools/test_unreal_talent_rules.cpp"

ROW = re.compile(
    r'^\s*\{"([^"]+)","([^"]+)",Branch::(\w+),(\d+),(\d+),(-?\d+),(-?\d+),'
    r'(\d+),Stat::(\w+),([0-9.eE+-]+),(true|false)\},?$',
    re.MULTILINE,
)
BRANCHES = {"might": "Might", "agility": "Agility", "arcana": "Arcana"}
STATS = {
    "health": "Health", "mana": "Mana", "stamina": "Stamina",
    "melee_damage": "MeleeDamage", "ranged_damage": "RangedDamage",
    "magic_damage": "MagicDamage",
}


class UnrealTalentParityTests(unittest.TestCase):
    def test_all_141_godot_nodes_and_prerequisites_match(self) -> None:
        data = json.loads(SOURCE.read_text(encoding="utf-8"))
        source = HEADER.read_text(encoding="utf-8")
        nodes = data["nodes"]
        self.assertEqual(data["schema_version"], 1)
        self.assertEqual(data["points_per_level"], 2)
        self.assertEqual(len(nodes), 141)
        captured = ROW.findall(source)
        self.assertEqual(len(captured), len(nodes), "generated catalog differs from Godot data")
        ids = {node["id"]: i for i, node in enumerate(nodes)}
        for position, (entry, raw) in enumerate(zip(nodes, captured)):
            (name, title, branch, tier, min_level, first, second, max_rank,
             stat, amount, is_percent) = raw
            with self.subTest(node=name):
                self.assertEqual(name, entry["id"])
                self.assertEqual(title, entry["title"])
                self.assertEqual(branch, BRANCHES[entry["branch"]])
                self.assertEqual(int(tier), entry["tier"])
                self.assertEqual(int(min_level), entry["min_level"])
                self.assertEqual(int(max_rank), entry["max_rank"])
                self.assertEqual(entry["cost_per_rank"], 1)
                requirements = [ids[parent] for parent in entry["requires"]]
                self.assertEqual([int(first), int(second)],
                                 (requirements + [-1, -1])[:2])
                self.assertEqual(stat, STATS[entry["effect"]["stat"]])
                self.assertAlmostEqual(float(amount), entry["effect"]["amount_per_rank"])
                self.assertEqual(is_percent == "true", entry["effect"]["kind"] == "percent")
        self.assertEqual([sum(n["branch"] == name for n in nodes) for name in BRANCHES],
                         [47, 47, 47])

    def test_unreal_gameplay_save_hooks(self) -> None:
        h = HERO_H.read_text(encoding="utf-8")
        cpp = HERO_CPP.read_text(encoding="utf-8")
        save = SAVE.read_text(encoding="utf-8")
        self.assertIn("int32 Version = 13", save)
        self.assertIn("UPROPERTY() TArray<int32> TalentRanks;", save)
        self.assertIn("UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=\"Moonfall|Talents\") TArray<int32> TalentRanks;", h)
        for fn in ("GetAvailableTalentPoints", "SpendTalentNode",
                   "GetTalentRank", "GetTalentBonus", "RespecAllTalents"):
            self.assertIn("AMoonfallHeroPawn::" + fn, cpp)
        self.assertIn("ReadMoonfallTalentRanks(Level,TalentRanks", cpp)
        self.assertIn("Save->TalentRanks = TalentRanks;", cpp)
        self.assertIn("Save->Version > 13", cpp)
        self.assertIn("ReadMoonfallTalentRanks(Save->Level,Save->TalentRanks", cpp)
        self.assertIn("Save->Version >= 13 ? Save->TalentRanks : TArray<int32>{}", cpp)
        self.assertIn("bPersistentSaveWritesBlocked = true;", cpp)

    def test_portable_cplusplus_rules(self) -> None:
        compiler = shutil.which("g++") or shutil.which("clang++")
        self.assertIsNotNone(compiler, "A C++17 compiler is required for talent parity checks")
        with tempfile.TemporaryDirectory(prefix="moonfall_talents_") as tmp:
            executable = Path(tmp) / "talent_rules"
            subprocess.run(
                [compiler, "-std=c++17", "-Wall", "-Wextra", "-Werror",
                 "-pedantic", str(TEST_CPP), "-o", str(executable)],
                check=True, cwd=ROOT, capture_output=True, text=True,
            )
            subprocess.run([str(executable)], check=True, cwd=ROOT,
                           capture_output=True, text=True)


if __name__ == "__main__":
    unittest.main()

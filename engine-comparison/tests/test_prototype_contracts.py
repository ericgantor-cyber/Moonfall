"""Static guardrails for the isolated Moonfall engine comparison.

These checks are NOT Unity or Unreal compilation or gameplay tests.
"""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
UNITY = ROOT / "unity/Assets/Scripts/MoonfallPrototype.cs"
CPP = ROOT / "unreal/MoonfallPrototype/Source/MoonfallPrototype/MoonfallPawn.cpp"
HEADER = ROOT / "unreal/MoonfallPrototype/Source/MoonfallPrototype/MoonfallPawn.h"
SAVES = ROOT / "unreal/MoonfallPrototype/Source/MoonfallPrototype/MoonfallProgressSave.h"
INPUT = ROOT / "unreal/MoonfallPrototype/Config/DefaultInput.ini"
UPROJECT = ROOT / "unreal/MoonfallPrototype/MoonfallPrototype.uproject"


class PrototypeContracts(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.unity = UNITY.read_text(encoding="utf-8")
        cls.cpp = CPP.read_text(encoding="utf-8")
        cls.header = HEADER.read_text(encoding="utf-8")

    def test_sources_exist(self):
        for path in (UNITY, CPP, HEADER, SAVES, INPUT, UPROJECT):
            with self.subTest(path=path):
                self.assertTrue(path.is_file(), f"Missing {path}")

    def test_attack_mapping_is_click_only(self):
        input_text = INPUT.read_text(encoding="utf-8")
        self.assertIn('ActionName="Attack",Key=LeftMouseButton', input_text)
        self.assertNotIn('ActionName="Attack",Key=SpaceBar', input_text)
        self.assertIn("Input.GetMouseButtonDown(0)", self.unity)

    def test_dodge_has_recovery_not_permanent_immunity(self):
        self.assertIn("DodgeCooldown = 1.2f;", self.cpp)
        self.assertIn("DodgeCooldown <= 0.f", self.cpp)
        self.assertIn("DodgeCooldown = FMath::Max", self.cpp)
        self.assertIn("dodgeCooldown=1.2f;", self.unity)
        self.assertIn("dodgeCooldown<=0", self.unity)
        self.assertIn("dodgeCooldown = Mathf.Max", self.unity)

    def test_boss_timing_parity(self):
        self.assertIn("CastTimer = 1.15f;", self.cpp)
        self.assertIn("castTimer=1.15f;", self.unity)
        self.assertIn("CastCooldown = 3.5f;", self.cpp)
        self.assertIn("castCd=3.5f;", self.unity)
        self.assertIn("BossHealth - 15.f", self.cpp)
        self.assertIn("bossHp-15", self.unity)

    def test_progression_cap_and_save_isolation(self):
        self.assertIn("FMath::Min(100, Victories + 1)", self.cpp)
        self.assertIn("Mathf.Min(100, wins + 1)", self.unity)
        self.assertIn('MoonfallPrototypeBossRewards', self.cpp)
        self.assertIn('moonfall_experiment_wins', self.unity)
        self.assertIn("SaveGameToSlot", self.cpp)
        self.assertIn("PlayerPrefs.Save()", self.unity)

    def test_godot_inventory_ports_include_guardrails(self):
        unity = (ROOT / "unity/Assets/Scripts/MoonfallInventoryRules.cs").read_text(encoding="utf-8")
        unreal = (ROOT / "unreal/MoonfallPrototype/Source/MoonfallPrototype/MoonfallInventoryRules.h").read_text(encoding="utf-8")
        for src in (unity, unreal):
            self.assertIn("BagStartCapacity = 100", src)
            self.assertIn("BagMaxCapacity = 500", src)
            self.assertIn("MaterialStackSize = 999", src)
            self.assertIn("MaterialLimit = 99999", src)
            self.assertIn("CraftIssue", src)
            self.assertIn("TryCraft", src)
            self.assertIn("Bag full.", src)
            self.assertIn("Invalid recipe cost.", src)

    def test_gear_transfer_sort_and_discard_ports(self):
        unity = (ROOT / "unity/Assets/Scripts/MoonfallGearRules.cs").read_text(encoding="utf-8")
        unreal = (ROOT / "unreal/MoonfallPrototype/Source/MoonfallPrototype/MoonfallGearRules.h").read_text(encoding="utf-8")
        for src in (unity, unreal):
            for token in ("Transfer(", "MatchingIndices(", "FirstDiscardableDrop(",
                          "Common", "Uncommon", "Legendary", "Mythic",
                          "Power", "Rarity", "Name"):
                self.assertIn(token, src)
        self.assertIn("source.RemoveAt(index)", unity)
        self.assertIn("Source.RemoveAt(Index)", unreal)
        self.assertIn("a.CompareTo(b)", unity)
        self.assertIn("A < B", unreal)

    def test_no_artwork_or_production_changes(self):
        self.assertFalse(list(ROOT.rglob("*.png")))
        self.assertFalse(list(ROOT.rglob("*.jpg")))
        self.assertFalse(list(ROOT.rglob("*.godot")))
        self.assertFalse(list(ROOT.rglob("*.tscn")))


if __name__ == "__main__":
    unittest.main()

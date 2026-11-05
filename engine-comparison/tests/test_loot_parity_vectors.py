"""Behavioral reference vectors for Godot equipment_rng.gd rarity rules.

This validates independent mathematical vectors and source contract tokens only.
It does NOT compile C#/C++, call their functions, or prove identical seeded RNG.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]

def rarity_index(chance: int, luck: int = 0, pity: int = 0) -> int:
    if not 0 <= chance <= 999:
        raise ValueError("chance must be 0..999")
    boost = max(0, min(40, luck))
    rarity = (4 if chance < 10 + boost // 2
              else 3 if chance < 50 + boost
              else 2 if chance < 170 + boost * 2
              else 1 if chance < 410 + boost * 3
              else 0)
    if pity >= 30:
        rarity = max(rarity, 2)
    if pity >= 80:
        rarity = max(rarity, 3)
    return rarity

def next_pity(current: int, rarity: int) -> int:
    return 0 if rarity >= 2 else min(80, max(0, current) + 1)

class LootMathVectors(unittest.TestCase):
    def test_base_threshold_edges(self):
        for chance, expected in ((0,4),(9,4),(10,3),(49,3),(50,2),
                                 (169,2),(170,1),(409,1),(410,0),(999,0)):
            with self.subTest(chance=chance):
                self.assertEqual(rarity_index(chance), expected)

    def test_luck_and_pity_edges(self):
        self.assertEqual(rarity_index(29,40),4)
        self.assertEqual(rarity_index(30,40),3)
        self.assertEqual(rarity_index(89,40),3)
        self.assertEqual(rarity_index(90,40),2)
        self.assertEqual(rarity_index(900,0,29),0)
        self.assertEqual(rarity_index(900,0,30),2)
        self.assertEqual(rarity_index(900,0,80),3)
        self.assertEqual(rarity_index(999,1000),0)

    def test_pity_increment_and_reset(self):
        self.assertEqual(next_pity(79,0),80)
        self.assertEqual(next_pity(80,1),80)
        self.assertEqual(next_pity(-4,0),1)
        self.assertEqual(next_pity(44,2),0)

    def test_engine_files_have_parity_contract(self):
        unity=(ROOT/"unity/Assets/Scripts/MoonfallLootRules.cs").read_text()
        unreal=(ROOT/"unreal/MoonfallPrototype/Source/MoonfallPrototype/MoonfallLootRules.h").read_text()
        for source in (unity, unreal):
            for marker in ("RarityIndex(", "NextPity(", "AffixCount(", "PickAffixes(",
                           "10 +", "50 +", "170 +", "410 +", "30", "80"):
                self.assertIn(marker, source)

if __name__ == "__main__":
    unittest.main()

#!/usr/bin/env python3
"""Source-contract checks for Unreal equipment stat validation.

These do not replace Unreal compilation or gameplay tests.
"""
from pathlib import Path
import re
import unittest

SOURCE = (Path(__file__).resolve().parents[1] / "MoonfallUnreal" /
          "Source" / "MoonfallUnreal" / "MoonfallHeroPawn.cpp")


def method_body(source: str, name: str) -> str:
    match = re.search(r"(?:int32|float) AMoonfallHeroPawn::" + re.escape(name) +
                      r"\([^)]*\) const\s*\{", source)
    if not match:
        raise AssertionError(f"Missing {name}")
    depth = 1
    cursor = match.end()
    while cursor < len(source) and depth:
        if source[cursor] == "{":
            depth += 1
        elif source[cursor] == "}":
            depth -= 1
        cursor += 1
    if depth:
        raise AssertionError(f"Unbalanced {name}")
    return source[match.end():cursor - 1]


class UnrealEquipmentStatValidation(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source = SOURCE.read_text(encoding="utf-8")

    def test_decoder_rejects_invalid_slot_rarity_tier_and_encoding(self):
        source = self.source
        for token in ("Code <= 0", "Slot != ExpectedSlot",
                      "Trait < 1 || Trait > 6", "Rarity < 1 || Rarity > 5",
                      "Tier < 1 || Tier > 25",
                      "Code != Trait * 100000 + Slot * 10000 + Rarity * 1000 + Tier"):
            self.assertIn(token, source)

    def test_all_derived_stats_use_validated_equipment(self):
        expectations = {
            "GetSetPieceCount": "DecodeEquippedItem(Codes[Slot], Slot",
            "GetMight": "ValidEquippedTier(EquippedWeaponCode, 0)",
            "GetInsight": "ValidEquippedTier(EquippedAccessoryCode, 2)",
            "GetResolve": "ValidEquippedTier(EquippedArmorCode, 1)",
            "GetCriticalChance": "ValidEquippedRarityBonus(EquippedWeaponCode, 0, 1",
            "GetHasteBonus": "ValidEquippedRarityBonus(EquippedAccessoryCode, 2, 3",
            "GetMasteryBonus": "ValidEquippedTier(EquippedWeaponCode, 0)",
            "GetVersatilityBonus": "ValidEquippedRarityBonus(EquippedArmorCode, 1, 2",
        }
        for method, token in expectations.items():
            with self.subTest(method=method):
                self.assertIn(token, method_body(self.source, method))


if __name__ == "__main__":
    unittest.main()

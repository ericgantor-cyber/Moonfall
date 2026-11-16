"""Source-level Unreal equipment validation checks; native compilation still required."""
from pathlib import Path
import unittest
SOURCE = (Path(__file__).resolve().parents[1] / "MoonfallUnreal" / "Source" / "MoonfallUnreal" / "MoonfallHeroPawn.cpp")
class EquipmentStatSafety(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source = SOURCE.read_text(encoding="utf-8")
    def test_decoder_guards_corrupt_codes(self):
        body = self.source.split("bool DecodeEquippedItem(", 1)[1].split("int32 ValidEquippedTier(", 1)[0]
        for token in ("Code <= 0", "Slot != ExpectedSlot", "Trait < 0 || Trait > 3",
                      "Rarity < 0 || Rarity > 5", "Tier < 1 || Tier > 25",
                      "(Rarity == 0 && Trait != 0)",
                      "Code != Trait * 100000 + Slot * 10000 + Rarity * 1000 + Tier"):
            self.assertIn(token, body)
    def test_stat_bonuses_validate_ownership(self):
        for token in ("InventoryItemCodes.Contains(EquippedWeaponCode)",
                      "InventoryItemCodes.Contains(EquippedAccessoryCode)",
                      "InventoryItemCodes.Contains(EquippedArmorCode)",
                      "ValidEquippedTier(", "ValidEquippedTraitRarity("):
            self.assertIn(token, self.source)
if __name__ == "__main__":
    unittest.main()

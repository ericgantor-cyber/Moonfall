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
        self.assertIn("MoonfallEquipmentSlots::Known(ExpectedSlot)", body)
        self.assertIn("MoonfallEquipmentSlots::Decode(Code)", body)
        self.assertIn("Item.Slot!=ExpectedSlot", body)
        for field in ("Affix", "Rarity", "Tier"):
            self.assertIn("Item."+field, body)
        policy = (SOURCE.parent / "MoonfallEquipmentSlots.h").read_text(encoding="utf-8")
        for token in ("Code<=0", "!Known(Item.Slot)", "Item.Affix>3",
                      "Item.Rarity>5", "Item.Tier<1", "Item.Tier>25",
                      "Item.Rarity==0&&Item.Affix!=0"):
            self.assertIn(token, policy)
    def test_stat_bonuses_validate_ownership(self):
        for token in ("InventoryItemCodes.Contains(EquippedWeaponCode)",
                      "InventoryItemCodes.Contains(EquippedAccessoryCode)",
                      "InventoryItemCodes.Contains(EquippedArmorCode)",
                      "ValidEquippedTier(", "ValidEquippedTraitRarity("):
            self.assertIn(token, self.source)
    def test_expanded_stats_use_owned_tiers_and_preserve_caps(self):
        for fn, slots in (("GetMight", ("Shoulders", "Hands")),
                          ("GetInsight", ("Head", "Ring")),
                          ("GetResolve", ("Legs", "Feet"))):
            body = self.source.split("AMoonfallHeroPawn::"+fn+"() const", 1)[1].split("\n}", 1)[0]
            self.assertIn("MoonfallEquipmentSlots::ExtraPrimaryBonus(", body)
            for slot in slots:
                self.assertIn("GetValidatedEquippedTier(MoonfallEquipmentSlots::"+slot+")", body)
            self.assertIn(", 10, 100);", body)
        mastery = self.source.split("AMoonfallHeroPawn::GetMasteryBonus() const", 1)[1].split("\n}", 1)[0]
        self.assertEqual(mastery.count("GetValidatedEquippedTier(MoonfallEquipmentSlots::"), 6)
        self.assertIn("ExtraUnits * 0.000375f, 0.f, 0.12f", mastery)
        self.assertIn("0.f, 0.16f", mastery)
        tier = self.source.split("AMoonfallHeroPawn::GetValidatedEquippedTier(int32 Slot) const", 1)[1].split("\n}", 1)[0]
        self.assertLess(tier.index("Code <= 0"), tier.index("InventoryItemCodes.Contains(Code)"))
if __name__ == "__main__":
    unittest.main()

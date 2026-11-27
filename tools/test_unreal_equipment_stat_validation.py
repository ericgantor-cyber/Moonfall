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
        self.assertIn("ExpectedSlot >= MoonfallEquipmentLayout::ActiveSlotCount", body)
        self.assertIn("!MoonfallEquipmentLayout::Valid(Code, ExpectedSlot)", body)
        policy = SOURCE.with_name("MoonfallGearAdvisorPolicy.h").read_text(encoding="utf-8")
        for token in ("Code <= 0", "EncodedSlot == Slot", "Affix >= 0 && Affix <= 3",
                      "Rarity >= 0", "Rarity <= 5", "Tier >= 1", "Tier <= 25",
                      "(Rarity != 0 || Affix == 0)",
                      "Code == Affix * 100000 + Slot * 10000 + Rarity * 1000 + Tier",
                      "MoonfallGearAdvisor::Valid(Code-Slot*10000,0)"):
            self.assertIn(token, policy)
    def test_stat_bonuses_validate_ownership(self):
        getter = self.source.split("int32 AMoonfallHeroPawn::GetEquipmentSlotCode(", 1)[1].split("int32 AMoonfallHeroPawn::GetValidatedEquippedTier(", 1)[0]
        tier = self.source.split("int32 AMoonfallHeroPawn::GetValidatedEquippedTier(", 1)[1].split("int32 AMoonfallHeroPawn::GetItemPower(", 1)[0]
        self.assertIn("GetValidatedEquippedTier(Slot) > 0 ? Code : 0", getter)
        self.assertIn("InventoryItemCodes.Contains(Code) ? ValidEquippedTier(Code, Slot) : 0", tier)
        self.assertIn("Slot < 0 || Slot >= MoonfallEquipmentLayout::ActiveSlotCount", tier)
        self.assertIn("GetRawEquipmentSlotCode(Slot)", getter)
        self.assertIn("GetRawEquipmentSlotCode(Slot)", tier)
        raw = self.source.split("int32 AMoonfallHeroPawn::GetRawEquipmentSlotCode(", 1)[1].split("int32 AMoonfallHeroPawn::GetEquipmentSlotCode(", 1)[0]
        for slot, field in enumerate(("EquippedWeaponCode", "EquippedArmorCode", "EquippedAccessoryCode")):
            self.assertIn(f"case {slot}: return {field};", raw)
        self.assertIn("Slot >= MoonfallEquipmentLayout::SlotCount) return 0;", raw)
        self.assertIn("Slot < MoonfallEquipmentLayout::LegacySlotCount", raw)
        self.assertIn("return ExtraEquippedCodes[Slot-MoonfallEquipmentLayout::LegacySlotCount];", raw)
        for name, slot, trait in (("Weapon", 0, 1), ("Armor", 1, 2), ("Accessory", 2, 3)):
            self.assertIn(f"Owned{name}Code = GetEquipmentSlotCode({slot})", self.source)
            self.assertIn(f"ValidEquippedTraitRarity(Owned{name}Code, {slot}, {trait})", self.source)
            self.assertIn(f"ValidEquippedTier(Owned{name}Code, {slot})", self.source)
        self.assertIn("const float Tier = static_cast<float>(GetValidatedEquippedTier(0));", self.source)
if __name__ == "__main__":
    unittest.main()

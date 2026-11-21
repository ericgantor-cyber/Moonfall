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
    def test_power_comparison_uses_validated_slot_accessor(self):
        body = self.source.split("int32 AMoonfallHeroPawn::GetItemPowerDelta(", 1)[1].split(
            "int32 AMoonfallHeroPawn::GetProjectedSetPieceCount(", 1)[0]
        for token in ("InventoryItemCodes.Contains(ItemCode)",
                      "GetItemPower(ItemCode)",
                      "MoonfallEquipmentSlots::IsLegacySlot(Slot)",
                      "GetEquipmentSlotCode(Slot)",
                      "CurrentCode > 0 ? GetItemPower(CurrentCode) : 0"):
            self.assertIn(token, body)
        self.assertNotIn("EquippedWeaponCode", body)
        self.assertNotIn("DecodeEquippedItem(CurrentCode", body)

    def test_projected_set_bonus_reuses_validated_equipped_slot(self):
        body = self.source.split("int32 AMoonfallHeroPawn::GetProjectedSetPieceCount(", 1)[1].split(
            "bool AMoonfallHeroPawn::HasLegendaryPerk(", 1)[0]
        self.assertIn("GetEquipmentSlotCode(Slot)", body)
        self.assertIn("CurrentCode > 0", body)
        self.assertNotIn("EquippedWeaponCode", body)

    def test_right_click_does_not_equip_behind_character_details(self):
        body = self.source.split("void AMoonfallHeroPawn::CastRightMouse()", 1)[1].split(
            "if (bResidentContextOpen)", 1)[0]
        self.assertIn("if (bInventoryOpen) {", body)
        self.assertIn("if (bCharacterStatsOpen) return;", body)
        self.assertLess(body.index("if (bCharacterStatsOpen) return;"),
                        body.index("EquipInventoryItem(HitIndex);"))

    def test_total_power_uses_only_validated_owned_gear(self):
        body = self.source.split("int32 AMoonfallHeroPawn::GetEquipmentPower()", 1)[1].split(
            "int32 AMoonfallHeroPawn::GetItemPowerDelta(", 1)[0]
        self.assertIn("GetEquipmentSlotCode(Slot)", body)
        self.assertIn("MoonfallEquipmentSlots::LegacyCount", body)
        self.assertNotIn("EquippedWeaponCode", body)
        self.assertNotIn("DecodeEquippedItem(", body)

    def test_legendary_perks_reject_unowned_and_future_slots(self):
        body = self.source.split("bool AMoonfallHeroPawn::HasLegendaryPerk(", 1)[1].split(
            "int32 AMoonfallHeroPawn::GetSetPieceCount(", 1)[0]
        self.assertIn("MoonfallEquipmentSlots::IsLegacySlot(Slot)", body)
        self.assertIn("GetEquipmentSlotCode(Slot)", body)
        self.assertIn("Code <= 0", body)
        self.assertIn("DecodeEquippedItem(Code, Slot", body)
        self.assertNotIn("EquippedWeaponCode", body)

    def test_stat_bonuses_validate_ownership(self):
        for token in ("InventoryItemCodes.Contains(EquippedWeaponCode)",
                      "InventoryItemCodes.Contains(EquippedAccessoryCode)",
                      "InventoryItemCodes.Contains(EquippedArmorCode)",
                      "ValidEquippedTier(", "ValidEquippedTraitRarity("):
            self.assertIn(token, self.source)
if __name__ == "__main__":
    unittest.main()

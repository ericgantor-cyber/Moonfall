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

    def test_set_piece_count_rejects_invalid_equipped_slots(self):
        body = self.source.split("int32 AMoonfallHeroPawn::GetSetPieceCount(", 1)[1].split(
            "int32 AMoonfallHeroPawn::GetMight(", 1)[0]
        self.assertIn("MoonfallEquipmentSlots::LegacyCount", body)
        self.assertIn("GetEquipmentSlotCode(Slot)", body)
        self.assertIn("if (Code <= 0) continue;", body)
        self.assertIn("DecodeEquippedItem(Code, Slot", body)
        self.assertNotIn("EquippedWeaponCode", body)
        self.assertNotIn("InventoryItemCodes.Contains(Codes[Slot])", body)

    def test_stat_bonuses_validate_ownership(self):
        tier = self.source.split("int32 AMoonfallHeroPawn::GetValidatedEquippedTier(", 1)[1].split(
            "int32 AMoonfallHeroPawn::GetItemPower(", 1)[0]
        self.assertIn("MoonfallEquipmentSlots::IsLegacySlot(Slot)", tier)
        self.assertIn("GetEquipmentSlotCode(Slot)", tier)
        self.assertIn("ValidEquippedTier(Code, Slot)", tier)

        body = "int32 AMoonfallHeroPawn::GetMight()" + self.source.split("int32 AMoonfallHeroPawn::GetMight()", 1)[1].split(
            "AMoonfallHeroPawn::AMoonfallHeroPawn()", 1)[0]
        expected = {
            "GetMight": "GetValidatedEquippedTier(0)",
            "GetInsight": "GetValidatedEquippedTier(2)",
            "GetResolve": "GetValidatedEquippedTier(1)",
            "GetCriticalChance": "GetEquipmentSlotCode(0)",
            "GetHasteBonus": "GetEquipmentSlotCode(2)",
            "GetMasteryBonus": "GetValidatedEquippedTier(2)",
            "GetVersatilityBonus": "GetEquipmentSlotCode(1)",
        }
        for name, token in expected.items():
            method = body.split("AMoonfallHeroPawn::" + name + "()", 1)[1].split("\\n}", 1)[0]
            self.assertIn(token, method, name)
            self.assertNotIn("InventoryItemCodes.Contains(Equipped", method, name)
        self.assertIn("ValidEquippedTraitRarity(", body)
if __name__ == "__main__":
    unittest.main()

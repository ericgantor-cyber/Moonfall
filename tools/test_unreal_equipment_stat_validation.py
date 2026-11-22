"""Source-level Unreal equipment validation checks; native compilation still required."""
from pathlib import Path
import unittest
SOURCE = (Path(__file__).resolve().parents[1] / "MoonfallUnreal" / "Source" / "MoonfallUnreal" / "MoonfallHeroPawn.cpp")
SAVE = (Path(__file__).resolve().parents[1] / "MoonfallUnreal" / "Source" / "MoonfallUnreal" / "MoonfallPersistentSave.h")
class EquipmentStatSafety(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source = SOURCE.read_text(encoding="utf-8")
        cls.save_header = SAVE.read_text(encoding="utf-8")
    def test_v13_equipment_save_migration_is_fail_closed(self):
        self.assertIn("UPROPERTY() int32 Version = 12;", self.save_header)
        self.assertIn("UPROPERTY() TArray<int32> ExpandedEquippedCodes;", self.save_header)
        save = self.source.split("bool AMoonfallHeroPawn::SaveWorldProgress()", 1)[1].split(
            "bool AMoonfallHeroPawn::LoadWorldProgress()", 1)[0]
        load = self.source.split("bool AMoonfallHeroPawn::LoadWorldProgress()", 1)[1].split(
            "void AMoonfallHeroPawn::RespawnAtCheckpoint()", 1)[0]
        for token in ("EquippedExtraItemCodes.Num() != MoonfallEquipmentSlots::FutureCount",
                      "MoonfallEquipmentSlots::IsCanonicalForSlot(Code, Slot)",
                      "InventoryItemCodes.Contains(Code)",
                      "Save->Version = MoonfallEquipmentSlots::ExpandedSaveVersion",
                      "Save->ExpandedEquippedCodes.Init(0, MoonfallEquipmentSlots::SlotCount)",
                      "if (bNeedsV13)"):
            self.assertIn(token, save)
        self.assertIn("Save->Version > MoonfallEquipmentSlots::ExpandedSaveVersion", load)
        self.assertIn("Save->ExpandedEquippedCodes.Num() != MoonfallEquipmentSlots::SlotCount", load)
        self.assertIn("Code != SavedWornCodes[Slot]", load)
        self.assertIn("Save->InventoryItemCodes.Contains(Code)", load)
        self.assertIn("bPersistentSaveWritesBlocked = true;", load)
        self.assertIn("EquippedExtraItemCodes.Init(0, MoonfallEquipmentSlots::FutureCount)", load)
        self.assertLess(load.index("Save->ExpandedEquippedCodes.Num() != MoonfallEquipmentSlots::SlotCount"),
                        load.index("bWorldProgressLoadInProgress = true;"))

    def test_nine_slot_loot_and_equip_validate_owned_items(self):
        getter = self.source.split(
            "int32 AMoonfallHeroPawn::GetEquipmentSlotCode(", 1)[1].split(
            "int32 AMoonfallHeroPawn::GetValidatedEquippedTier(", 1)[0]
        self.assertIn("MoonfallEquipmentSlots::IsFutureSlot(Slot)", getter)
        self.assertIn("MoonfallEquipmentSlots::IsCanonicalForSlot(ExtraCode, Slot)", getter)
        self.assertIn("InventoryItemCodes.Contains(ExtraCode)", getter)
        equip = self.source.split(
            "bool AMoonfallHeroPawn::EquipInventoryItemWithPreviousTier(", 1)[1].split(
            "namespace {", 1)[0]
        self.assertIn("InventoryItemCodes.IsValidIndex(InventoryIndex)", equip)
        self.assertIn("MoonfallEquipmentSlots::IsKnownSlot(SlotIndex)", equip)
        self.assertIn("MoonfallEquipmentSlots::IsFutureSlot(SlotIndex)", equip)
        self.assertIn("EquippedExtraItemCodes[Index] = Code;", equip)
        self.assertLess(equip.index("if (MoonfallEquipmentSlots::IsFutureSlot(SlotIndex))"),
                        equip.index("const float HealthDelta = 2.f * (Tier - PreviousTier);"))
        roll = self.source.split("int32 AMoonfallHeroPawn::RollEquipmentCode(", 1)[1].split(
            "void AMoonfallHeroPawn::AwardEquipmentDrop(", 1)[0]
        self.assertIn("FMath::RandRange(0, MoonfallEquipmentSlots::SlotCount - 1)", roll)
        sets = self.source.split("int32 AMoonfallHeroPawn::GetSetPieceCount(", 1)[1].split(
            "int32 AMoonfallHeroPawn::GetMight(", 1)[0]
        self.assertIn("MoonfallEquipmentSlots::SlotCount", sets)
        self.assertIn("return Pieces;", sets)

    def test_salvage_all_nine_slots_and_protect_equipped_ownership(self):
        salvage = self.source.split(
            "bool AMoonfallHeroPawn::SalvageInventoryItem(", 1
        )[1].split("bool AMoonfallHeroPawn::UpgradeInventoryItem(", 1)[0]
        self.assertIn("MoonfallEquipmentSlots::IsKnownSlot(SlotIndex)", salvage)
        self.assertNotIn("SlotIndex < 0 || SlotIndex > 2", salvage)
        self.assertIn("MoonfallEquipmentSlots::IsKnownSlot(WornSlot)", salvage)
        self.assertIn("Code == GetEquipmentSlotCode(WornSlot)", salvage)
        self.assertIn("if (Copies <= 1) return false;", salvage)
        self.assertIn("Code != Affix * 100000 + SlotIndex * 10000 + Rarity * 1000 + Tier", salvage)
        self.assertLess(salvage.index("if (Copies <= 1) return false;"),
                        salvage.index("Salvage = SafeBalance +"))
        self.assertLess(salvage.index("Salvage = SafeBalance +"),
                        salvage.index("InventoryItemCodes.RemoveAt(InventoryIndex);"))

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
                      "MoonfallEquipmentSlots::IsKnownSlot(Slot)",
                      "GetEquipmentSlotCode(Slot)",
                      "CurrentCode > 0 ? GetItemPower(CurrentCode) : 0"):
            self.assertIn(token, body)
        self.assertNotIn("EquippedWeaponCode", body)
        self.assertNotIn("DecodeEquippedItem(CurrentCode", body)

    def test_nine_slot_set_preview_does_not_cap_raw_counts_at_three(self):
        projected = self.source.split(
            "int32 AMoonfallHeroPawn::GetProjectedSetPieceCount(", 1
        )[1].split("bool AMoonfallHeroPawn::HasLegendaryPerk(", 1)[0]
        actual = self.source.split(
            "int32 AMoonfallHeroPawn::GetSetPieceCount(", 1
        )[1].split("int32 AMoonfallHeroPawn::GetMight(", 1)[0]
        self.assertIn("GetSetPieceCount(Affix)", projected)
        self.assertIn("GetEquipmentSlotCode(Slot)", projected)
        self.assertIn("MoonfallEquipmentSlots::SlotCount", projected)
        self.assertNotIn("0, 3);", projected)
        self.assertIn("MoonfallEquipmentSlots::SlotCount", actual)
        self.assertIn("return Pieces;", actual)

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
        self.assertIn("MoonfallEquipmentSlots::SlotCount", body)
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
        self.assertIn("MoonfallEquipmentSlots::SlotCount", body)
        self.assertIn("GetEquipmentSlotCode(Slot)", body)
        self.assertIn("if (Code <= 0) continue;", body)
        self.assertIn("DecodeEquippedItem(Code, Slot", body)
        self.assertNotIn("EquippedWeaponCode", body)
        self.assertNotIn("InventoryItemCodes.Contains(Codes[Slot])", body)

    def test_crafting_blocks_reentrant_save_and_inventory_actions(self):
        header = (SOURCE.parent / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        self.assertIn("bool bEquipmentCraftInProgress = false;", header)
        saver = self.source.split("bool AMoonfallHeroPawn::SaveWorldProgress()", 1)[1].split(
            "bool AMoonfallHeroPawn::LoadWorldProgress()", 1)[0]
        loader = self.source.split("bool AMoonfallHeroPawn::LoadWorldProgress()", 1)[1].split(
            "void AMoonfallHeroPawn::RespawnAtCheckpoint()", 1)[0]
        self.assertIn("|| bEquipmentCraftInProgress || bPersistentSaveWritesBlocked", saver)
        self.assertIn("if (bEquipmentCraftInProgress) return false;", loader)
        for method in ("EquipInventoryItem", "SalvageInventoryItem"):
            body = self.source.split(
                "bool AMoonfallHeroPawn::" + method + "(int32 InventoryIndex)", 1
            )[1].split("\n}", 1)[0]
            self.assertIn("bEquipmentCraftInProgress", body, method)
        best = self.source.split(
            "int32 AMoonfallHeroPawn::EquipBestInventoryUpgrades()", 1
        )[1].split("\n}", 1)[0]
        self.assertIn("if (bEquipmentCraftInProgress || !bInventoryOpen", best)
        for method in ("UpgradeInventoryItem", "ReforgeInventoryItemAffix",
                       "AscendInventoryItemRarity"):
            body = self.source.split(
                "bool AMoonfallHeroPawn::" + method + "(int32 InventoryIndex)", 1
            )[1].split("\n}", 1)[0]
            self.assertIn("if (bEquipmentCraftInProgress || !bInventoryOpen", body, method)
            self.assertLess(body.index("bEquipmentCraftInProgress = true;"),
                            body.index("InventoryItemCodes[InventoryIndex] = NewCode;"), method)
            rollback = body.index("InventoryItemCodes[InventoryIndex] = Code;")
            self.assertLess(rollback,
                            body.index("bEquipmentCraftInProgress = false;", rollback), method)
            self.assertEqual(body.count("bEquipmentCraftInProgress = false;"), 2, method)
            self.assertLess(body.rindex("bEquipmentCraftInProgress = false;"),
                            body.index("OnEquipment", body.index("Salvage -= Cost;")), method)

    def test_worn_armor_crafting_preserves_validated_previous_tier(self):
        # The inventory code is rewritten before re-equip, so the old worn
        # code is no longer owned. Without the pre-mutation tier override,
        # changing the affix/rarity grants free armor max health.
        functions = (
            ("UpgradeInventoryItem", "ReforgeInventoryItemAffix"),
            ("ReforgeInventoryItemAffix", "ReforgeSelectedInventoryItem"),
            ("AscendInventoryItemRarity", "AscendSelectedInventoryItem"),
        )
        for name, next_name in functions:
            body = self.source.split(
                "AMoonfallHeroPawn::" + name + "(", 1
            )[1].split("AMoonfallHeroPawn::" + next_name + "(", 1)[0]
            self.assertIn("PreviousEquippedTier", body, name)
            self.assertIn("GetValidatedEquippedTier(Slot)", body, name)
            self.assertIn("EquipInventoryItemWithPreviousTier(InventoryIndex, PreviousEquippedTier)", body, name)
            self.assertLess(body.index("GetValidatedEquippedTier(Slot)"),
                            body.index("InventoryItemCodes[InventoryIndex] = NewCode"), name)
            self.assertNotIn("!EquipInventoryItem(InventoryIndex)", body, name)

        equip = self.source.split(
            "bool AMoonfallHeroPawn::EquipInventoryItem(int32 InventoryIndex)", 1
        )[1].split("namespace {", 1)[0]
        self.assertIn("return EquipInventoryItemWithPreviousTier(InventoryIndex, -1);", equip)
        self.assertIn("const int32 PreviousTier = PreviousTierOverride >= 0", equip)
        self.assertIn("FMath::Clamp(PreviousTierOverride, 0, 25)", equip)
        self.assertIn("const float HealthDelta = 2.f * (Tier - PreviousTier);", equip)

    def test_stat_bonuses_validate_ownership(self):
        tier = self.source.split("int32 AMoonfallHeroPawn::GetValidatedEquippedTier(", 1)[1].split(
            "int32 AMoonfallHeroPawn::GetItemPower(", 1)[0]
        self.assertIn("MoonfallEquipmentSlots::IsKnownSlot(Slot)", tier)
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

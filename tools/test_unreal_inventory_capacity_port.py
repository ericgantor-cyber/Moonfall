#!/usr/bin/env python3
"""Source-level port contract for Godot inventory capacity rules in UE5.

This does not substitute for native Unreal compilation or live pickup testing.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1] / "MoonfallUnreal" / "Source" / "MoonfallUnreal"


def body(source: str, signature: str) -> str:
    start = source.index(signature)
    opening = source.index("{", start)
    depth = 1
    end = opening + 1
    while depth and end < len(source):
        if source[end] == "{":
            depth += 1
        elif source[end] == "}":
            depth -= 1
        end += 1
    assert depth == 0
    return source[opening + 1 : end - 1]


class InventoryCapacityPort(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.hero = (ROOT / "MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        cls.loot = (ROOT / "MoonfallGroundLoot.cpp").read_text(encoding="utf-8")

    def test_scripted_reward_fails_before_mutation(self):
        method = body(self.hero, "void AMoonfallHeroPawn::AwardEquipmentDrop(")
        guard = "if (InventoryItemCodes.Num() >= MaxInventoryItems) return;"
        self.assertIn("MaxInventoryItems = 9999;", (ROOT / "MoonfallHeroPawn.h").read_text(encoding="utf-8"))
        self.assertLess(method.index(guard), method.index("InventoryItemCodes.Add(ItemCode);"))
        self.assertLess(method.index(guard), method.index("LastLootAffix ="))

    def test_ground_collection_fails_without_phantom_receipt(self):
        method = body(self.hero, "bool AMoonfallHeroPawn::CollectGroundEquipmentCode(")
        guard = "if (InventoryItemCodes.Num() >= MaxInventoryItems) return false;"
        self.assertIn("MaxInventoryItems = 9999;", (ROOT / "MoonfallHeroPawn.h").read_text(encoding="utf-8"))
        self.assertLess(method.index(guard), method.index("InventoryItemCodes.Add(ItemCode);"))
        self.assertLess(method.index(guard), method.index("LastLootAffix ="))

    def test_inventory_hud_displays_capacity_and_full_warning(self):
        hud = (ROOT / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        self.assertIn("AMoonfallHeroPawn::MaxInventoryItems", hud)
        self.assertIn('TEXT("BAG FULL - salvage unused gear to collect more loot")', hud)

    def test_expanded_inventory_preserves_mouse_row_selection(self):
        hud = (ROOT / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        self.assertEqual(hud.count("const bool bWideInventory = Canvas->SizeX >= 1100.f;"), 3)
        self.assertIn("if (X < Left + 12.f || X > Left + ListWidth - 12.f) return -1;", hud)
        self.assertIn("Left + 12.f, Y - 3.f, ListWidth - 24.f, RowSpacing - 3.f);", hud)
        self.assertIn('TEXT("GEAR INSPECTION")', hud)
        self.assertIn('TEXT("CRAFTING & SALVAGE")', hud)
        self.assertIn("GetEquipmentUpgradeScore(Selected)", hud)
        self.assertIn("bWideInventory ? 1040.f : 680.f", hud)

    def test_inventory_sort_is_explicit_and_does_not_modify_gear(self):
        method = body(self.hero, "void AMoonfallHeroPawn::CycleInventorySort()")
        self.assertIn("!bInventoryOpen || bCharacterStatsOpen || bOptionsOpen", method)
        self.assertIn("InventoryItemCodes.StableSort(", method)
        self.assertIn("SelectedOccurrence", method)
        self.assertIn("InventorySelection = Index;", method)
        self.assertIn("if (AValid != BValid) return AValid;", method)
        self.assertIn("if (Mode == 2 && ASlot != BSlot) return ASlot < BSlot;", method)
        for forbidden in ("InventoryItemCodes.RemoveAt(", "Salvage -=", "EquipInventoryItem(",
                          "InventoryItemCodes.Empty(", "Destroy("):
            self.assertNotIn(forbidden, method)

    def test_sort_controls_and_visible_mode(self):
        config = (ROOT.parents[1] / "Config" / "DefaultInput.ini").read_text(encoding="utf-8")
        hud = (ROOT / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        self.assertIn('ActionName="InventorySort",Key=F8', config)
        self.assertIn('ActionName="InventorySort",Key=Gamepad_LeftThumbstick', config)
        self.assertIn('Input->BindAction(TEXT("InventorySort")', self.hero)
        self.assertIn("InventorySortMode == 1", hud)
        self.assertIn('TEXT("F8 / L3: sort gear', hud)

    def test_clickable_sort_button_changes_sort_without_equipping(self):
        hud = (ROOT / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        header = (ROOT / "MoonfallCombatHUD.h").read_text(encoding="utf-8")
        self.assertIn("IsInventorySortButtonAtScreenPosition(", header)
        self.assertIn("bool AMoonfallCombatHUD::IsInventorySortButtonAtScreenPosition(", hud)
        self.assertIn('TEXT("SORT: %s")', hud)
        self.assertIn("ButtonX + 148.f", hud)
        self.assertIn("ButtonY + 33.f", hud)
        left = body(self.hero, "void AMoonfallHeroPawn::LeftClickContextAction()")
        self.assertIn("HUD->IsInventorySortButtonAtScreenPosition(this, MouseX, MouseY)", left)
        self.assertLess(left.index("CycleInventorySort();"), left.index("GetInventoryItemAtScreenPosition"))
        self.assertLess(left.index("CycleInventorySort();"), left.index("InventorySelection = ClickedIndex"))
        self.assertNotIn("EquipInventoryItem(", left)

    def test_inventory_has_no_equip_or_page_buttons(self):
        hud = (ROOT / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        left = body(self.hero, "void AMoonfallHeroPawn::LeftClickContextAction()")
        self.assertNotIn("IsInventoryEquipButtonAtScreenPosition", hud + left)
        self.assertNotIn('TEXT("EQUIP SELECTED")', hud)
        self.assertNotIn('TEXT("PREV PAGE")', hud)
        self.assertNotIn('TEXT("NEXT PAGE")', hud)
        self.assertNotIn("EquipSelectedInventoryItem();", left)
        self.assertIn("GetInventoryItemAtScreenPosition", left)

    def test_right_click_requires_a_visible_item_row_to_equip(self):
        right = body(self.hero, "void AMoonfallHeroPawn::RightClickContextAction()")
        guard = "if (InventoryItemCodes.IsValidIndex(ClickedIndex)) {"
        self.assertIn(guard, right)
        selected = right.index("InventorySelection = ClickedIndex;")
        equipped = right.index("EquipSelectedInventoryItem();")
        end_guard = right.index("\n     }", equipped)
        self.assertLess(right.index(guard), selected)
        self.assertLess(selected, equipped)
        self.assertLess(equipped, end_guard)
        self.assertNotIn("EquipSelectedInventoryItem();", right[end_guard:])
        self.assertIn("HUD->GetInventoryItemAtScreenPosition(this, MouseX, MouseY)", right)
        self.assertNotIn("IsInventoryEquipButtonAtScreenPosition", right)

    def test_confirm_shortcuts_removed_but_right_click_stays(self):
        inputs = (ROOT.parents[1] / "Config" / "DefaultInput.ini").read_text(encoding="utf-8")
        hud = (ROOT / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        header = (ROOT / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        for source in (inputs, self.hero, header):
            self.assertNotIn("InventoryConfirm", source)
        self.assertNotIn('ActionName="InventoryConfirm",Key=Enter', inputs)
        self.assertNotIn('ActionName="InventoryConfirm",Key=Gamepad_FaceButton_Top', inputs)
        self.assertIn("EquipSelectedInventoryItem();", body(self.hero, "void AMoonfallHeroPawn::RightClickContextAction()"))
        self.assertIn("Right-click row: equip", hud)
        self.assertIn('ActionName="Atlas",Key=J', inputs)
        self.assertNotIn('TEXT("EQUIP SELECTED")', hud)

    def test_failed_pickup_keeps_world_actor(self):
        method = body(self.loot, "bool AMoonfallGroundLoot::TryCollect()")
        self.assertLess(
            method.index("if (!Hero->CollectGroundEquipmentCode(ItemCode)) return false;"),
            method.index("bCollected = true;"),
        )
        self.assertLess(method.index("bCollected = true;"), method.index("Destroy();"))


if __name__ == "__main__":
    unittest.main()

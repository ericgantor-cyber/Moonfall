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
        self.assertEqual(hud.count("const bool bWideInventory = Canvas->SizeX >= 1100.f && Canvas->SizeY >= 600.f;"), 3)
        self.assertIn("if (X < Left + 12.f || X > Left + ListWidth - 12.f) return -1;", hud)
        self.assertIn("Left + 12.f, Y - 3.f, ListWidth - 24.f, RowSpacing - 3.f);", hud)
        self.assertIn('TEXT("GEAR INSPECTION")', hud)
        self.assertIn('TEXT("CRAFTING & SALVAGE")', hud)
        self.assertIn("GetEquipmentUpgradeScore(Selected)", hud)
        self.assertIn("bWideInventory ? 1040.f : 680.f", hud)
        # Short desktop windows use the compact layout so the 339px detail
        # stack does not run into the panel footer.
        self.assertIn("Canvas->SizeY >= 600.f", hud)
        self.assertIn("Hero->Salvage >= UpgradeCost", hud)
        self.assertIn("Hero->Salvage >= AscendCost", hud)
        self.assertIn("Hero->Salvage >= ReforgeCost", hud)
        for forbidden in ('TEXT("EQUIP SELECTED")', 'TEXT("PREV PAGE")',
                          'TEXT("NEXT PAGE")'):
            self.assertNotIn(forbidden, hud)

    def test_inventory_sort_is_explicit_and_does_not_modify_gear(self):
        method = body(self.hero, "void AMoonfallHeroPawn::CycleInventorySort()")
        self.assertIn("!bBagPanelOpen || bOptionsOpen", method)
        self.assertIn("InventoryItemCodes.StableSort(", method)
        # Sorting changes persisted bag order, so the 16s autosave must see it.
        self.assertLess(method.index("bProgressDirty = true;"), method.index("if (InventoryItemCodes.Num() < 2) return;"))
        self.assertIn("SelectedOccurrence", method)
        self.assertIn("InventorySelection = Index;", method)
        self.assertIn("if (AValid != BValid) return AValid;", method)
        self.assertIn("if (Mode == 2 && ASlot != BSlot) return ASlot < BSlot;", method)
        for forbidden in ("InventoryItemCodes.RemoveAt(", "Salvage -=", "EquipInventoryItem(",
                          "InventoryItemCodes.Empty(", "Destroy("):
            self.assertNotIn(forbidden, method)

    def test_auto_expanding_key_ring_pickups_and_persistence(self):
        header = (ROOT / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        save = (ROOT / "MoonfallProgressSaveGame.h").read_text(encoding="utf-8")
        loot = (ROOT / "MoonfallGroundLoot.cpp").read_text(encoding="utf-8")
        hud = (ROOT / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        for member in ("TArray<int32> KeyRingIds;", "bool AddKeyToRing(int32 KeyId)",
                       "bool HasKey(int32 KeyId) const",
                       "bool SpawnGroundKeyDrop(int32 KeyId, FVector DropLocation)"):
            self.assertIn(member, header)
        self.assertIn("UPROPERTY(SaveGame) TArray<int32> KeyRingIds;", save)
        ring = body(self.hero, "bool AMoonfallHeroPawn::AddKeyToRing(int32 KeyId)")
        for statement in ("KeyRingIds.Contains(KeyId)", "KeyRingIds.Add(KeyId);",
                          "KeyRingIds.Sort();", "bProgressDirty = true;"):
            self.assertIn(statement, ring)
        self.assertNotIn("InventoryItemCodes.Add", ring)
        self.assertNotIn("MaxInventoryItems", ring)
        collector = body(self.hero, "bool AMoonfallHeroPawn::CollectGroundEquipmentCode(int32 ItemCode)")
        self.assertLess(collector.index("return AddKeyToRing("),
                        collector.index("InventoryItemCodes.Num() >= MaxInventoryItems"))
        self.assertIn("AMoonfallHeroPawn::KeyLootBase", loot)
        self.assertIn("KEY %d  [F / X]", loot)
        self.assertIn("Save->KeyRingIds = KeyRingIds;", self.hero)
        self.assertIn("KeyRingIds = Save->KeyRingIds;", self.hero)
        self.assertIn("Save->KeyRingIds[I-1] >= Id", self.hero)
        self.assertIn("DrawKeyRingSummary(", hud)
        self.assertIn('TEXT("KEY RING  %d  %s")', hud)

    def test_compact_b_c_pane_focus_and_bag_only_actions(self):
        hud = (ROOT / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        header = (ROOT / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        inputs = (ROOT.parents[1] / "Config" / "DefaultInput.ini").read_text(encoding="utf-8")
        self.assertIn("void SwitchOpenInventoryPane();", header)
        self.assertIn('ActionName="InventoryPaneSwitch",Key=Gamepad_RightThumbstick', inputs)
        self.assertIn('ActionName="InventoryPaneSwitch",Key=BackSlash', inputs)
        self.assertIn('BindAction(TEXT("InventoryPaneSwitch"), IE_Pressed', self.hero)
        switch = body(self.hero, "void AMoonfallHeroPawn::SwitchOpenInventoryPane()")
        self.assertIn("bMenuFocusCharacter = !bMenuFocusCharacter;", switch)
        self.assertIn("MenuActionFocus = -1;", switch)
        self.assertIn("const bool bDrawCharacter=Hero->bCharacterStatsOpen", hud)
        self.assertIn("!Hero->bBagPanelOpen || bDual || Hero->bMenuFocusCharacter", hud)
        hit = body(hud, "int32 AMoonfallCombatHUD::GetInventoryActionAtScreenPosition(")
        self.assertIn("Hero->bMenuFocusCharacter", hit)
        gear = body(hud, "int32 AMoonfallCombatHUD::GetCharacterActionAtScreenPosition(")
        self.assertIn("!Hero->bMenuFocusCharacter", gear)
        salvage = body(self.hero, "void AMoonfallHeroPawn::InventorySalvage()")
        self.assertIn("bCharacterStatsOpen && bMenuFocusCharacter", salvage)
        sorter = body(self.hero, "void AMoonfallHeroPawn::CycleInventorySort()")
        self.assertIn("bCharacterStatsOpen && bMenuFocusCharacter", sorter)
        for zoom in ("ZoomCameraIn", "ZoomCameraOut"):
            section = body(self.hero, "void AMoonfallHeroPawn::" + zoom + "()")
            self.assertIn("if (bBagPanelOpen)", section)

    def test_b_and_c_are_independent_and_simultaneously_clickable(self):
        header = (ROOT / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        hud = (ROOT / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        self.assertIn("bool bBagPanelOpen = false;", header)
        self.assertIn("bool bMenuFocusCharacter = false;", header)
        bag = body(self.hero, "void AMoonfallHeroPawn::ToggleInventory()")
        char = body(self.hero, "void AMoonfallHeroPawn::ToggleCharacterStats()")
        self.assertIn("bBagPanelOpen = !bBagPanelOpen;", bag)
        self.assertNotIn("bCharacterStatsOpen = false;", bag)
        self.assertIn("bCharacterStatsOpen = !bCharacterStatsOpen;", char)
        self.assertNotIn("bBagPanelOpen = false;", char)
        for method in (bag, char):
            self.assertIn("bInventoryOpen = bBagPanelOpen || bCharacterStatsOpen;", method)
        self.assertIn("MoonfallDualPanels(", hud)
        self.assertIn("MoonfallDualBagGeometry(", hud)
        self.assertIn("MoonfallDualCharacterGeometry(", hud)
        self.assertIn('TEXT("MOONFALL  |  BAG")', hud)
        self.assertIn("if (bDual) MoonfallDualCharacterGeometry(", hud)
        self.assertIn("if (MoonfallDualPanels(Canvas,Hero))", hud)
        self.assertIn("MoonfallBagActionRect(Canvas,true,Action", hud)
        self.assertIn("MoonfallCharacterActionRect(Canvas,MoonfallDualPanels(Canvas,Hero),Action", hud)
        self.assertIn("!Hero->bBagPanelOpen", hud)
        self.assertIn("bMenuFocusCharacter = false;", self.hero)

    def test_character_sheet_allows_deliberate_gear_actions(self):
        # C can equip and upgrade the deliberately selected bag item;
        # B retains sorting and salvage, and gameplay cannot forge.
        for name in (
            "UpgradeInventoryItem",
            "ReforgeInventoryItemAffix",
            "AscendInventoryItemRarity",
            "ForgeInventoryItemLevels",
            "EquipInventoryItem",
        ):
            method = body(self.hero, "AMoonfallHeroPawn::" + name + "(")
            self.assertIn("!bInventoryOpen", method, name)
            self.assertIn("bOptionsOpen", method, name)
            self.assertNotIn("bCharacterStatsOpen || bOptionsOpen", method, name)
        for name in ("CycleInventorySort", "SalvageInventoryItem"):
            method = body(self.hero, "AMoonfallHeroPawn::" + name + "(")
            self.assertIn("!bBagPanelOpen || bOptionsOpen", method, name)
        for name in ("UpgradeSelectedInventoryItem",
                     "ReforgeSelectedInventoryItem",
                     "AscendSelectedInventoryItem",
                     "ForgeFiveSelectedInventoryItem"):
            method = body(self.hero, "void AMoonfallHeroPawn::" + name + "()")
            self.assertNotIn("if (!bCharacterStatsOpen)", method)
        hud = (ROOT / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        header = (ROOT / "MoonfallCombatHUD.h").read_text(encoding="utf-8")
        self.assertIn("GetCharacterActionAtScreenPosition(", header)
        hit = body(hud, "int32 AMoonfallCombatHUD::GetCharacterActionAtScreenPosition(")
        self.assertIn("!Hero->bCharacterStatsOpen", hit)
        self.assertIn("for (int32 Action = 0; Action < 11; ++Action)", hit)
        self.assertIn("MoonfallCharacterActionRect(Canvas,MoonfallDualPanels(Canvas,Hero),Action", hit)
        self.assertIn("return Action;", hit)
        row_hit = body(hud, "int32 AMoonfallCombatHUD::GetInventoryItemAtScreenPosition(")
        self.assertIn("if (!Hero->bBagPanelOpen) return -1;", row_hit)
        for label in ("PREV", "NEXT", "EQUIP", "UPGRADE",
                      "ASCEND", "REFORGE", "FORGE x5", "OPEN BAG"):
            self.assertIn('TEXT("' + label + '")', hud)
        left = body(self.hero, "void AMoonfallHeroPawn::LeftClickContextAction()")
        self.assertIn("HUD->GetCharacterActionAtScreenPosition(this, MouseX, MouseY)", left)
        self.assertIn("if (Action == 7) {", left)
        self.assertIn("if (bBagPanelOpen) { bMenuFocusCharacter = false; MenuActionFocus = -1; }", left)
        for action, method in ((8, "PreviousCharacterStatsPage"),
                               (9, "NextCharacterStatsPage"),
                               (10, "ToggleCharacterStats")):
            self.assertIn(f"if (Action == {action}) {{ {method}(); return; }}", left)
        for label in ("STATS -", "STATS +", "CLOSE"):
            self.assertIn('TEXT("' + label + '")', hud)
        for action, call in ((2,"EquipSelectedInventoryItem"),
                             (3,"UpgradeSelectedInventoryItem"),
                             (4,"AscendSelectedInventoryItem"),
                             (5,"ReforgeSelectedInventoryItem"),
                             (6,"ForgeFiveSelectedInventoryItem")):
            self.assertIn(f"case {action}: {call}(); break;", left)
        self.assertLess(left.index("if (bCharacterStatsOpen) {"),
                        left.index("IsInventorySortButtonAtScreenPosition"))
        right = body(self.hero, "void AMoonfallHeroPawn::RightClickContextAction()")
        self.assertIn("if (!bBagPanelOpen) return;", right)
        self.assertIn("HUD->GetInventoryItemAtScreenPosition", right)


    def test_talent_respec_requires_explicit_yes_no_confirmation(self):
        header = (ROOT / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        inputs = (ROOT.parents[1] / "Config" / "DefaultInput.ini").read_text(encoding="utf-8")
        hud = (ROOT / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        self.assertIn("bool bTalentRespecArmed = false;", header)
        self.assertIn("int32 TalentRespecChoice = 1;", header)  # No by default.
        self.assertNotIn("TalentRespecConfirmUntil", header + self.hero + hud)
        self.assertIn('ActionName="TalentRespec",Key=BackSpace', inputs)
        self.assertIn('ActionName="TalentRespec",Key=Gamepad_FaceButton_Top', inputs)
        self.assertIn('ActionName="CancelTalentDialog",Key=Gamepad_FaceButton_Right', inputs)
        self.assertIn('BindAction(TEXT("CancelTalentDialog"), IE_Pressed', self.hero)
        cancel = body(self.hero, "void AMoonfallHeroPawn::CancelTalentRespecDialog()")
        self.assertIn("if (!bAtlasOpen || !bTalentsOpen || !bTalentRespecArmed) return;", cancel)
        self.assertIn("ResolveTalentRespecConfirmation(false);", cancel)
        esc = body(self.hero, "void AMoonfallHeroPawn::OpenOptions()")
        self.assertIn("CancelTalentRespecDialog();", esc)
        self.assertLess(esc.index("CancelTalentRespecDialog();"),
                        esc.index("bAtlasOpen = false;"))
        self.assertIn('TEXT("D-pad: choose   A: confirm   B/Esc: cancel")', hud)

        action = body(self.hero, "void AMoonfallHeroPawn::RespecTalentAction()")
        self.assertIn("if (bTalentRespecArmed) return;", action)
        self.assertIn("bTalentRespecArmed = true;", action)
        self.assertIn("TalentRespecChoice = 1;", action)
        self.assertNotIn("RespecTalents();", action)
        resolution = body(self.hero, "void AMoonfallHeroPawn::ResolveTalentRespecConfirmation(")
        self.assertIn("if (!bTalentRespecArmed) return;", resolution)
        self.assertIn("if (bConfirm) {", resolution)
        self.assertIn("bTalentRespecCommitAuthorized = true;", resolution)
        self.assertIn("RespecTalents();", resolution)
        self.assertIn("bTalentRespecCommitAuthorized = false;", resolution)
        self.assertIn("bool bTalentRespecCommitAuthorized = false;", header)
        reset = body(self.hero, "bool AMoonfallHeroPawn::RespecTalents()")
        self.assertIn("const bool bAuthorized = bTalentRespecCommitAuthorized;", reset)
        self.assertIn("bTalentRespecCommitAuthorized = false;", reset)
        self.assertIn("if (!bAuthorized) return false;", reset)
        self.assertLess(reset.index("if (!bAuthorized) return false;"),
                        reset.index("TalentForceRank = 0;"))
        self.assertIn("bTalentRespecArmed = false;", resolution)
        chosen = body(self.hero, "void AMoonfallHeroPawn::ActivateSelectedTalentNode()")
        self.assertIn("ResolveTalentRespecConfirmation(TalentRespecChoice == 0);", chosen)
        activation = body(self.hero, "bool AMoonfallHeroPawn::ActivateTalentNode(")
        self.assertIn("if (bTalentRespecArmed) return false;", activation)
        for direction, choice in (("OptionsLeft", 0), ("OptionsUp", 0),
                                  ("OptionsRight", 1), ("OptionsDown", 1)):
            controls = body(self.hero, "void AMoonfallHeroPawn::" + direction + "()")
            self.assertIn("TalentRespecChoice=" + str(choice), controls)
        click = body(self.hero, "void AMoonfallHeroPawn::LeftClickContextAction()")
        self.assertIn("if (bTalentsOpen && Nav == 3) { RespecTalentAction(); return; }", click)
        self.assertIn("if (Nav == 4) ResolveTalentRespecConfirmation(true);", click)
        self.assertIn("else if (Nav == 5) ResolveTalentRespecConfirmation(false);", click)
        self.assertIn("if (bTalentsOpen && bTalentRespecArmed)", click)
        hit = body(hud, "int32 AMoonfallCombatHUD::GetAtlasMenuActionAtScreenPosition(")
        self.assertIn("if (Talents && Hero->bTalentRespecArmed)", hit)
        self.assertIn("return 4; // YES", hit)
        self.assertIn("return 5; // NO", hit)
        self.assertIn("return -1; // Modal dialog blocks", hit)
        self.assertIn("DrawRespecConfirmation(Left,Top,Width,Height);", hud)
        for label in ("ARE YOU SURE?", "YES", "NO"):
            self.assertIn('TEXT("' + label + '")', hud)
        self.assertIn("Hero->TalentRespecChoice==Choice", hud)
        for name in ("SelectNextTalentPage", "SelectPreviousTalentPage",
                     "SelectNextTalentNode", "SelectPreviousTalentNode"):
            method = body(self.hero, "AMoonfallHeroPawn::" + name + "(")
            self.assertIn("if (bTalentRespecArmed) return;", method)
        for name in ("ToggleTalents", "ToggleAtlas", "ToggleInventory",
                     "ToggleCharacterStats", "OpenOptions"):
            method = body(self.hero, "AMoonfallHeroPawn::" + name + "(")
            self.assertIn("bTalentRespecArmed = false;", method)


    def test_mouse_and_controller_focus_share_the_same_button(self):
        click = body(self.hero, "void AMoonfallHeroPawn::LeftClickContextAction()")
        # Mouse actions update controller focus, so switching input methods
        # cannot silently activate a different, stale menu action.
        for name in ("Action", "BagAction"):
            statement = "MenuActionFocus=" + name + ";"
            self.assertIn(statement, click)
        self.assertLess(click.index("MenuActionFocus=BagAction;"),
                        click.index("if (BagAction == 6)"))
        self.assertLess(click.index("MenuActionFocus=Action;", click.index("if (bCharacterStatsOpen)")),
                        click.index("if (Action == 7)"))
        self.assertIn("MenuActionFocus=Action;\\n       OptionsSelection=Action%7;".replace("\\n","\n"),click)

    def test_respec_guards_preserve_valid_cpp_statements(self):
        # Source-only CI cannot compile Unreal. Catch the exact regression
        # where insertion split a comment and left prose inside C++.
        self.assertNotIn("bTalentRespecArmed = false; controller A", self.hero)
        self.assertNotIn("bTalentRespecArmed = false; do not reopen", self.hero)
        for name in ("ToggleCharacterStats", "OpenOptions"):
            method = body(self.hero, "void AMoonfallHeroPawn::" + name + "()")
            self.assertIn("bTalentRespecArmed = false;", method)
            self.assertIn("ResetMouseCastChord();", method)
        self.assertIn("// The defeat screen owns input until respawn; do not reopen gameplay menus.", self.hero)

    def test_cursor_mode_updates_after_menu_state_transitions(self):
        inventory = body(self.hero, "void AMoonfallHeroPawn::ToggleInventory()")
        character = body(self.hero, "void AMoonfallHeroPawn::ToggleCharacterStats()")
        options = body(self.hero, "void AMoonfallHeroPawn::OpenOptions()")
        for toggle in (inventory, character):
            self.assertIn("bInventoryOpen = bBagPanelOpen || bCharacterStatsOpen;", toggle)
            self.assertIn("MenuActionFocus = -1;", toggle)
            self.assertLess(toggle.index("bInventoryOpen = bBagPanelOpen || bCharacterStatsOpen;"),
                            toggle.index("UpdateInventoryMouseMode();"))
        self.assertIn("bBagPanelOpen = !bBagPanelOpen;", inventory)
        self.assertIn("bCharacterStatsOpen = !bCharacterStatsOpen;", character)
        for flag in ("bAtlasOpen = false;", "bInventoryOpen = false;"):
            branch = options.split(flag, 1)[1].split("return;", 1)[0]
            self.assertIn("MenuActionFocus = -1;", branch)
            self.assertIn("UpdateInventoryMouseMode();", branch)


    def test_controller_focus_and_confirm_for_main_menus(self):
        inputs = (ROOT.parents[1] / "Config" / "DefaultInput.ini").read_text(encoding="utf-8")
        header = (ROOT / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        hud = (ROOT / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        self.assertIn("int32 MenuActionFocus = -1;", header)
        self.assertIn("void ActivateFocusedMenuAction();", header)
        self.assertIn('ActionName="InventoryPage",Key=Gamepad_FaceButton_Bottom', inputs)
        self.assertNotIn('ActionName="CharacterStatsNext",Key=Gamepad_DPad_Right', inputs)
        self.assertNotIn('ActionName="CharacterStatsPrevious",Key=Gamepad_DPad_Left', inputs)
        confirm = body(self.hero, "void AMoonfallHeroPawn::ActivateFocusedMenuAction()")
        for action in ("AdjustOption(", "ToggleAtlas()", "ToggleInventory()",
                       "ToggleCharacterStats()", "NextCharacterStatsPage()",
                       "PreviousCharacterStatsPage()", "EquipSelectedInventoryItem()",
                       "UpgradeSelectedInventoryItem()", "SalvageSelectedInventoryItem()"):
            self.assertIn(action, confirm)
        self.assertIn("if (!bTalentsOpen && MenuActionFocus == 0) ToggleAtlas();", confirm)
        trigger = body(self.hero, "void AMoonfallHeroPawn::SwitchInventoryPage()")
        self.assertIn("ActivateFocusedMenuAction();", trigger)
        for name in ("OptionsUp", "OptionsDown"):
            step = body(self.hero, "void AMoonfallHeroPawn::" + name + "()")
            self.assertIn("MenuActionFocus", step)
            self.assertIn("bInventoryOpen", step)
            self.assertIn("bOptionsOpen", step)
        for name in ("OptionsLeft", "OptionsRight"):
            step = body(self.hero, "void AMoonfallHeroPawn::" + name + "()")
            self.assertIn("InventorySelection", step)
        self.assertIn("Hero->MenuActionFocus==Action", hud)
        self.assertIn("Hero->MenuActionFocus==i+7", hud)
        self.assertIn("Hero->MenuActionFocus==14", hud)
        self.assertIn("Hero->MenuActionFocus == 0 ? FLinearColor(0.72f,0.53f,0.20f)", hud)
        for name in ("OptionsUp", "OptionsDown"):
            controls = body(self.hero, "void AMoonfallHeroPawn::" + name + "()")
            self.assertIn("bAtlasOpen && !bTalentsOpen", controls)
            self.assertIn("MenuActionFocus=0; return;", controls)
        for name in ("OptionsLeft", "OptionsRight"):
            controls = body(self.hero, "void AMoonfallHeroPawn::" + name + "()")
            self.assertIn("if (MenuActionFocus < 0)", controls)
            self.assertLess(controls.index("if (MenuActionFocus < 0)"),
                            controls.index("AdjustOption("))
        # Opening a menu must not leave a destructive action selected.
        self.assertIn("if (MenuActionFocus < 0) return;", confirm)
        self.assertEqual(confirm.count("if (MenuActionFocus < 0) return;"), 2)
        for name in ("OptionsUp", "OptionsDown"):
            navigation = body(self.hero, "void AMoonfallHeroPawn::" + name + "()")
            self.assertIn("MenuActionFocus", navigation)
        self.assertIn("MenuActionFocus = -1;", self.hero)
        jump = body(self.hero, "void AMoonfallHeroPawn::TryJump()")
        self.assertIn("bOptionsOpen || bInventoryOpen || bAtlasOpen", jump)

    def test_clickable_backpack_action_bar(self):
        hud = (ROOT / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        header = (ROOT / "MoonfallCombatHUD.h").read_text(encoding="utf-8")
        self.assertIn("GetInventoryActionAtScreenPosition(", header)
        hit = body(hud, "int32 AMoonfallCombatHUD::GetInventoryActionAtScreenPosition(")
        self.assertIn("Hero->bBagPanelOpen", hit)
        self.assertIn("MoonfallBagActionRect(Canvas,MoonfallDualPanels(Canvas,Hero),Action", hit)
        self.assertIn("return Action;", hit)
        for title in ("EQUIP", "UPGRADE", "ASCEND", "REFORGE",
                      "FORGE x5", "SALVAGE", "CHARACTER", "CLOSE"):
            self.assertIn('TEXT("' + title + '")', hud)
        click = body(self.hero, "void AMoonfallHeroPawn::LeftClickContextAction()")
        self.assertIn("HUD->GetInventoryActionAtScreenPosition(this,MouseX,MouseY)", click)
        self.assertIn("if (BagAction == 6) {", click)
        self.assertIn("if (bCharacterStatsOpen) { bMenuFocusCharacter = true; MenuActionFocus = -1; }", click)
        self.assertIn("if (BagAction == 7) { ToggleInventory(); return; }", click)
        for action, method in ((0,"EquipSelectedInventoryItem"),
                               (1,"UpgradeSelectedInventoryItem"),
                               (2,"AscendSelectedInventoryItem"),
                               (3,"ReforgeSelectedInventoryItem"),
                               (4,"ForgeFiveSelectedInventoryItem"),
                               (5,"SalvageSelectedInventoryItem")):
            self.assertIn(f"case {action}: {method}(); break;", click)
        self.assertLess(click.index("GetInventoryActionAtScreenPosition"),
                        click.index("IsInventorySortButtonAtScreenPosition"))

    def test_clickable_settings_and_atlas_navigation(self):
        hud = (ROOT / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        header = (ROOT / "MoonfallCombatHUD.h").read_text(encoding="utf-8")
        self.assertIn("GetOptionsActionAtScreenPosition(", header)
        self.assertIn("GetAtlasMenuActionAtScreenPosition(", header)
        settings = body(hud, "int32 AMoonfallCombatHUD::GetOptionsActionAtScreenPosition(")
        self.assertIn("return 14;", settings)
        self.assertIn("return Row;", settings)
        self.assertIn("return 7+Row;", settings)
        atlas = body(hud, "int32 AMoonfallCombatHUD::GetAtlasMenuActionAtScreenPosition(")
        self.assertIn("return 0;", atlas)
        self.assertIn("return 1;", atlas)
        self.assertIn("return 2;", atlas)
        self.assertIn("return 3;", atlas)
        click = body(self.hero, "void AMoonfallHeroPawn::LeftClickContextAction()")
        for call in ("GetOptionsActionAtScreenPosition", "GetAtlasMenuActionAtScreenPosition",
                     "AdjustOption(Action<7 ? -1 : 1)", "SelectPreviousTalentPage",
                     "SelectNextTalentPage", "ToggleAtlas()", "ToggleTalents()"):
            self.assertIn(call, click)
        self.assertLess(click.index("if (bOptionsOpen || bAtlasOpen)"),
                        click.index("if (bAtlasOpen && bTalentsOpen)"))
        mouse = body(self.hero, "void AMoonfallHeroPawn::UpdateInventoryMouseMode()")
        self.assertIn("bInventoryOpen || bAtlasOpen || bOptionsOpen", mouse)
        self.assertIn("bOptionsOpen = !bOptionsOpen;", self.hero)
        self.assertIn("MenuActionFocus = -1;\\n UpdateInventoryMouseMode();".replace("\\n","\n"), self.hero)

    def test_sort_mode_survives_version_two_save_round_trip(self):
        source = self.hero
        save_header = (ROOT / "MoonfallProgressSaveGame.h").read_text(encoding="utf-8")
        self.assertIn("UPROPERTY(SaveGame) int32 InventorySortMode = 0;", save_header)
        save = body(source, "bool AMoonfallHeroPawn::SaveProgress()")
        load = body(source, "bool AMoonfallHeroPawn::LoadProgress()")
        self.assertIn("Save->InventorySortMode = InventorySortMode;", save)
        self.assertIn("InventorySortMode = Save->InventorySortMode;", load)
        # Invalid values cannot be written or loaded, and loading only
        # applies the mode after the snapshot has passed validation.
        guard = "(Save->InventorySortMode < 0 || Save->InventorySortMode > 2)"
        self.assertIn(guard, save)
        self.assertIn(guard, load)
        # A fresh character uses pickup order (0); this is valid and
        # must not block the 16-second autosave or a clean reload.
        hero_header = (ROOT / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        self.assertIn("int32 InventorySortMode = 0;", hero_header)
        self.assertLess(load.index(guard), load.index("InventorySortMode = Save->InventorySortMode;"))
        self.assertLess(load.index("if (Save->Version == 2) {"), load.index("InventorySortMode = Save->InventorySortMode;"))

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

    def test_sort_button_blocks_underlying_gear_row_hit_test(self):
        hud = (ROOT / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        hit = body(hud, "int32 AMoonfallCombatHUD::GetInventoryItemAtScreenPosition(")
        sort_guard = "if (IsInventorySortButtonAtScreenPosition(Hero, X, Y)) return -1;"
        self.assertIn(sort_guard, hit)
        self.assertLess(hit.index(sort_guard), hit.index("const int32 Count"))
        self.assertLess(hit.index(sort_guard), hit.index("for (int32 Index = PageStart"))
        right = body(self.hero, "void AMoonfallHeroPawn::RightClickContextAction()")
        self.assertIn("HUD->GetInventoryItemAtScreenPosition(this, MouseX, MouseY)", right)
        self.assertNotIn("IsInventoryEquipButtonAtScreenPosition", right)

    def test_mouse_wheel_browses_bag_without_changing_camera(self):
        hud = (ROOT / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        inputs = (ROOT.parents[1] / "Config" / "DefaultInput.ini").read_text(encoding="utf-8")
        self.assertIn('ActionName="ZoomIn",Key=MouseScrollUp', inputs)
        self.assertIn('ActionName="ZoomOut",Key=MouseScrollDown', inputs)
        for name, change in (("ZoomCameraIn", "InventoryViewStart - 1"),
                             ("ZoomCameraOut", "InventoryViewStart + 1")):
            action = body(self.hero, "void AMoonfallHeroPawn::" + name + "()")
            self.assertIn("if (bBagPanelOpen)", action)
            self.assertIn(change, action)
            self.assertIn("GetInventoryVisibleRowCount()", action)
            self.assertIn("InventoryItemCodes.Num() - VisibleRows", action)
            self.assertNotIn("InventorySelection =", action)
            self.assertIn("return;", action)
            self.assertLess(action.index("if (bBagPanelOpen)"),
                            action.index("CameraDistance ="))
            self.assertNotIn("EquipInventoryItem(", action)
            self.assertNotIn("SalvageInventoryItem(", action)
            self.assertNotIn("InventoryItemCodes.RemoveAt(", action)
        self.assertIn("Wheel: scroll bag", hud)
        self.assertIn("InventoryViewStart", (ROOT / "MoonfallHeroPawn.h").read_text(encoding="utf-8"))
        self.assertIn("Hero->InventoryViewStart", hud)
        self.assertIn("showing %d-%d", hud)
        self.assertIn("const int32 PageStart = FMath::Clamp(", hud)
        self.assertNotIn("PageStart = Selected / PerPage * PerPage", hud)
        self.assertNotIn('TEXT("EQUIP SELECTED")', hud)
        self.assertNotIn('TEXT("PREV PAGE")', hud)
        self.assertNotIn('TEXT("NEXT PAGE")', hud)

    def test_one_big_bag_uses_a_single_continuous_scroll_viewport(self):
        hud = (ROOT / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        self.assertIn("One continuous scrollable bag", hud)
        self.assertGreaterEqual(hud.count("860.f, FMath::Max(280.f, Canvas->SizeY - 24.f)"), 4)
        self.assertGreaterEqual(hud.count("FMath::FloorToInt((PanelHeight - 355.f) / 25.f), 2, 20"), 3)
        self.assertEqual(hud.count("FMath::Max(21.f, (PanelHeight - 355.f) / PerPage)"), 2)
        self.assertGreaterEqual(hud.count("Hero->InventoryViewStart"), 2)
        self.assertIn("showing %d-%d", hud)
        self.assertIn("Count - PerPage", hud)
        self.assertNotIn("Canvas-only paginated inventory", hud)
        self.assertNotIn('TEXT("PREV PAGE")', hud)
        self.assertNotIn('TEXT("NEXT PAGE")', hud)

    def test_big_bag_scrollbar_and_salvage_scroll_clamp(self):
        hud = (ROOT / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        header = (ROOT / "MoonfallCombatHUD.h").read_text(encoding="utf-8")
        self.assertIn("GetInventoryVisibleRowCount() const", header)
        self.assertIn("int32 AMoonfallCombatHUD::GetInventoryVisibleRowCount() const", hud)
        self.assertIn("TrackHeight * PerPage / Count", hud)
        self.assertIn("PageStart) / FMath::Max(1, Count - PerPage)", hud)
        self.assertIn('TEXT("Selected item %d is %s")', hud)
        self.assertIn("Selected < PageStart || Selected >= PageEnd", hud)
        salvage = body(self.hero, "bool AMoonfallHeroPawn::SalvageInventoryItem(")
        self.assertLess(salvage.index("InventoryItemCodes.RemoveAt(InventoryIndex)"),
                        salvage.index("InventoryViewStart = FMath::Clamp("))
        self.assertNotIn('TEXT("PREV PAGE")', hud)
        self.assertNotIn('TEXT("NEXT PAGE")', hud)
        self.assertNotIn('TEXT("EQUIP SELECTED")', hud)

    def test_large_bag_advisor_is_throttled_and_invalidated_safely(self):
        hud = (ROOT / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        header = (ROOT / "MoonfallCombatHUD.h").read_text(encoding="utf-8")
        self.assertIn("CachedAdvisorIndex", header)
        self.assertIn("CachedAdvisorHero", header)
        self.assertIn("NextAdvisorRefreshAt", header)
        self.assertIn("AdvisorNow + 0.25f", hud)
        self.assertIn("CachedAdvisorCount != Count", hud)
        self.assertIn("CachedAdvisorWeapon != Hero->EquippedWeaponCode", hud)
        self.assertIn("CachedAdvisorArmor != Hero->EquippedArmorCode", hud)
        self.assertIn("CachedAdvisorAccessory != Hero->EquippedAccessoryCode", hud)
        self.assertIn("CachedAdvisorSortMode != Hero->InventorySortMode", hud)
        self.assertIn("bAdvisorItemChanged", hud)
        self.assertIn("CachedAdvisorHero = nullptr;", hud)
        self.assertIn("Hero->GetRecommendedEquipmentIndex()", hud)
        self.assertIn("const int32 AdvisedIndex = CachedAdvisorIndex;", hud)
        self.assertNotIn('TEXT("EQUIP SELECTED")', hud)

    def test_m_map_and_k_talents_are_separate_without_fake_allocations(self):
        config = (ROOT.parents[1] / "Config" / "DefaultInput.ini").read_text(encoding="utf-8")
        hero_h = (ROOT / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        hud = (ROOT / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        self.assertIn('ActionName="Atlas",Key=M', config)
        self.assertIn('ActionName="Talents",Key=K', config)
        self.assertNotIn('ActionName="Atlas",Key=J', config)
        self.assertIn('BindAction(TEXT("Talents"), IE_Pressed', self.hero)
        self.assertIn("void AMoonfallHeroPawn::ToggleTalents()", self.hero)
        self.assertIn("bTalentsOpen = false;", hero_h)
        self.assertIn('TEXT("MOONFALL  |  MAP")', hud)
        self.assertIn('CELESTIAL TALENTS', hud)
        self.assertIn('UNSPENT TALENT POINTS: %d', hud)
        self.assertIn('TEXT("1  FORCE")', hud)
        self.assertIn('TEXT("2  FOCUS")', hud)
        self.assertIn('TEXT("3  WARD")', hud)
        self.assertIn("GetUnspentTalentPoints()", hud)
        self.assertIn("if (Hero->bAtlasOpen && Hero->bTalentsOpen)", hud)
        self.assertIn("bTalentsOpen = false;", self.hero)
        self.assertNotIn('TEXT("EQUIP SELECTED")', hud)

    def test_shared_talents_spend_points_only_in_k_panel_and_affect_combat(self):
        inputs = (ROOT.parents[1] / "Config" / "DefaultInput.ini").read_text(encoding="utf-8")
        header = (ROOT / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        for action, key, method in (("TalentForce", "One", "SpendForceTalent"),
                                    ("TalentFocus", "Two", "SpendFocusTalent"),
                                    ("TalentWard", "Three", "SpendWardTalent")):
            self.assertIn(f'ActionName="{action}",Key={key}', inputs)
            self.assertIn(f'BindAction(TEXT("{action}"), IE_Pressed', self.hero)
            self.assertIn(f"void AMoonfallHeroPawn::{method}()", self.hero)
        for rank in ("TalentForceRank", "TalentFocusRank", "TalentWardRank"):
            self.assertIn(rank + " = 0;", header)
        spend = body(self.hero, "bool AMoonfallHeroPawn::SpendTalentPoint(int32 Branch)")
        self.assertIn("!bAtlasOpen || !bTalentsOpen", spend)
        self.assertIn("GetUnspentTalentPoints() <= 0", spend)
        self.assertIn("*Rank >= 10", spend)
        self.assertIn("++(*Rank);", spend)
        points = body(self.hero, "int32 AMoonfallHeroPawn::GetUnspentTalentPoints() const")
        self.assertIn("FMath::Clamp(Level, 1, 100) - 1", points)
        self.assertIn("TalentForceRank - TalentFocusRank - TalentWardRank", points)
        self.assertIn("FMath::Clamp(TalentForceRank, 0, 10) * 0.005f", self.hero)
        self.assertIn("FMath::Clamp(TalentFocusRank, 0, 10) * 0.01f", self.hero)
        self.assertIn("FMath::Clamp(TalentWardRank, 0, 10) * 0.008f", self.hero)
        self.assertNotIn("ZTalentRank", header)
        self.assertNotIn("NyraTalentRank", header)

    def test_constellation_keystones_links_and_respec_are_gated(self):
        inputs = (ROOT.parents[1] / "Config" / "DefaultInput.ini").read_text(encoding="utf-8")
        header = (ROOT / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        hud = (ROOT / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        for name, key in (("TalentForceKeystone", "Four"),
                          ("TalentFocusKeystone", "Five"),
                          ("TalentWardKeystone", "Six"),
                          ("TalentBattleWeave", "Seven"),
                          ("TalentArcaneAegis", "Eight"),
                          ("TalentIronWill", "Nine"),
                          ("TalentRespec", "BackSpace")):
            self.assertIn(f'ActionName="{name}",Key={key}', inputs)
            self.assertIn(f'BindAction(TEXT("{name}"), IE_Pressed', self.hero)
        for field in ("bTalentForceKeystone", "bTalentFocusKeystone",
                      "bTalentWardKeystone", "bTalentBattleWeave",
                      "bTalentArcaneAegis", "bTalentIronWill"):
            self.assertIn(field + " = false;", header)
        gate = body(self.hero, "bool AMoonfallHeroPawn::CanUnlockTalentNode(int32 Node) const")
        self.assertIn("GetUnspentTalentPoints() <= 0", gate)
        for check in ("TalentForceRank >= 3", "TalentFocusRank >= 3",
                      "TalentWardRank >= 3",
                      "bTalentForceKeystone && bTalentFocusKeystone",
                      "bTalentFocusKeystone && bTalentWardKeystone",
                      "bTalentWardKeystone && bTalentForceKeystone"):
            self.assertIn(check, gate)
        unlock = body(self.hero, "bool AMoonfallHeroPawn::UnlockTalentNode(int32 Node)")
        self.assertIn("if (!CanUnlockTalentNode(Node)) return false;", unlock)
        self.assertIn("*Flag = true;", unlock)
        points = body(self.hero, "int32 AMoonfallHeroPawn::GetUnspentTalentPoints() const")
        self.assertIn("int32(bTalentForceKeystone)", points)
        self.assertIn("int32(bTalentIronWill)", points)
        respec = body(self.hero, "bool AMoonfallHeroPawn::RespecTalents()")
        self.assertIn("!bAtlasOpen || !bTalentsOpen", respec)
        self.assertIn("TalentForceRank = 0;", respec)
        self.assertIn("bTalentIronWill = false;", respec)
        self.assertIn('TEXT("7  BATTLE WEAVE")', hud)
        self.assertIn('TEXT("8  ARCANE AEGIS")', hud)
        self.assertIn('TEXT("9  IRON WILL")', hud)
        self.assertIn("DrawLine(CenterX[", hud)
        self.assertIn("Requires 2 keystones", hud)
        self.assertIn("Backspace: respec", hud)
        self.assertNotIn("ZTalentRank", header)
        self.assertNotIn("NyraTalentRank", header)

    def test_constellation_visuals_use_glowing_stars_not_boxed_node_cards(self):
        hud = (ROOT / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        panel = hud.split("if (Hero->bAtlasOpen && Hero->bTalentsOpen) {", 1)[1].split(
            "if (Hero->bAtlasOpen) {", 1)[0]
        self.assertIn("CELESTIAL TALENTS", panel)
        self.assertIn("const float Pulse =", panel)
        self.assertIn("for (int32 Star = 0; Star < 85; ++Star)", panel)
        self.assertIn("for (int32 Seg = 0; Seg < 24; ++Seg)", panel)
        self.assertIn("R * FMath::Cos(A)", panel)
        self.assertIn("R * FMath::Sin(A)", panel)
        self.assertIn("const int32 LinkA[] = {0,1,2};", panel)
        self.assertIn("const int32 LinkB[] = {1,2,0};", panel)
        self.assertIn("Hero->CanUnlockTalentNode(", panel)
        self.assertIn("DrawLine(CenterX[", panel)
        self.assertIn("Backspace: respec", panel)
        self.assertNotIn("NodeWidth", panel)
        self.assertNotIn("RowY + Branch * 49.f", panel)
        self.assertNotIn("SpendTalentPoint(", panel)

    def test_constellation_focus_zoom_and_pointer_actions_are_isolated(self):
        hud = (ROOT / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        hud_header = (ROOT / "MoonfallCombatHUD.h").read_text(encoding="utf-8")
        header = (ROOT / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        inputs = (ROOT.parents[1] / "Config" / "DefaultInput.ini").read_text(encoding="utf-8")
        for name, key in (("TalentNext", "Tab"), ("TalentPrevious", "Gamepad_DPad_Left"),
                          ("TalentActivate", "Enter"), ("TalentActivate", "Gamepad_FaceButton_Bottom")):
            self.assertIn(f'ActionName="{name}",Key={key}', inputs)
            self.assertIn(f'BindAction(TEXT("{name}"), IE_Pressed', self.hero)
        self.assertIn("TalentZoom = 1.f", header)
        self.assertIn("TalentSelectedNode = 0", header)
        self.assertIn("GetTalentNodeAtScreenPosition(", hud_header)
        self.assertIn("GetTalentNodeAtScreenPosition(", hud)
        hit = body(hud, "int32 AMoonfallCombatHUD::GetTalentNodeAtScreenPosition(")
        self.assertIn("DistanceSq <= Radius * Radius && DistanceSq < BestDistanceSq", hit)
        self.assertIn("ClosestNode = Tier * 3 + Branch;", hit)
        left = body(self.hero, "void AMoonfallHeroPawn::LeftClickContextAction()")
        self.assertIn("if (bAtlasOpen && bTalentsOpen)", left)
        self.assertIn("HUD->GetTalentNodeAtScreenPosition(this,X,Y)", left)
        self.assertIn("ActivateTalentNode(Node);", left)
        self.assertIn("if (bAtlasOpen) return;", left)
        activation = body(self.hero, "bool AMoonfallHeroPawn::ActivateTalentNode(int32 Node)")
        self.assertIn("Node < 0 || Node >= 138", activation)
        self.assertIn("SpendTalentPoint(Node) : Node < 27 ? UnlockTalentNode(Node)", activation)
        self.assertIn("UnlockAstralTalent(Node - 27)", activation)
        zoomin = body(self.hero, "void AMoonfallHeroPawn::ZoomCameraIn()")
        zoomout = body(self.hero, "void AMoonfallHeroPawn::ZoomCameraOut()")
        self.assertIn("TalentZoom = FMath::Clamp", zoomin)
        self.assertIn("TalentZoom = FMath::Clamp", zoomout)
        self.assertIn("CameraDistance = FMath::Clamp", zoomin)
        self.assertIn("CameraDistance = FMath::Clamp", zoomout)
        mouse = body(self.hero, "void AMoonfallHeroPawn::UpdateInventoryMouseMode()")
        self.assertIn("bInventoryOpen || bAtlasOpen || bOptionsOpen", mouse)
        panel = hud.split("if (Hero->bAtlasOpen && Hero->bTalentsOpen) {", 1)[1].split(
            "if (Hero->bAtlasOpen) {", 1)[0]
        self.assertIn("TalentSelectedNode == Tier * 3 + Branch", panel)
        self.assertIn("Hero->bReduceMotion ? 1.f", panel)
        self.assertIn("Wheel: zoom", panel)
        self.assertIn("const float Zoom =", panel)

    def test_talent_keystones_provide_distinct_live_combat_perks(self):
        # Unlock flags must affect combat calculations, not merely change HUD color.
        cast = body(self.hero, "bool AMoonfallHeroPawn::TryCast(int32 School, int32 Hand, float Multiplier, bool bOvercharged)")
        self.assertIn("(bTalentFocusKeystone ? 0.90f : 1.f)", cast)
        self.assertIn("(bTalentForceKeystone ? 0.20f : 0.f)", cast)
        self.assertIn("(bTalentBattleWeave ? 0.15f : 0.f)", cast)
        self.assertIn("Mana < Costs[School] * ManaCostMultiplier", cast)
        self.assertIn("Mana -= Costs[School] * ManaCostMultiplier;", cast)
        hurt = body(self.hero, "void AMoonfallHeroPawn::ApplyMoonfallDamage(float Amount)")
        self.assertIn("DamageAfterGuard *= 0.4f;", hurt)
        self.assertIn("if (bTalentWardKeystone) DamageAfterGuard *= 0.8f;", hurt)
        tick = body(self.hero, "void AMoonfallHeroPawn::Tick(float DeltaSeconds)")
        self.assertIn("(bTalentArcaneAegis ? 1.15f : 1.f)", tick)
        self.assertIn("(bTalentIronWill ? 1.20f : 1.f)", tick)
        self.assertIn("FMath::Min(MaxMana, Mana +", tick)
        self.assertIn("FMath::Clamp(Stamina +", tick)
        self.assertIn("bTalentForceKeystone = false;", self.hero)
        self.assertIn("bTalentArcaneAegis = false;", self.hero)
        self.assertIn("bTalentIronWill = false;", self.hero)

    def test_fourth_tier_apex_talents_are_spendable_and_reversible(self):
        hud = (ROOT / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        header = (ROOT / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        for field in ("bTalentStarbreaker", "bTalentAstralSurge", "bTalentEternalSentinel"):
            self.assertIn(field + " = false;", header)
            self.assertIn("Hero->" + field, hud)
        gate = body(self.hero, "bool AMoonfallHeroPawn::CanUnlockTalentNode(int32 Node) const")
        for term in ("Node == 9", "Node == 10", "Node == 11",
                     "bTalentBattleWeave && bTalentIronWill",
                     "bTalentBattleWeave && bTalentArcaneAegis",
                     "bTalentArcaneAegis && bTalentIronWill"):
            self.assertIn(term, gate)
        act = body(self.hero, "bool AMoonfallHeroPawn::ActivateTalentNode(int32 Node)")
        self.assertIn("Node >= 138", act)
        self.assertIn("UnlockTalentNode(Node)", act)
        self.assertIn("% 138", body(self.hero, "void AMoonfallHeroPawn::SelectNextTalentNode()"))
        self.assertIn("% 138", body(self.hero, "void AMoonfallHeroPawn::SelectPreviousTalentNode()"))
        unlock = body(self.hero, "bool AMoonfallHeroPawn::UnlockTalentNode(int32 Node)")
        self.assertIn("Node == 11 ? &bTalentEternalSentinel", unlock)
        points = body(self.hero, "int32 AMoonfallHeroPawn::GetUnspentTalentPoints() const")
        self.assertIn("int32(bTalentStarbreaker)", points)
        self.assertIn("int32(bTalentEternalSentinel)", points)
        respec = body(self.hero, "bool AMoonfallHeroPawn::RespecTalents()")
        for field in ("bTalentStarbreaker", "bTalentAstralSurge", "bTalentEternalSentinel"):
            self.assertIn(field + " = false;", respec)
        cast = body(self.hero, "bool AMoonfallHeroPawn::TryCast(int32 School, int32 Hand, float Multiplier, bool bOvercharged)")
        self.assertIn("(bTalentStarbreaker ? 0.25f : 0.f)", cast)
        self.assertIn("(bTalentAstralSurge ? 1.10f : 1.f)", cast)
        hurt = body(self.hero, "void AMoonfallHeroPawn::ApplyMoonfallDamage(float Amount)")
        self.assertIn("if (bTalentEternalSentinel) DamageAfterGuard *= 0.85f;", hurt)
        self.assertIn("const TCHAR* ApexNames[]", hud)
        self.assertIn("for (int32 Tier = 0; Tier < 9; ++Tier)", hud)
        self.assertIn("const int32 ApexA[] = {0,0,1};", hud)
        self.assertIn("const int32 ApexB[] = {2,1,2};", hud)
        self.assertIn("Selected star %d / 27", hud)

    def test_fifth_tier_crowns_have_gated_effects_and_zoomed_mouse_targets(self):
        hud = (ROOT / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        header = (ROOT / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        for flag in ("bTalentVoidCrown", "bTalentCelestialWell", "bTalentAdamantOath"):
            self.assertIn(flag + " = false;", header)
            self.assertIn("Hero->" + flag, hud)
            self.assertIn(flag + " = false;", body(self.hero, "bool AMoonfallHeroPawn::RespecTalents()"))
        gate = body(self.hero, "bool AMoonfallHeroPawn::CanUnlockTalentNode(int32 Node) const")
        for fragment in ("Node == 12", "Node == 13", "Node == 14",
                         "bTalentStarbreaker && bTalentAstralSurge",
                         "bTalentAstralSurge && bTalentEternalSentinel",
                         "bTalentEternalSentinel && bTalentStarbreaker"):
            self.assertIn(fragment, gate)
        unlock = body(self.hero, "bool AMoonfallHeroPawn::UnlockTalentNode(int32 Node)")
        self.assertIn("Node == 14 ? &bTalentAdamantOath", unlock)
        points = body(self.hero, "int32 AMoonfallHeroPawn::GetUnspentTalentPoints() const")
        self.assertIn("int32(bTalentVoidCrown)", points)
        self.assertIn("int32(bTalentAdamantOath)", points)
        cast = body(self.hero, "bool AMoonfallHeroPawn::TryCast(int32 School, int32 Hand, float Multiplier, bool bOvercharged)")
        self.assertIn("(bTalentVoidCrown ? 0.20f : 0.f)", cast)
        self.assertIn("(bTalentCelestialWell ? 0.90f : 1.f)", cast)
        hurt = body(self.hero, "void AMoonfallHeroPawn::ApplyMoonfallDamage(float Amount)")
        self.assertIn("if (bTalentAdamantOath) DamageAfterGuard *= 0.90f;", hurt)
        self.assertIn("const TCHAR* CrownNames[]", hud)
        self.assertIn("const int32 CrownA[] = {0,1,2};", hud)
        self.assertIn("const int32 CrownB[] = {1,2,0};", hud)
        hit = body(hud, "int32 AMoonfallCombatHUD::GetTalentNodeAtScreenPosition(")
        self.assertIn("for (int32 Tier = 0; Tier < 9; ++Tier)", hit)
        self.assertIn("ClosestNode = Tier * 3 + Branch;", hit)
        self.assertIn("Selected star %d / 27", hud)
        self.assertIn("% 138", body(self.hero, "void AMoonfallHeroPawn::SelectNextTalentNode()"))
        self.assertIn("% 138", body(self.hero, "void AMoonfallHeroPawn::SelectPreviousTalentNode()"))

    def test_sixth_tier_mythics_gate_spending_refunds_and_combat(self):
        hud = (ROOT / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        header = (ROOT / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        flags = ("bTalentEclipseHeart", "bTalentInfiniteCurrent", "bTalentWorldguard")
        for flag in flags:
            self.assertIn(flag + " = false;", header)
            self.assertIn("Hero->" + flag, hud)
            self.assertIn(flag + " = false;", body(self.hero, "bool AMoonfallHeroPawn::RespecTalents()"))
        gate = body(self.hero, "bool AMoonfallHeroPawn::CanUnlockTalentNode(int32 Node) const")
        for part in ("Node == 15", "Node == 16", "Node == 17",
                     "bTalentVoidCrown && bTalentAdamantOath",
                     "bTalentVoidCrown && bTalentCelestialWell",
                     "bTalentCelestialWell && bTalentAdamantOath"):
            self.assertIn(part, gate)
        unlock = body(self.hero, "bool AMoonfallHeroPawn::UnlockTalentNode(int32 Node)")
        self.assertIn("Node == 17 ? &bTalentWorldguard", unlock)
        points = body(self.hero, "int32 AMoonfallHeroPawn::GetUnspentTalentPoints() const")
        self.assertIn("int32(bTalentEclipseHeart)", points)
        self.assertIn("int32(bTalentWorldguard)", points)
        cast = body(self.hero, "bool AMoonfallHeroPawn::TryCast(int32 School, int32 Hand, float Multiplier, bool bOvercharged)")
        self.assertIn("(bTalentEclipseHeart ? 0.25f : 0.f)", cast)
        self.assertIn("(bTalentInfiniteCurrent ? 0.90f : 1.f)", cast)
        hurt = body(self.hero, "void AMoonfallHeroPawn::ApplyMoonfallDamage(float Amount)")
        self.assertIn("if (bTalentWorldguard) DamageAfterGuard *= 0.90f;", hurt)
        self.assertIn("const TCHAR* MythicNames[]", hud)
        self.assertIn("const int32 MythicA[] = {0,0,1};", hud)
        self.assertIn("const int32 MythicB[] = {2,1,2};", hud)
        self.assertIn("for (int32 Tier = 0; Tier < 9; ++Tier)", hud)
        self.assertIn("Selected star %d / 27", hud)
        self.assertIn("(FMath::Clamp(TalentSelectedNode,0,137) + 1) % 138", self.hero)
        self.assertIn("(FMath::Clamp(TalentSelectedNode,0,137) + 137) % 138", self.hero)

    def test_seventh_tier_ascendants_are_gated_and_reversible(self):
        hud = (ROOT / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        header = (ROOT / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        flags = ("bTalentDuskSovereign", "bTalentSoulConduit", "bTalentUnbrokenBastion")
        for flag in flags:
            self.assertIn(flag + " = false;", header)
            self.assertIn("Hero->" + flag, hud)
            self.assertIn(flag + " = false;", body(self.hero, "bool AMoonfallHeroPawn::RespecTalents()"))
        gate = body(self.hero, "bool AMoonfallHeroPawn::CanUnlockTalentNode(int32 Node) const")
        for part in ("Node == 18", "Node == 19", "Node == 20",
                     "bTalentEclipseHeart && bTalentWorldguard",
                     "bTalentEclipseHeart && bTalentInfiniteCurrent",
                     "bTalentInfiniteCurrent && bTalentWorldguard"):
            self.assertIn(part, gate)
        unlock = body(self.hero, "bool AMoonfallHeroPawn::UnlockTalentNode(int32 Node)")
        self.assertIn("Node == 20 ? &bTalentUnbrokenBastion", unlock)
        points = body(self.hero, "int32 AMoonfallHeroPawn::GetUnspentTalentPoints() const")
        self.assertIn("int32(bTalentDuskSovereign)", points)
        self.assertIn("int32(bTalentUnbrokenBastion)", points)
        cast = body(self.hero, "bool AMoonfallHeroPawn::TryCast(int32 School, int32 Hand, float Multiplier, bool bOvercharged)")
        self.assertIn("(bTalentDuskSovereign ? 0.20f : 0.f)", cast)
        self.assertIn("(bTalentSoulConduit ? 0.95f : 1.f)", cast)
        hurt = body(self.hero, "void AMoonfallHeroPawn::ApplyMoonfallDamage(float Amount)")
        self.assertIn("if (bTalentUnbrokenBastion) DamageAfterGuard *= 0.95f;", hurt)
        self.assertIn("const TCHAR* AscendantNames[]", hud)
        self.assertIn("const int32 AscendantA[] = {0,0,1};", hud)
        self.assertIn("const int32 AscendantB[] = {2,1,2};", hud)
        self.assertIn("for (int32 Tier = 0; Tier < 9; ++Tier)", hud)
        self.assertIn("Selected star %d / 27", hud)
        self.assertIn("(FMath::Clamp(TalentSelectedNode,0,137) + 1) % 138", self.hero)
        self.assertIn("(FMath::Clamp(TalentSelectedNode,0,137) + 137) % 138", self.hero)

    def test_eighth_tier_transcendent_unlocks_are_connected_and_bounded(self):
        hud = (ROOT / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        header = (ROOT / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        flags = ("bTalentNightfallAscension", "bTalentRadiantNexus", "bTalentTitanSoul")
        for flag in flags:
            self.assertIn(flag + " = false;", header)
            self.assertIn("Hero->" + flag, hud)
            self.assertIn(flag + " = false;", body(self.hero, "bool AMoonfallHeroPawn::RespecTalents()"))
        gate = body(self.hero, "bool AMoonfallHeroPawn::CanUnlockTalentNode(int32 Node) const")
        for fragment in ("Node == 21", "Node == 22", "Node == 23",
                         "bTalentDuskSovereign && bTalentUnbrokenBastion",
                         "bTalentDuskSovereign && bTalentSoulConduit",
                         "bTalentSoulConduit && bTalentUnbrokenBastion"):
            self.assertIn(fragment, gate)
        unlock = body(self.hero, "bool AMoonfallHeroPawn::UnlockTalentNode(int32 Node)")
        self.assertIn("Node == 23 ? &bTalentTitanSoul", unlock)
        points = body(self.hero, "int32 AMoonfallHeroPawn::GetUnspentTalentPoints() const")
        self.assertIn("int32(bTalentNightfallAscension)", points)
        self.assertIn("int32(bTalentTitanSoul)", points)
        cast = body(self.hero, "bool AMoonfallHeroPawn::TryCast(int32 School, int32 Hand, float Multiplier, bool bOvercharged)")
        self.assertIn("(bTalentNightfallAscension ? 0.15f : 0.f)", cast)
        self.assertIn("(bTalentRadiantNexus ? 0.95f : 1.f)", cast)
        hurt = body(self.hero, "void AMoonfallHeroPawn::ApplyMoonfallDamage(float Amount)")
        self.assertIn("if (bTalentTitanSoul) DamageAfterGuard *= 0.95f;", hurt)
        self.assertIn("const TCHAR* TranscendentNames[]", hud)
        self.assertIn("const int32 TranscendentA[] = {0,0,1};", hud)
        self.assertIn("const int32 TranscendentB[] = {2,1,2};", hud)
        self.assertIn("for (int32 Tier = 0; Tier < 9; ++Tier)", hud)
        self.assertIn("Selected star %d / 27", hud)
        self.assertIn("(FMath::Clamp(TalentSelectedNode,0,137) + 1) % 138", self.hero)
        self.assertIn("(FMath::Clamp(TalentSelectedNode,0,137) + 137) % 138", self.hero)
        hit = body(hud, "int32 AMoonfallCombatHUD::GetTalentNodeAtScreenPosition(")
        self.assertIn("for (int32 Tier = 0; Tier < 9; ++Tier)", hit)
        self.assertIn("ClosestNode = Tier * 3 + Branch;", hit)

    def test_ninth_tier_convergence_skills_have_real_gated_effects(self):
        hud = (ROOT / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        header = (ROOT / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        flags = ("bTalentCrimsonNova", "bTalentAetherCovenant", "bTalentLastCitadel")
        for flag in flags:
            self.assertIn(flag + " = false;", header)
            self.assertIn("Hero->" + flag, hud)
            self.assertIn(flag + " = false;", body(self.hero, "bool AMoonfallHeroPawn::RespecTalents()"))
        gate = body(self.hero, "bool AMoonfallHeroPawn::CanUnlockTalentNode(int32 Node) const")
        for fragment in ("Node == 24", "Node == 25", "Node == 26",
                         "bTalentNightfallAscension && bTalentTitanSoul",
                         "bTalentNightfallAscension && bTalentRadiantNexus",
                         "bTalentRadiantNexus && bTalentTitanSoul"):
            self.assertIn(fragment, gate)
        unlock = body(self.hero, "bool AMoonfallHeroPawn::UnlockTalentNode(int32 Node)")
        self.assertIn("Node == 26 ? &bTalentLastCitadel", unlock)
        points = body(self.hero, "int32 AMoonfallHeroPawn::GetUnspentTalentPoints() const")
        self.assertIn("int32(bTalentCrimsonNova)", points)
        self.assertIn("int32(bTalentLastCitadel)", points)
        cast = body(self.hero, "bool AMoonfallHeroPawn::TryCast(int32 School, int32 Hand, float Multiplier, bool bOvercharged)")
        self.assertIn("(bTalentCrimsonNova ? 0.10f : 0.f)", cast)
        self.assertIn("(bTalentAetherCovenant ? 0.95f : 1.f)", cast)
        hurt = body(self.hero, "void AMoonfallHeroPawn::ApplyMoonfallDamage(float Amount)")
        self.assertIn("if (bTalentLastCitadel) DamageAfterGuard *= 0.95f;", hurt)
        self.assertIn("const TCHAR* ConvergenceNames[]", hud)
        self.assertIn("const int32 ConvergenceA[] = {0,0,1};", hud)
        self.assertIn("const int32 ConvergenceB[] = {2,1,2};", hud)
        self.assertIn("for (int32 Tier = 0; Tier < 9; ++Tier)", hud)
        self.assertIn("Selected star %d / 27", hud)
        self.assertIn("(FMath::Clamp(TalentSelectedNode,0,137) + 1) % 138", self.hero)
        self.assertIn("(FMath::Clamp(TalentSelectedNode,0,137) + 137) % 138", self.hero)
        hit = body(hud, "int32 AMoonfallCombatHUD::GetTalentNodeAtScreenPosition(")
        self.assertIn("for (int32 Tier = 0; Tier < 9; ++Tier)", hit)
        self.assertIn("ClosestNode = Tier * 3 + Branch;", hit)

    def test_constellation_spacing_and_max_zoom_are_safe(self):
        hud = (ROOT / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        panel = hud.split("if (Hero->bAtlasOpen && Hero->bTalentsOpen) {", 1)[1].split(
            "if (Hero->bAtlasOpen) {", 1)[0]
        hit = body(hud, "int32 AMoonfallCombatHUD::GetTalentNodeAtScreenPosition(")
        # The input hit test and rendering must be based on identical coordinates,
        # with no overlapping giant hit circles stealing points from other nodes.
        self.assertEqual(hud.count("const float LayoutZoom = FMath::Min(Zoom,1.10f);"), 2)
        self.assertEqual(hud.count("(12.f + Tier * 0.75f) * Scale * Zoom"), 2)
        self.assertIn("ClosestNode = Tier * 3 + Branch;", hit)
        self.assertIn("return ClosestNode;", hit)
        self.assertNotIn("if (DX * DX + DY * DY <= Radius * Radius) return", hit)
        for fraction in ("0.19f", "0.26f", "0.33f", "0.40f", "0.47f",
                         "0.54f", "0.61f", "0.68f", "0.75f"):
            expected = f"({fraction} - 0.47f) * LayoutZoom"
            self.assertIn(expected, hit)
            self.assertIn(expected, panel)
        self.assertIn("if (!bSmall || Hero->TalentSelectedNode == Tier * 3 + Branch)", panel)
        self.assertIn("const float FocusR = R + 5.f * Scale;", panel)
        self.assertIn("Unlock higher tiers by owning both linked stars.", panel)
        # Zooming must not place the first or last row outside the visible panel.
        for zoom in (0.75, 1.0, 1.1, 1.35):
            effective = min(zoom, 1.10)
            ys = [0.47 + (0.19 + 0.07 * i - 0.47) * effective for i in range(9)]
            self.assertTrue(all(0.08 < y < 0.82 for y in ys))
            self.assertTrue(all(b - a > 0 for a, b in zip(ys, ys[1:])))

    def test_111_astral_talents_are_real_gated_paged_and_bounded(self):
        hud = (ROOT / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        header = (ROOT / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        inputs = (ROOT.parents[1] / "Config" / "DefaultInput.ini").read_text(encoding="utf-8")
        for field in ("static constexpr int32 AstralTalentCount = 111",
                      "TArray<uint8> AstralTalentUnlocked",
                      "CanUnlockAstralTalent(int32 Index)",
                      "UnlockAstralTalent(int32 Index)",
                      "GetAstralTalentName(int32 Index)"):
            self.assertIn(field, header)
        self.assertIn("AstralTalentUnlocked.Init(uint8(0), AstralTalentCount);", self.hero)
        self.assertIn("GetAstralTalentCount(-1)", body(self.hero,
            "int32 AMoonfallHeroPawn::GetUnspentTalentPoints() const"))
        self.assertIn("GetAstralTalentCount(0) * 0.002f", self.hero)
        self.assertIn("GetAstralTalentCount(1) * 0.003f", self.hero)
        self.assertIn("GetAstralTalentCount(2) * 0.003f", self.hero)
        self.assertIn("Node >= 138", body(self.hero,
            "bool AMoonfallHeroPawn::ActivateTalentNode(int32 Node)"))
        self.assertIn("UnlockAstralTalent(Node - 27)", self.hero)
        gate = body(self.hero, "bool AMoonfallHeroPawn::CanUnlockAstralTalent(int32 Index) const")
        for condition in ("Index < 0 || Index >= AstralTalentCount",
                          "GetUnspentTalentPoints() <= 0", "Stage == 0",
                          "bTalentCrimsonNova,bTalentAetherCovenant,bTalentLastCitadel",
                          "IsAstralTalentUnlocked(Index - 3)",
                          "Stage % 6 == 0", "(Stage - 1) * 3 + (Branch + 1) % 3"):
            self.assertIn(condition, gate)
        self.assertIn("AstralTalentUnlocked[Index] = 1;", body(self.hero,
            "bool AMoonfallHeroPawn::UnlockAstralTalent(int32 Index)"))
        respec = body(self.hero, "bool AMoonfallHeroPawn::RespecTalents()")
        self.assertIn("AstralTalentUnlocked.Init(uint8(0), AstralTalentCount);", respec)
        name = body(self.hero, "FString AMoonfallHeroPawn::GetAstralTalentName(int32 Index) const")
        self.assertIn("Motifs[37]", name)
        self.assertIn("Branches[3]", name)
        for branch in range(3):
            indices = [3 * stage + branch for stage in range(37)]
            self.assertEqual(len(indices), 37)
            self.assertTrue(all(0 <= i < 111 for i in indices))
        self.assertIn("MoonfallAstralStarPosition(", hud)
        hit = body(hud, "int32 AMoonfallCombatHUD::GetTalentNodeAtScreenPosition(")
        self.assertIn("Node >= 138", hit)
        self.assertIn("BestNode = Node;", hit)
        self.assertIn("return BestNode;", hit)
        self.assertIn("if (Hero->TalentSelectedNode >= 27)", hud)
        self.assertIn("ASTRAL CONSTELLATION", hud)
        self.assertIn("Page * 27 + Local", hud)
        self.assertIn("111 new talents", hud)
        self.assertIn("GetAstralTalentName(Selected)", hud)
        self.assertIn("CanUnlockAstralTalent(Astral)", hud)
        for key in ("PageDown", "PageUp", "Gamepad_LeftShoulder", "Gamepad_RightShoulder"):
            self.assertIn("Key=" + key, inputs)
        self.assertIn("SelectNextTalentPage()", self.hero)
        self.assertIn("SelectPreviousTalentPage()", self.hero)
        self.assertIn("% 138", body(self.hero,
            "void AMoonfallHeroPawn::SelectNextTalentNode()"))
        self.assertIn("% 138", body(self.hero,
            "void AMoonfallHeroPawn::SelectPreviousTalentNode()"))

    def test_forever_inspired_stats_are_derived_from_equipped_items(self):
        header = (ROOT / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        getters = ("GetStrength", "GetAgility", "GetStaminaAttribute", "GetIntellect",
                   "GetSpirit", "GetArmorRating",
                   "GetDefenseRating", "GetMeleeAttackPower", "GetRangedAttackPower",
                   "GetSpellDamagePower", "GetSpellHealingPower",
                   "GetSpellPenetration", "GetSpellPierce", "GetResistance",
                   "GetExpertiseRating", "GetManaRegenPerFive", "GetHealthRegenPerFive")
        for method in getters:
            self.assertIn(method + "(", header)
            self.assertIn("AMoonfallHeroPawn::" + method + "(", self.hero)
        for method in ("GetStrength", "GetAgility", "GetStaminaAttribute",
                       "GetIntellect", "GetArmorRating", "GetSpellDamagePower"):
            # No non-equipped inventory item should grant a character bonus.
            part = self.hero.split("AMoonfallHeroPawn::" + method + "(",1)[1].split("\n}\n",1)[0]
            self.assertNotIn("InventoryItemCodes",part)
        for reference in ("ValidEquippedTier(EquippedWeaponCode,0)",
                          "ValidEquippedTier(EquippedAccessoryCode,2)",
                          "ValidEquippedTier(EquippedArmorCode,1)"):
            self.assertIn(reference,self.hero)
        resistance = self.hero.split("AMoonfallHeroPawn::GetResistance(",1)[1].split("\n}\n",1)[0]
        self.assertIn("School < 0 || School >= 5",resistance)
        self.assertIn("FMath::Clamp(Natural+SchoolBonus,0.f,100.f)",resistance)

    def test_forever_stats_have_safe_combat_hooks_and_character_pages(self):
        hud = (ROOT / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        header = (ROOT / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        inputs = (ROOT.parents[1] / "Config" / "DefaultInput.ini").read_text(encoding="utf-8")
        self.assertIn("CharacterStatsPage = 0",header)
        self.assertIn("void AMoonfallHeroPawn::NextCharacterStatsPage()",self.hero)
        self.assertIn("void AMoonfallHeroPawn::PreviousCharacterStatsPage()",self.hero)
        self.assertIn("bInventoryOpen && bCharacterStatsOpen",self.hero)
        for mapping in ("CharacterStatsNext", "CharacterStatsPrevious"):
            self.assertIn('ActionName="' + mapping + '"',inputs)
            self.assertIn('BindAction(TEXT("' + mapping + '")',self.hero)
        self.assertIn("if (Hero->CharacterStatsPage != 0)",hud)
        self.assertIn("StatLinesOffense[]",hud)
        self.assertIn("StatLinesDefense[]",hud)
        for label in ("Strength:", "Agility:", "Stamina:", "Intellect:", "Spirit:",
                      "Critical Strike:", "Melee AP:", "Ranged AP:", "Spell Damage:",
                      "Spell Healing:", "Spell Penetration:", "Spell Pierce:",
                      "Armor:", "Defense:", "Fire:", "Frost:", "Nature:",
                      "Shadow:", "Arcane:"):
            self.assertIn(label,hud)
        self.assertIn("GetSpellHealingPower()*0.0002f",self.hero)
        self.assertIn("GetSpellDamagePower()*0.0002f",self.hero)
        self.assertIn("GetArmorRating()*0.000025f",self.hero)
        self.assertIn("GetManaRegenPerFive() * 0.20f * SafeRecoveryDelta * CombatManaFactor",self.hero)
        self.assertIn("GetHealthRegenPerFive() * 0.20f * SafeRecoveryDelta",self.hero)
        self.assertIn("GetStaminaAttribute()*0.02f",self.hero)
        self.assertIn("GetIntellect()*0.01f",self.hero)
        for label in ("Expertise:", "MP5:", "HP regen/5s:"):
            self.assertIn(label,hud)
        self.assertIn("Mana -= Costs[School] * ManaCostMultiplier;",self.hero)
        self.assertIn("LastCastTime[Hand] = Time;",self.hero)

    def test_unwanted_hit_dodge_parry_block_stats_are_absent(self):
        hud = (ROOT / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        header = (ROOT / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        for name in ("GetHitChance", "GetDodgeChance", "GetParryChance",
                     "GetBlockChance", "GetBlockValue"):
            self.assertNotIn(name, header)
            self.assertNotIn("AMoonfallHeroPawn::" + name, self.hero)
            self.assertNotIn("Hero->" + name, hud)
        for label in ("Hit Chance:", "Dodge:", "Parry:", "Block chance:", "Block value:"):
            self.assertNotIn(label, hud)
        self.assertIn("Enemy->TakeSpellDamage(SpellDamage);", self.hero)
        self.assertNotIn("bSpellHit", self.hero)
        self.assertIn("DamageAfterGuard *= 0.4f;", self.hero)
        self.assertIn("Guard, dodge and parry use player input, not passive stats", hud)

    def test_elemental_resistances_and_penetration_affect_actual_combat(self):
        enemy_h = (ROOT / "MoonfallEnemy.h").read_text(encoding="utf-8")
        enemy = (ROOT / "MoonfallEnemy.cpp").read_text(encoding="utf-8")
        hero_h = (ROOT / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        hud = (ROOT / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        for name in ("FireResistance", "FrostResistance", "NatureResistance",
                     "ShadowResistance", "ArcaneResistance"):
            self.assertIn("float " + name + " = 0.f;", enemy_h)
            self.assertIn(name, enemy)
        self.assertIn("AttackMagicSchool = -1;", enemy_h)
        self.assertIn("GetSchoolResistance(int32 School) const", enemy_h)
        self.assertIn("FMath::Clamp(Resistance,0.f,100.f)", enemy)
        self.assertIn("Hero->ApplyMoonfallElementalDamageFrom(", enemy)
        self.assertIn("Hero->ApplyMoonfallDamageFrom(", enemy)
        self.assertIn("bool ApplyMoonfallElementalDamageFrom(", hero_h)
        self.assertIn("IncomingDamageSchool = -1;", hero_h)
        magical = body(self.hero,
            "bool AMoonfallHeroPawn::ApplyMoonfallElementalDamageFrom(")
        self.assertIn("if (School < 0 || School >= 5) return ApplyMoonfallDamageFrom", magical)
        self.assertIn("IncomingDamageSchool = School;", magical)
        self.assertIn("IncomingDamageSchool = -1;", magical)
        hurt = body(self.hero,"void AMoonfallHeroPawn::ApplyMoonfallDamage(float Amount)")
        self.assertIn("GetResistance(IncomingDamageSchool)*0.004f",hurt)
        self.assertIn("SchoolMitigation : ArmorMitigation;",hurt)
        self.assertIn("DamageAfterGuard *= 0.4f;",hurt)
        cast = body(self.hero,
            "bool AMoonfallHeroPawn::TryCast(int32 School, int32 Hand, float Multiplier, bool bOvercharged)")
        self.assertIn("ElementSchoolForCast[6] = {0,1,4,-1,2,3}",cast)
        self.assertIn("Enemy->GetSchoolResistance(ElementSchool)",cast)
        self.assertIn("- GetSpellPenetration() - GetSpellPierce()",cast)
        self.assertIn("ResistanceMultiplier * (1.f + GetMasteryBonus())",cast)
        self.assertIn("Elemental defenses active for school attacks",hud)
        for label in ("GetHitChance", "GetDodgeChance", "GetParryChance",
                      "GetBlockChance", "GetBlockValue"):
            self.assertNotIn(label,self.hero)
            self.assertNotIn(label,hero_h)

    def test_progress_save_is_versioned_validated_and_not_per_frame(self):
        save_path = ROOT / "MoonfallProgressSaveGame.h"
        save = save_path.read_text(encoding="utf-8")
        header = (ROOT / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        self.assertIn("class MOONFALLUNREAL_API UMoonfallProgressSaveGame : public USaveGame", save)
        self.assertIn("Version = 1",save)
        self.assertIn("TArray<int32> RootRanks",save)
        self.assertIn("TArray<uint8> CoreUnlocks",save)
        self.assertIn("TArray<uint8> AstralUnlocks",save)
        self.assertIn("EndPlay(const EEndPlayReason::Type EndPlayReason) override",header)
        self.assertIn("SaveProgress()",header)
        self.assertIn("LoadProgress()",header)
        self.assertIn("LoadProgress();",body(self.hero,"void AMoonfallHeroPawn::BeginPlay()"))
        saving = body(self.hero,"bool AMoonfallHeroPawn::SaveProgress()")
        loading = body(self.hero,"bool AMoonfallHeroPawn::LoadProgress()")
        self.assertIn("SaveGameToSlot(Save,TEXT(\"MoonfallHeroProgressV1\"),0)",saving)
        self.assertIn("LoadGameFromSlot(TEXT(\"MoonfallHeroProgressV1\"),0)",loading)
        self.assertIn("Save->CoreUnlocks.Num() != 24",loading)
        self.assertIn("Save->AstralUnlocks.Num() != AstralTalentCount",loading)
        self.assertIn("Save->Version != 1",loading)
        self.assertIn("Spent > Save->Level - 1",loading)
        self.assertIn("Save->RootRanks[I] < 3",loading)
        self.assertIn("PrerequisiteA[24]",loading)
        self.assertIn("PrerequisiteB[24]",loading)
        self.assertIn("Stage % 6 == 0",loading)
        self.assertIn("if (Stage == 0)",loading)
        self.assertIn("AstralTalentUnlocked = Save->AstralUnlocks",loading)
        self.assertIn("MaxHealth - 2.f * FMath::Clamp(ArmorTier,0,25)",saving)
        self.assertIn("MaxHealth = Save->MaxHealth + 2.f * FMath::Clamp(ArmorTier,0,25)",loading)
        self.assertIn("if (bProgressDirty && GetWorld()",body(self.hero,"void AMoonfallHeroPawn::Tick(float DeltaSeconds)"))
        self.assertIn("LastProgressSaveAt >= 16.f",self.hero)
        self.assertIn("if (bProgressDirty) SaveProgress();",body(self.hero,"void AMoonfallHeroPawn::EndPlay("))
        for method in ("SpendTalentPoint(int32 Branch)", "UnlockTalentNode(int32 Node)",
                       "UnlockAstralTalent(int32 Index)", "RespecTalents()"):
            # Every valid edit marks the snapshot dirty; invalid attempts cannot trigger saving.
            section = self.hero.split("AMoonfallHeroPawn::"+method,1)[1].split("\n}\n",1)[0]
            self.assertIn("bProgressDirty = true;",section)
        self.assertIn("bProgressDirty = true;",body(self.hero,"void AMoonfallHeroPawn::AwardExperience(int32 Amount)"))
        self.assertNotIn("GetHitChance(",self.hero)
        self.assertNotIn("GetDodgeChance(",self.hero)
        self.assertNotIn("GetParryChance(",self.hero)

    def test_inventory_and_gear_persist_in_validated_v2_snapshots(self):
        save = (ROOT / "MoonfallProgressSaveGame.h").read_text(encoding="utf-8")
        for field in ("InventoryCodes", "EquippedWeapon", "EquippedArmor",
                      "EquippedAccessory", "SalvageMaterials"):
            self.assertIn(field, save)
        saving = body(self.hero,"bool AMoonfallHeroPawn::SaveProgress()")
        loading = body(self.hero,"bool AMoonfallHeroPawn::LoadProgress()")
        self.assertIn("Save->Version = 2;", saving)
        self.assertIn("Save->InventoryCodes = InventoryItemCodes;", saving)
        self.assertIn("Save->SalvageMaterials = Salvage;", saving)
        self.assertIn("Save->InventoryCodes.Num() > MaxInventoryItems", saving)
        self.assertIn("!DecodeEquippedItem(Code,Slot,Trait,Rarity,Tier)", saving)
        self.assertIn("!Save->InventoryCodes.Contains(WornCodes[Slot])", saving)
        self.assertIn("(Save->Version != 1 && Save->Version != 2)", loading)
        self.assertIn("if (Save->Version == 2)", loading)
        self.assertIn("Save->InventoryCodes.Num() > MaxInventoryItems", loading)
        self.assertIn("!DecodeEquippedItem(Code,Slot,Trait,Rarity,Tier)", loading)
        self.assertIn("!Save->InventoryCodes.Contains(WornCodes[Slot])", loading)
        self.assertLess(loading.index("Save->InventoryCodes.Num() > MaxInventoryItems"),
                        loading.index("Level = Save->Level;"))
        self.assertIn("InventoryItemCodes = Save->InventoryCodes;", loading)
        self.assertIn("EquippedWeaponCode = Save->EquippedWeapon;", loading)
        self.assertIn("EquippedArmorCode = Save->EquippedArmor;", loading)
        self.assertIn("EquippedAccessoryCode = Save->EquippedAccessory;", loading)
        self.assertIn("WeaponTier = ValidEquippedTier(EquippedWeaponCode,0);", loading)
        self.assertIn("ArmorTier = ValidEquippedTier(EquippedArmorCode,1);", loading)
        self.assertIn("AccessoryTier = ValidEquippedTier(EquippedAccessoryCode,2);", loading)
        self.assertIn("EquipmentTier = FMath::Max", loading)
        self.assertIn("Salvage = Save->SalvageMaterials;", loading)
        for method in ("EquipInventoryItem", "SalvageInventoryItem",
                       "UpgradeInventoryItem", "ReforgeInventoryItemAffix",
                       "AscendInventoryItemRarity", "CollectGroundEquipmentCode"):
            piece = self.hero.split("AMoonfallHeroPawn::"+method+"(",1)[1].split("\n}\n",1)[0]
            self.assertIn("bProgressDirty = true;",piece)
        award = self.hero.split("AMoonfallHeroPawn::AwardEquipmentDrop(",1)[1].split("\n}\n",1)[0]
        self.assertIn("bProgressDirty = true;",award)
        self.assertNotIn("GetHitChance(",self.hero)
        self.assertNotIn("GetDodgeChance(",self.hero)
        self.assertNotIn("GetParryChance(",self.hero)

    def test_inventory_has_no_equip_or_page_buttons(self):
        hud = (ROOT / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        left = body(self.hero, "void AMoonfallHeroPawn::LeftClickContextAction()")
        self.assertNotIn("IsInventoryEquipButtonAtScreenPosition", hud + left)
        self.assertNotIn('TEXT("EQUIP SELECTED")', hud)
        self.assertNotIn('TEXT("PREV PAGE")', hud)
        self.assertNotIn('TEXT("NEXT PAGE")', hud)
        self.assertIn("case 0: EquipSelectedInventoryItem(); break;", left)
        self.assertIn("if (InventoryItemCodes.IsValidIndex(ClickedIndex))", left)
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
        self.assertIn('ActionName="Atlas",Key=M', inputs)
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

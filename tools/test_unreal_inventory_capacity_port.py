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
            self.assertIn("if (bInventoryOpen)", action)
            self.assertIn(change, action)
            self.assertIn("GetInventoryVisibleRowCount()", action)
            self.assertIn("InventoryItemCodes.Num() - VisibleRows", action)
            self.assertNotIn("InventorySelection =", action)
            self.assertIn("return;", action)
            self.assertLess(action.index("if (bInventoryOpen)"),
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
        self.assertEqual(hud.count("860.f, FMath::Max(280.f, Canvas->SizeY - 24.f)"), 4)
        self.assertEqual(hud.count("FMath::FloorToInt((PanelHeight - 355.f) / 25.f), 2, 20"), 3)
        self.assertEqual(hud.count("FMath::Max(21.f, (PanelHeight - 355.f) / PerPage)"), 2)
        self.assertEqual(hud.count("Hero->InventoryViewStart"), 2)
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
        self.assertIn("DX * DX + DY * DY <= Radius * Radius", hit)
        self.assertIn("return Tier * 3 + Branch;", hit)
        left = body(self.hero, "void AMoonfallHeroPawn::LeftClickContextAction()")
        self.assertIn("if (bAtlasOpen && bTalentsOpen)", left)
        self.assertIn("HUD->GetTalentNodeAtScreenPosition(this,X,Y)", left)
        self.assertIn("ActivateTalentNode(Node);", left)
        self.assertIn("if (bAtlasOpen) return;", left)
        activation = body(self.hero, "bool AMoonfallHeroPawn::ActivateTalentNode(int32 Node)")
        self.assertIn("Node < 0 || Node >= 27", activation)
        self.assertIn("SpendTalentPoint(Node) : UnlockTalentNode(Node)", activation)
        zoomin = body(self.hero, "void AMoonfallHeroPawn::ZoomCameraIn()")
        zoomout = body(self.hero, "void AMoonfallHeroPawn::ZoomCameraOut()")
        self.assertIn("TalentZoom = FMath::Clamp", zoomin)
        self.assertIn("TalentZoom = FMath::Clamp", zoomout)
        self.assertIn("CameraDistance = FMath::Clamp", zoomin)
        self.assertIn("CameraDistance = FMath::Clamp", zoomout)
        mouse = body(self.hero, "void AMoonfallHeroPawn::UpdateInventoryMouseMode()")
        self.assertIn("bInventoryOpen || (bAtlasOpen && bTalentsOpen)", mouse)
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
        self.assertIn("Node >= 27", act)
        self.assertIn("UnlockTalentNode(Node)", act)
        self.assertIn("% 27", body(self.hero, "void AMoonfallHeroPawn::SelectNextTalentNode()"))
        self.assertIn("% 27", body(self.hero, "void AMoonfallHeroPawn::SelectPreviousTalentNode()"))
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
        self.assertIn("return Tier * 3 + Branch;", hit)
        self.assertIn("Selected star %d / 27", hud)
        self.assertIn("% 27", body(self.hero, "void AMoonfallHeroPawn::SelectNextTalentNode()"))
        self.assertIn("% 27", body(self.hero, "void AMoonfallHeroPawn::SelectPreviousTalentNode()"))

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
        self.assertIn("(FMath::Clamp(TalentSelectedNode,0,26) + 1) % 27", self.hero)
        self.assertIn("(FMath::Clamp(TalentSelectedNode,0,26) + 26) % 27", self.hero)

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
        self.assertIn("(FMath::Clamp(TalentSelectedNode,0,26) + 1) % 27", self.hero)
        self.assertIn("(FMath::Clamp(TalentSelectedNode,0,26) + 26) % 27", self.hero)

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
        self.assertIn("(FMath::Clamp(TalentSelectedNode,0,26) + 1) % 27", self.hero)
        self.assertIn("(FMath::Clamp(TalentSelectedNode,0,26) + 26) % 27", self.hero)
        hit = body(hud, "int32 AMoonfallCombatHUD::GetTalentNodeAtScreenPosition(")
        self.assertIn("for (int32 Tier = 0; Tier < 9; ++Tier)", hit)
        self.assertIn("return Tier * 3 + Branch;", hit)

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
        self.assertIn("(FMath::Clamp(TalentSelectedNode,0,26) + 1) % 27", self.hero)
        self.assertIn("(FMath::Clamp(TalentSelectedNode,0,26) + 26) % 27", self.hero)
        hit = body(hud, "int32 AMoonfallCombatHUD::GetTalentNodeAtScreenPosition(")
        self.assertIn("for (int32 Tier = 0; Tier < 9; ++Tier)", hit)
        self.assertIn("return Tier * 3 + Branch;", hit)

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

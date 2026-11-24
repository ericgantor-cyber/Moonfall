"""Regression contracts for the native UE5 modal-panel presentation layer.

Source-level UI checks only. These do not claim Unreal Editor compilation,
live input testing, screen-reader compatibility or Windows packaging.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
HUD = (ROOT / "MoonfallUnreal/Source/MoonfallUnreal/MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
HERO = (ROOT / "MoonfallUnreal/Source/MoonfallUnreal/MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
INPUT = (ROOT / "MoonfallUnreal/Config/DefaultInput.ini").read_text(encoding="utf-8")


class TestNativeModalPanels(unittest.TestCase):
    def test_one_safe_shared_chrome_without_entity_scans(self):
        frame = HUD.split("const auto DrawModalChrome = [this]", 1)[1].split(
            "const auto DrawSelectionRail = [this]", 1
        )[0]
        self.assertIn("FMath::IsFinite(W)", frame)
        self.assertIn("W < 48.f || H < 48.f", frame)
        self.assertIn("DrawRect(Accent,L,T,W,4.f);", frame)
        self.assertIn("DrawRect(Edge,L+W-2.f,T,2.f,H);", frame)
        self.assertNotIn("GetAllActorsOfClass", frame)
        self.assertNotIn("TActorIterator", frame)

    def test_all_six_modal_surfaces_use_shared_chrome(self):
        expected = (
            "DrawModalChrome(L,T,W,H,FLinearColor(0.57f",  # talents
            "DrawModalChrome(L,T,W,H,FLinearColor(0.55f",  # NPC
            "DrawModalChrome(Left,Top,Width,Height,FLinearColor(",  # atlas
            "DrawModalChrome(MenuLeft,MenuTop,MenuWidth,MenuHeight,",  # options
            "DrawModalChrome(Left,Top,PanelWidth,PanelHeight,",  # bag/character
            "DrawModalChrome(Book.Left,Book.Top,Book.Width,Book.Height,",  # grimoire
        )
        for token in expected:
            with self.subTest(panel=token):
                self.assertIn(token, HUD)
        # The display-only function is defined once; no per-panel duplicates.
        self.assertEqual(HUD.count("const auto DrawModalChrome = [this]"), 1)

    def test_focus_is_visible_without_changing_click_geometry(self):
        focus = HUD.split("const auto DrawSelectionRail = [this]", 1)[1].split(
            "if (Hero->bTalentWebOpen)", 1
        )[0]
        self.assertIn("FMath::IsFinite", focus)
        self.assertIn("DrawRect(FLinearColor(1.f,0.80f,0.37f", focus)
        required = (
            "DrawSelectionRail(float(Tab.X),float(Tab.Y),float(Tab.H));",  # talent
            "DrawSelectionRail(L+14.f,Y,43.f);",  # NPC
            "DrawSelectionRail(MenuLeft+10.f,Y,RowHeight);",  # options
            "DrawSelectionRail(Left+12.f,Y-3.f,",  # inventory
            "DrawSelectionRail(Book.RowX,Y,Book.RowHeight);",  # spell book
            "DrawSelectionRail(X,StripY,22.f);",  # favorites
        )
        for token in required:
            with self.subTest(selection=token):
                self.assertIn(token, HUD)
        self.assertIn("HitTestInventoryRow()", HUD)
        self.assertIn("MoonfallTalentWeb::BranchTab(P,B)", HUD)
        self.assertIn("Hero->ResidentContextSelection", HUD)

    def test_shared_hover_covers_existing_click_targets_without_input_mutation(self):
        hover = HUD.split("const auto DrawHoverFrame = [this", 1)[1].split(
            "if (Hero->bTalentWebOpen)", 1
        )[0]
        self.assertEqual(HUD.count("const auto DrawHoverFrame = [this"), 1)
        for token in (
            "bPanelPointerVisible", "GetMousePosition(PanelPointerX,PanelPointerY)",
            "FMath::IsFinite(PanelPointerX)", "PanelPointerX < X",
            "PanelPointerY > Y+H", "DrawRect(Focus,X+W-2.f,Y,2.f,H);",
        ):
            self.assertIn(token, hover)
        for forbidden in (
            "SaveWorldProgress", "ChangeFactionReputation", "EquipInventoryItem",
            "SetGamePaused", "BindAction(", "GetAllActorsOfClass",
        ):
            self.assertNotIn(forbidden, hover)
        for token in (
            "DrawHoverFrame(float(Close.X),float(Close.Y),float(Close.W),float(Close.H));",
            "DrawHoverFrame(float(Tab.X),float(Tab.Y),float(Tab.W),float(Tab.H));",
            "DrawHoverFrame(float(Spend.X),float(Spend.Y),float(Spend.W),float(Spend.H));",
            "DrawHoverFrame(float(Respec.X),float(Respec.Y),float(Respec.W),float(Respec.H));",
            "DrawHoverFrame(NpcCloseX,T+12.f,28.f,28.f);",
            "DrawHoverFrame(L+14.f,Y,W-28.f,43.f);",
            "DrawHoverFrame(AtlasCloseX,Top+12.f,28.f,28.f);",
            "DrawHoverFrame(Left+13.f,Y-4.f,AtlasSidebarWidth-22.f,RowHeight);",
            "DrawHoverFrame(OptionsCloseX,MenuTop+12.f,28.f,28.f);",
            "DrawHoverFrame(MenuLeft+10.f,Y,MenuWidth-20.f,RowHeight);",
            "DrawHoverFrame(MinusX,Y,25.f,ButtonHeight);",
            "DrawHoverFrame(PlusX,Y,25.f,ButtonHeight);",
            "DrawHoverFrame(CloseButtonX,Top+14.f,28.f,28.f);",
            "DrawHoverFrame(TabX,PageTabTop,PageTabWidth,32.f);",
            "DrawHoverFrame(Left+15.f,Y,PanelWidth-30.f,ReputationRowHeight-3.f);",
            "DrawHoverFrame(Book.TabX,Book.TabY,Book.TabWidth,Book.TabHeight);",
            "DrawHoverFrame(Book.RowX,Y,Book.RowWidth,Book.RowHeight);",
            "DrawHoverFrame(SortX,SortY,SortWidth,28.f);",
            "DrawHoverFrame(EquipX,EquipY,172.f,30.f);",
            "DrawHoverFrame(X,StripY,SlotWidth-3.f,22.f);",
            "DrawHoverFrame(Left+12.f,Y-3.f,PanelWidth-24.f,RowSpacing-3.f);",
        ):
            with self.subTest(target=token):
                self.assertIn(token, HUD)

    def test_world_hud_does_not_overlap_boss_and_honors_reduced_motion(self):
        for token in (
            "const bool bQuestOverlapsBoss = bBossHudVisible",
            "BossHudLeft < Left+Width && BossHudLeft+BossHudWidth > Left",
            "const float Top = bQuestOverlapsBoss ? 232.f : 78.f;",
            "const float LowHealthPulse = bCriticalHealth && !Hero->bReduceMotion",
            "DisplayHealth = Hero->bReduceMotion ? ActualHealth",
            "DisplayMana = Hero->bReduceMotion ? ActualMana",
            "DisplayStamina = Hero->bReduceMotion ? ActualStamina",
            "DisplayXP = Hero->bReduceMotion ? ActualXP",
        ):
            self.assertIn(token, HUD)

    def test_tiny_viewports_have_safe_noninteractive_fallbacks(self):
        for token in (
            "Canvas->SizeX < 284.f || Canvas->SizeY < 294.f",
            "Canvas->SizeX < 264.f || Canvas->SizeY < 304.f",
            "Canvas->SizeX < 304.f || Canvas->SizeY < 304.f",
            "TEXT(\"NPC | Enlarge window | ESC / RMB: close\")",
            "TEXT(\"OPTIONS | Enlarge window | ESC to close\")",
            "TEXT(\"CHARACTER | Enlarge window | ESC: back\")",
            "TEXT(\"INVENTORY | Enlarge window | B / ESC: close\")",
        ):
            self.assertIn(token, HUD)
        self.assertIn("DrawLiveMenuHitCue();", HUD)

    def test_character_navigation_and_atlas_tab_unchanged(self):
        for token in (
            'ActionName="CharacterStats",Key=C',
            'ActionName="ChronicleFocus",Key=Tab',
        ):
            self.assertIn(token, INPUT)
        self.assertIn("void AMoonfallHeroPawn::SelectCharacterPage(int32 Page)", HERO)
        self.assertIn("const float TabW=(PanelW-60.f)/3.f;", HERO)
        self.assertIn("for (int32 Tab=0;Tab<3;++Tab)", HERO)
        self.assertIn('TEXT("STATS & GEAR")', HUD)
        self.assertIn('TEXT("EQUIPPED GEAR")', HUD)
        self.assertNotIn("ActionName=\"CharacterStats\",Key=Tab", INPUT)


if __name__ == "__main__":
    unittest.main()

"""Regression contracts for the native UE5 modal-panel presentation layer.

Source-level UI checks only. These do not claim Unreal Editor compilation,
live input testing, screen-reader compatibility or Windows packaging.
"""
from pathlib import Path
import unittest
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
HUD = (ROOT / "MoonfallUnreal/Source/MoonfallUnreal/MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
HERO = (ROOT / "MoonfallUnreal/Source/MoonfallUnreal/MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
INPUT = (ROOT / "MoonfallUnreal/Config/DefaultInput.ini").read_text(encoding="utf-8")


class TestNativeModalPanels(unittest.TestCase):
    def test_active_page_has_one_title_and_compact_character_heading(self):
        self.assertIn("if (!Hero->bCharacterStatsOpen && !Hero->bSpellBookOpen)\n   DrawText(PanelWidth < 550.f", HUD)
        self.assertIn("DrawText(CharacterTitle, FLinearColor::White,", HUD)
        self.assertIn("Top + 17.f, nullptr, PanelWidth < 550.f ? 0.80f : 1.05f", HUD)
        for title in ("CHARACTER", "ANCESTRY", "REPUTATION"):
            self.assertIn(f'PanelWidth < 550.f ? TEXT("{title}")', HUD)

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
            "DrawSelectionRail(L+14.f,Y,Pitch-10.f);",  # NPC
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
        # Read the cursor only once, before the hover painter is invoked.
        self.assertIn("GetMousePosition(PanelPointerX,PanelPointerY)", HUD)
        self.assertIn("FMath::IsFinite(PanelPointerX)", HUD)
        for token in (
            "bPanelPointerVisible", "PanelPointerX < X",
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
            "DrawHoverFrame(L+14.f,Y,W-28.f,Pitch-10.f);",
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

    def test_companion_and_sword_warning_art_preserve_gameplay(self):
        self.assertIn("const FLinearColor TellFoil", HUD)
        self.assertIn("DrawRect(TellFoil,TellX,TellY+39.f,TellWidth,1.f);", HUD)
        self.assertIn("const FLinearColor OathInk=bDown?", HUD)
        self.assertIn("DrawMoonfallSigil(X+W-17.f,Y+49.f,8.f,OathInk,2);", HUD)
        self.assertIn("(W-20.f)*HP/100.f", HUD)
        self.assertIn('TEXT("%s STRIKE | GUARD")', HUD)

    def test_boss_and_quest_engravings_preserve_existing_progress(self):
        self.assertIn("const FLinearColor BossFoil", HUD)
        self.assertIn("Phase<=3;++Phase", HUD)
        self.assertIn("Phase<=Boss->BossPhase", HUD)
        self.assertIn("const FLinearColor QuestFoil", HUD)
        self.assertIn("Width*Percent,16.f", HUD)
        self.assertIn("Width*WarningProgress,4.f", HUD)
        self.assertIn("const float Top = bQuestOverlapsBoss ? 232.f : 78.f;", HUD)
        self.assertIn("TrackWidth*(Progress/5.f)", HUD)

    def test_expanded_engraved_art_is_bounded_and_display_only(self):
        for token in (
            "K<=4;++K", "Octagonal enamel seal", "Rarity-colored metal setting",
            "cardinal gilded studs", "Heraldic escutcheon", "Four engraved compass points",
            "Static gold bezel ticks", "DrawOptionGlyph(MenuLeft+25.f,Y+RowHeight*0.5f,i,",
        ):
            self.assertIn(token, HUD)
        options = HUD.split("const auto DrawOptionGlyph=", 1)[1].split(
            "const auto DrawSelectionRail", 1)[0]
        self.assertIn("Option<0 || Option>=8", options)
        for index in range(7):
            self.assertIn(f"case {index}:", options)
        self.assertIn("default: // Display mode", options)
        for forbidden in ("BindAction", "SaveWorldProgress", "LoadObject", "TActorIterator"):
            self.assertNotIn(forbidden, options)

    def test_constellation_interface_design_sheet_has_sixteen_editable_symbols(self):
        source = ROOT / "MoonfallUnreal/Art/UI/moonfall_constellation_interface_engraving.svg"
        root = ET.parse(source).getroot()
        ns = "{http://www.w3.org/2000/svg}"
        self.assertEqual(root.tag, ns + "svg")
        self.assertEqual(root.attrib["viewBox"], "0 0 760 870")
        tiles = [g for g in root if g.tag == ns + "g"]
        self.assertEqual(len(tiles), 16)
        self.assertEqual(len({g.attrib["id"] for g in tiles}), 16)
        for name in ("health", "mana", "stamina", "melee", "ranged", "magic",
                     "resolution", "render-scale", "quality", "frame-gen",
                     "reduced-motion", "combat-text", "boss-alerts", "display-mode",
                     "companion-oath", "boss-phase"):
            self.assertIn("rune-" + name, {g.attrib["id"] for g in tiles})
        for forbidden in ("script", "image", "foreignObject"):
            self.assertFalse(list(root.iter(ns + forbidden)))

    def test_talent_stat_runes_are_bounded_and_use_real_catalog_stats(self):
        art = HUD.split("const auto DrawTalentRune=", 1)[1].split("for(int I=0;", 1)[0]
        for stat in ("Health", "Mana", "Stamina", "MeleeDamage", "RangedDamage", "MagicDamage"):
            self.assertIn("case MoonfallTalents::Stat::" + stat, art)
        self.assertIn("NodeRect.W>=132 && NodeRect.H>=36", HUD)
        self.assertIn("float(NodeRect.Y+NodeRect.H-12),Node.AffectedStat,RuneInk", HUD)
        self.assertNotIn("SpendTalentPoint(", art)
        self.assertNotIn("LoadObject", art)
        self.assertNotIn("TActorIterator", art)

    def test_original_runtime_art_and_six_distinct_panel_signatures(self):
        chrome = HUD.split("const auto DrawModalChrome = [this]", 1)[1].split(
            "const auto DrawMoonfallSigil = [this]", 1
        )[0]
        self.assertIn("Original Moonfall engraving:", chrome)
        self.assertIn("const float Inset=7.f, Corner=11.f;", chrome)
        self.assertIn("Canvas->K2_DrawLine(", chrome)
        art = HUD.split("const auto DrawMoonfallSigil = [this]", 1)[1].split(
            "const auto DrawSelectionRail = [this]", 1
        )[0]
        self.assertEqual(HUD.count("const auto DrawMoonfallSigil = [this]"), 1)
        for token in (
            "constexpr int32 Segments=12;", "Radius<3.f || Radius>35.f",
            "FMath::IsFinite(Cx)", "FMath::IsFinite(Cy)",
            "switch (Type%4)", "Canvas->K2_DrawLine(",
        ):
            self.assertIn(token, art)
        for forbidden in ("TActorIterator", "GetAllActorsOfClass",
                          "SaveWorldProgress", "BindAction(", "GetWorld()->SpawnActor"):
            self.assertNotIn(forbidden, art)
        # The rendered art is wired to all six of the actual, native menus.
        for token in (
            "DrawMoonfallSigil(L+W-65.f,T+25.f",  # talent / NPC
            "DrawMoonfallSigil(Left+Width-67.f,Top+24.f",  # atlas
            "DrawMoonfallSigil(MenuLeft+MenuWidth-66.f,MenuTop+25.f",  # options
            "DrawMoonfallSigil(Left+PanelWidth-23.f,Top+PanelHeight-23.f",  # bag
            "DrawMoonfallSigil(Book.Left+Book.Width-26.f",  # book
            "DrawMoonfallSigil(float(Tab.X+Tab.W-13)",  # talents
            "DrawFactionCrest(Left+PanelWidth-30.f,Y+13.f",  # actual faction art
            "DrawGearGlyph(Left+PanelWidth-29.f,Y+CardHeight*0.5f",  # real gear icons
            "DrawMoonfallSigil(X+Width-13.f,Y+40.f",  # boss
            "DrawRegionEmblem(Left+Width-16.f,Top+109.f",  # quest current region
            "DrawMoonfallSigil(CX,CY-Radius*1.28f",  # resource orb
        ):
            with self.subTest(signature=token):
                self.assertIn(token, HUD)

    def test_eclipse_source_art_is_original_editable_svg(self):
        source = ROOT / "MoonfallUnreal/Art/UI/moonfall_eclipse_crest.svg"
        self.assertTrue(source.is_file())
        root = ET.parse(source).getroot()
        self.assertEqual(root.tag, "{http://www.w3.org/2000/svg}svg")
        self.assertEqual(root.attrib.get("viewBox"), "0 0 512 512")
        text = source.read_text(encoding="utf-8")
        for token in ("Moonfall · Eclipse Crest", 'id="crescent"',
                      'id="aether"', 'id="gold"'):
            self.assertIn(token, text)
        # Artwork is declarative reference SVG. Actual runtime visuals are
        # drawn by the native Canvas code, not by an unverified SVG importer.
        self.assertNotIn("<script", text)
        self.assertNotIn("<image", text)
        self.assertNotIn("http://", text.replace('xmlns="http://www.w3.org/2000/svg"', ""))

    def test_original_spell_school_and_gear_art_is_used_by_live_ui(self):
        spell = HUD.split("const auto DrawSpellGlyph = [this]", 1)[1].split(
            "const auto DrawGearGlyph = [this]", 1
        )[0]
        gear = HUD.split("const auto DrawGearGlyph = [this]", 1)[1].split(
            "const auto DrawSelectionRail = [this]", 1
        )[0]
        self.assertEqual(HUD.count("const auto DrawSpellGlyph = [this]"), 1)
        self.assertEqual(HUD.count("const auto DrawGearGlyph = [this]"), 1)
        for token in (
            "School<0 || School>=6", "R<4.f || R>20.f",
            "FMath::IsFinite(Cx)", "FMath::IsFinite(Cy)",
            "switch (School)", "case 0:", "case 1:", "case 2:",
            "case 3:", "case 4:", "default:", "Canvas->K2_DrawLine",
        ):
            self.assertIn(token, spell)
        for token in ("Slot<0 || Slot>=MoonfallEquipmentLayout::ActiveSlotCount", "R<4.f || R>19.f",
                      "switch (Slot)", "case 0:", "case 1:", "default:"):
            self.assertIn(token, gear)
        for forbidden in ("GetAllActorsOfClass", "TActorIterator",
                          "SaveWorldProgress", "ChangeFactionReputation",
                          "EquipInventoryItem(", "BindAction("):
            self.assertNotIn(forbidden, spell)
            self.assertNotIn(forbidden, gear)
        for token in (
            "DrawSpellGlyph(Book.RowX+19.f,",
            "School);",
            "Book.RowX+(bDrawSchoolIcon?37.f:12.f)",
            "DrawGearGlyph(Left+PanelWidth-29.f,Y+FMath::Min(11.f,RowSpacing*0.40f)",
            "DrawGearGlyph(Left+PanelWidth-29.f,Y+CardHeight*0.5f",
            "const bool bDrawSchoolIcon=Book.RowHeight>=25.f && Book.RowWidth>=350.f;",
            "if (bValid && PanelWidth>=440.f && RowSpacing>=23.f)",
            "DrawMoonfallSigil(CompassX,CompassY,13.f,",
            "if (MapWidth>=260.f && MapHeight>=170.f)",
            "if (!Hero->HasExploredAtlasCell(Zone, Column, Row)) continue;",
        ):
            with self.subTest(icon=token):
                self.assertIn(token, HUD)

    def test_editable_grimoire_armory_design_sheet_has_nine_original_glyphs(self):
        source = ROOT / "MoonfallUnreal/Art/UI/moonfall_grimoire_armory_atlas.svg"
        self.assertTrue(source.is_file())
        svg = ET.parse(source).getroot()
        self.assertEqual(svg.tag, "{http://www.w3.org/2000/svg}svg")
        self.assertEqual(svg.attrib.get("viewBox"), "0 0 658 658")
        contents = source.read_text(encoding="utf-8")
        for title in ("EMBER BOLT", "FROST SHARD", "ECLIPSE LANCE",
                      "MENDING LIGHT", "STORM SPARK", "THORN DART",
                      "WEAPON", "ARMOR", "ACCESSORY"):
            with self.subTest(title=title):
                self.assertIn(title, contents)
        self.assertEqual(len(svg.findall("{http://www.w3.org/2000/svg}g")), 9)
        self.assertNotIn("<script", contents)
        self.assertNotIn("<image", contents)

    def test_six_real_biome_seals_in_atlas_and_quest_without_fog_leaks(self):
        emblem = HUD.split("const auto DrawRegionEmblem = [this]", 1)[1].split(
            "const auto DrawSelectionRail = [this]", 1
        )[0]
        self.assertEqual(HUD.count("const auto DrawRegionEmblem = [this]"), 1)
        for key in ("Region<0 || Region>=6", "R<5.f || R>24.f",
                    "FMath::IsFinite(X)", "FMath::IsFinite(Y)",
                    "constexpr int32 Edges=8", "switch (Region)"):
            self.assertIn(key, emblem)
        for n in range(5):
            self.assertIn(f"case {n}:", emblem)
        self.assertIn("default: // Eclipse Wastes", emblem)
        for forbidden in ("TActorIterator", "GetAllActorsOfClass",
                          "SaveWorldProgress", "BindAction(",
                          "HasDiscoveredDungeon", "HasExploredAtlasCell"):
            self.assertNotIn(forbidden, emblem)
        for token in (
            "DrawRegionEmblem(Left+AtlasSidebarWidth-25.f,Y+RowHeight*0.5f-3.f,",
            "if (AtlasSidebarWidth>=210.f && RowHeight>=33.f)",
            "9.f,Zone,AtlasZoneInk[Zone]);",
            "if (!bSanctuaryObjective)",
            "DrawRegionEmblem(Left+Width-16.f,Top+109.f,8.f,ClimateRegion,",
            "MoonfallLivingSimulation::RegionFromWorldX(Hero->GetActorLocation().X)",
            "if (!Hero->HasExploredAtlasCell(Zone, Column, Row)) continue;",
            "if (!bFound && !bCleared) continue;",
        ):
            self.assertIn(token, HUD)
        for token in (
            "const FLinearColor Etch(0.38f,0.42f,0.52f,0.54f);",
            "const float MidY=T+H*0.5f;",
            "Canvas->K2_DrawLine(FVector2D(L+3.f,MidY-13.f)",
        ):
            self.assertIn(token, HUD)

    def test_original_editable_six_biome_vector_asset(self):
        asset = ROOT / "MoonfallUnreal/Art/UI/moonfall_six_region_emblems.svg"
        self.assertTrue(asset.is_file())
        svg = ET.parse(asset).getroot()
        self.assertEqual(svg.tag, "{http://www.w3.org/2000/svg}svg")
        self.assertEqual(svg.attrib.get("viewBox"), "0 0 750 506")
        ns = "{http://www.w3.org/2000/svg}"
        # Count the six top-level region tiles, not nested glyph drawing groups.
        self.assertEqual(len([e for e in svg if e.tag == ns+"g"]), 6)
        body = asset.read_text(encoding="utf-8")
        for name in ("ASHEN WILDS", "MOONVEIL FOREST", "HOLLOW DEPTHS",
                     "IRON FOUNDRY", "FROSTBOUND REACH", "ECLIPSE WASTES"):
            self.assertIn(name, body)
        self.assertNotIn("<script", body)
        self.assertNotIn("<image", body)

    def test_five_distinct_real_faction_crests_and_npc_fallback(self):
        crest = HUD.split("const auto DrawFactionCrest = [this]", 1)[1].split(
            "const auto DrawSelectionRail = [this]", 1
        )[0]
        for token in ("Faction<0 || Faction>=5", "R<5.f || R>21.f",
                      "FMath::IsFinite(X)", "FMath::IsFinite(Y)",
                      "switch(Faction)", "default: // Ashen Clans"):
            self.assertIn(token, crest)
        for n in range(4):
            self.assertIn(f"case {n}:", crest)
        self.assertEqual(HUD.count("const auto DrawFactionCrest = [this]"), 1)
        for token in (
            "DrawFactionCrest(Left+PanelWidth-30.f,Y+13.f,9.f,FactionIndex,StandingColor);",
            "Resident->FactionId == FName(TEXT(\"Wanderers\"))",
            "Resident->FactionId == FName(TEXT(\"VerdantCircle\"))",
            "Resident->FactionId == FName(TEXT(\"StoneHearth\"))",
            "Resident->FactionId == FName(TEXT(\"MoonCoven\"))",
            "Resident->FactionId == FName(TEXT(\"AshenClans\"))",
            "if (ResidentFactionArt!=INDEX_NONE)",
            "DrawFactionCrest(L+W-67.f,T+75.f,10.f,ResidentFactionArt,",
            "else DrawMoonfallSigil(L+W-67.f,T+75.f,10.f,",
        ):
            self.assertIn(token, HUD)
        for forbidden in ("ChangeFactionReputation", "SaveWorldProgress",
                          "GetAllActorsOfClass", "TActorIterator",
                          "BindAction(", "EquipInventoryItem("):
            self.assertNotIn(forbidden, crest)

    def test_original_editable_five_faction_vector_sheet(self):
        asset = ROOT / "MoonfallUnreal/Art/UI/moonfall_five_faction_crests.svg"
        self.assertTrue(asset.is_file())
        svg = ET.parse(asset).getroot()
        self.assertEqual(svg.tag, "{http://www.w3.org/2000/svg}svg")
        self.assertEqual(svg.attrib.get("viewBox"), "0 0 1070 230")
        ns = "{http://www.w3.org/2000/svg}"
        self.assertEqual(len([e for e in svg if e.tag == ns+"g"]), 5)
        content = asset.read_text(encoding="utf-8")
        for name in ("WANDERERS", "VERDANT CIRCLE", "STONE HEARTH",
                     "MOON COVEN", "ASHEN CLANS"):
            self.assertIn(name, content)
        self.assertNotIn("<script", content)
        self.assertNotIn("<image", content)

    def test_tiny_viewports_have_safe_noninteractive_fallbacks(self):
        for token in (
            "!Hero->HasResidentContextPointerSurface(Canvas->SizeX,Canvas->SizeY)",
            "Canvas->SizeX < 264.f || Canvas->SizeY < 304.f",
            "Canvas->SizeX < 304.f || Canvas->SizeY < 304.f",
            "TEXT(\"NPC | Enlarge window | ESC / RMB: close\")",
            "TEXT(\"OPTIONS | Enlarge window | ESC to close\")",
            "TEXT(\"CHARACTER | Enlarge window | ESC: back\")",
            "TEXT(\"INVENTORY | Enlarge window | B / ESC: close\")",
        ):
            self.assertIn(token, HUD)
        self.assertIn("DrawLiveMenuHitCue();", HUD)

    def test_hidden_options_and_npc_pointer_actions_are_blocked(self):
        mouse = HERO.split("void AMoonfallHeroPawn::ResidentContextMouseClick()", 1)[1].split("\n}", 1)[0]
        options = mouse.split("if (bOptionsOpen) {", 1)[1].split("if (bInventoryOpen) {", 1)[0]
        self.assertIn("Width >= 264 && Height >= 304 && PC->bShowMouseCursor", options)
        self.assertIn("FMath::IsFinite(MouseX) && FMath::IsFinite(MouseY)", options)
        self.assertLess(options.index("Width >= 264"), options.index("AdjustOption("))
        npc = mouse.split("if (!bResidentContextOpen) { RightHandAction(); return; }", 1)[1]
        self.assertIn("!HasResidentContextPointerSurface(Width,Height) || !PC->bShowMouseCursor", npc)
        self.assertIn("!FMath::IsFinite(MouseX) || !FMath::IsFinite(MouseY)", npc)
        self.assertLess(npc.index("!HasResidentContextPointerSurface(Width,Height)"), npc.index("ChooseResidentContextAction("))

    def test_hidden_inventory_pointer_surface_rejects_clicks_and_drag(self):
        helper = HERO.split("bool HasVisibleInventoryPointerSurface(", 1)[1].split("\n}", 1)[0]
        self.assertIn("return W>=304 && H>=304;", helper)
        self.assertIn("!PC->bShowMouseCursor", helper)
        for method in ("CastRightMouse", "ResidentContextMouseClick"):
            body = HERO.split("void AMoonfallHeroPawn::" + method + "()", 1)[1].split("\n}", 1)[0]
            self.assertLess(body.index("if (!HasVisibleInventoryPointerSurface(Controller)) return;"),
                            body.index("HandleSpellBookPointer()"))
        drag = HERO.split("bool AMoonfallHeroPawn::ScrollInventoryToMouseTrack()", 1)[1].split("\n}", 1)[0]
        self.assertIn("if (!HasVisibleInventoryPointerSurface(Controller))", drag)
        self.assertIn("bInventoryScrollbarDragging = false;", drag)
        self.assertIn("Canvas->SizeX < 304.f || Canvas->SizeY < 304.f", HUD)

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

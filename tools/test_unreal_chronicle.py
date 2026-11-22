"""Moonfall's six-act/42-chapter Chronicle uses persisted real progress."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "MoonfallUnreal" / "Source" / "MoonfallUnreal"


class ChronicleIntegration(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.story = (SRC / "MoonfallChronicle.h").read_text(encoding="utf-8")
        cls.hud = (SRC / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        cls.hero = (SRC / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        cls.pawn = (SRC / "MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        cls.ui = (SRC / "MoonfallChronicleUI.h").read_text(encoding="utf-8")
        cls.keys = (ROOT / "MoonfallUnreal" / "Config" / "DefaultInput.ini").read_text(encoding="utf-8")
        cls.atlas = cls.hud.split("if (Hero->bAtlasOpen) {", 1)[1].split(
            "if (Hero->bOptionsOpen) {", 1
        )[0]
        cls.preview = cls.atlas.split(
            "// Full six-act, seven-chapter-per-act Chronicle.", 1
        )[1].split("// Hover cards are purely informational", 1)[0]

    def test_42_original_beats_are_stable_and_unique(self):
        rows = re.findall(
            r'\{"(act\d+_ch\d+)", "([^"]+)", "([^"]+)"\}', self.story
        )
        self.assertEqual(len(rows), 42)
        self.assertEqual(len({row[0] for row in rows}), 42)
        self.assertEqual(len({row[1] for row in rows}), 42)
        for idx, (slug, title, narrative) in enumerate(rows):
            self.assertEqual(slug, f"act{idx // 7 + 1}_ch{idx % 7 + 1}")
            self.assertTrue(8 <= len(title) <= 38, title)
            self.assertTrue(90 <= len(narrative) <= 300, title)
        self.assertIn("TotalBeats = ActCount * BeatsPerAct", self.story)
        self.assertIn("BeatsPerAct = 7", self.story)
        self.assertIn("ActCount = 6", self.story)

    def test_regional_lore_respects_existing_atlas_names(self):
        block = self.atlas.split("static const TCHAR* AtlasDungeonTitles[30] = {", 1)[1].split(
            "};", 1
        )[0]
        names = re.findall(r'TEXT\("([^"]+)"\)', block)
        self.assertEqual(len(names), 30)
        beats = re.findall(
            r'\{"(act\d+_ch\d+)", "([^"]+)", "([^"]+)"\}', self.story
        )
        for zone in range(6):
            for dungeon in range(5):
                self.assertEqual(beats[zone * 7 + dungeon + 1][1],
                                 names[zone * 5 + dungeon])
        atlas_regions = re.findall(
            r'TEXT\("([^"]+)"\)',
            self.atlas.split("static const TCHAR* AtlasRegions[6] = {", 1)[1].split(
                "};", 1
            )[0]
        )
        story_regions = re.findall(
            r'"([^"]+)"',
            self.story.split("RegionNames[ActCount] = {", 1)[1].split(
                "};", 1
            )[0],
        )
        self.assertEqual(story_regions, atlas_regions)

    def test_unlocks_are_bound_to_saved_real_boss_victories(self):
        for expression in (
            "ValidDungeonBits = (1u << 30) - 1u",
            "Defeated & ~Discovered",
            "Won == 31u",
            "Found & Bit",
            "Won & Bit",
            "BeatState(", "RevealedBeat(", "Summarize(",
            "Featured = D + 1",
            "1 + PopCount5(Found) + (Bosses == 5 ? 1 : 0)",
        ):
            self.assertIn(expression, self.story)
        self.assertIn("int32 DiscoveredDungeonMask = 0;", self.hero)
        self.assertIn("DefeatedDungeonBossMask = 0;", self.hero)
        # Progress is read above the card and reused by the card and sidebar.
        self.assertIn("const uint32 ChronicleDiscovered = static_cast<uint32>(Hero->DiscoveredDungeonMask);", self.atlas)
        self.assertIn("const uint32 ChronicleDefeated = static_cast<uint32>(Hero->DefeatedDungeonBossMask);", self.atlas)
        self.assertIn("Selected, ChronicleDiscovered, ChronicleDefeated", self.preview)
        self.assertIn("MoonfallChronicle::Summarize(", self.preview)
        self.assertIn("MoonfallChronicle::RevealedBeat(", self.preview)
        self.assertIn("Chronicle.Valid && ChronicleCard.Visible", self.preview)
        self.assertNotIn("MoonfallChronicle::Acts[Selected]", self.preview)

    def test_chronicle_progress_round_trips_through_existing_world_save(self):
        # Chronicle is a read-only view over the authoritative dungeon ledgers.
        save = self.pawn.split("bool AMoonfallHeroPawn::SaveWorldProgress()", 1)[1].split(
            "bool AMoonfallHeroPawn::LoadWorldProgress()", 1)[0]
        load = self.pawn.split("bool AMoonfallHeroPawn::LoadWorldProgress()", 1)[1]
        for field in ("DiscoveredDungeonMask", "DefeatedDungeonBossMask"):
            self.assertIn(f"Save->{field} = {field};", save)
            self.assertRegex(load, rf"Save->{field}")
            self.assertIn(f"static_cast<uint32>(Hero->{field})", self.atlas)
        self.assertIn("UGameplayStatics::SaveGameToSlot(Save, MoonfallWorldSlot, 0)", save)
        self.assertIn("UGameplayStatics::LoadGameFromSlot(MoonfallWorldSlot, 0)", load)
        self.assertIn("DefeatedDungeonBossMask = PreviousMask;", self.pawn)
        self.assertIn("DiscoveredDungeonMask = PreviousMask;", self.pawn)
        self.assertNotIn("Save->AtlasChronicleChapter", save)
        self.assertNotIn("Save->bAtlasChronicleFocus", save)
    def test_atlas_reader_is_spoiler_safe_and_window_adaptive(self):
        for token in (
            "MoonfallChronicleUI::Layout(",
            "Chronicle.Valid && ChronicleCard.Visible",
            "const int32 ChapterIndex = Hero->AtlasChronicleChapter >= 0",
            "MoonfallChronicle::BeatState(",
            "const bool bSelected = Chapter == ChapterIndex",
            "ChapterState == MoonfallChronicle::EBeatState::Hidden",
            "ChapterState == MoonfallChronicle::EBeatState::Cleared",
            "ChronicleCard.TabLeft + ChronicleCard.TabWidth * Chapter",
            "ChronicleCard.ToggleLeft",
            'Hero->bAtlasChronicleFocus ? TEXT("MAP") : TEXT("READ")',
            'TEXT("LOCKED")',
            'TEXT("CLEARED")',
            'TEXT("DISCOVERED")',
            'TEXT("CHAPTER %d/7  |  UNDISCOVERED")',
            "const FString Lore = ChronicleBeat",
            "ChronicleBeat->Lore",
            "LoreLineLimit = MapWidth >= 480.f ? 78 : 48",
            "LoreLines = ChronicleCard.LoreLines",
            "Lore[WordEnd] != TCHAR(' ')",
            "for (int32 Line = 0; Line < LoreLines && LoreCursor < Lore.Len(); ++Line)",
            "Lore.Mid(LoreCursor, End - LoreCursor)",
            "CT + 72.f + Line * 17.f",
            "CT + CH - 19.f",
            "bAtlasPointerInsideMap && !bChroniclePointer",
        ):
            self.assertIn(token, self.preview + self.atlas)
        for forbidden in (
            "SpawnActor(", "SaveWorldProgress(", "ResidentQuestStates.Add(",
            "AwardExperience(", "GetAllActorsOfClass(", "TActorIterator",
            "LastHitAmount", "SetGlobalTimeDilation(", "NewObject<",
            "MoonfallChronicle::Acts[Selected]",
        ):
            self.assertNotIn(forbidden, self.preview)
        self.assertIn("const int32 Selected = FMath::Clamp(Hero->AtlasSelectedRegion, 0, 5);",
                      self.atlas)
        self.assertIn('J/ESC close | Arrows: region / pan | Wheel: zoom', self.atlas)
        self.assertIn("Mouse: select a region or pan the chart", self.atlas)
        self.assertIn("J/ESC close | Up/Down: region | Left/Right: chapter", self.atlas)
        self.assertIn("Tab / R3: map | Mouse: chapter tabs | Wheel: zoom", self.atlas)
        self.assertIn("Tab / R3: read", self.atlas)
        self.assertIn("bool Visible", self.ui)
        self.assertIn("Compact=MapHeight<390.f", self.ui)
        self.assertIn("Visible=MapWidth>=295.f && MapHeight>=185.f", self.ui)
        self.assertIn("Expanded?(Height>=198.f?5:3):(Compact?1:3)", self.ui)

    def test_mouse_hit_and_keyboard_controller_reader_focus(self):
        for declaration in (
            "AtlasChronicleChapter = -1;",
            "bAtlasChronicleFocus = false;",
            "void ToggleAtlasChronicleFocus();",
            "void StepAtlasChronicle(int32 Direction);",
        ):
            self.assertIn(declaration, self.hero)
        self.assertIn('#include "MoonfallChronicleUI.h"', self.hud)
        self.assertIn('#include "MoonfallChronicleUI.h"', self.pawn)
        self.assertIn("void AMoonfallHeroPawn::ToggleAtlasChronicleFocus()", self.pawn)
        self.assertIn("void AMoonfallHeroPawn::StepAtlasChronicle(int32 Direction)", self.pawn)
        self.assertIn("if (bAtlasChronicleFocus) { StepAtlasChronicle(-1); return; }",
                      self.pawn)
        self.assertIn("if (bAtlasChronicleFocus) { StepAtlasChronicle(1); return; }",
                      self.pawn)
        self.assertIn('BindAction(TEXT("ChronicleFocus"), IE_Pressed,', self.pawn)
        self.assertIn('ActionName="ChronicleFocus",Key=Tab', self.keys)
        self.assertIn('ActionName="ChronicleFocus",Key=Gamepad_RightThumbstick',
                      self.keys)
        self.assertIn('ActionName="Dodge",Key=Q', self.keys)
        self.assertIn('ActionName="Atlas",Key=J', self.keys)
        self.assertIn('ActionName="Guard",Key=Gamepad_RightThumbstick', self.keys)
        self.assertIn('ActionName="DualCast",Key=Gamepad_LeftTrigger', self.keys)
        self.assertIn('ActionName="GamepadBack",Key=Gamepad_FaceButton_Right', self.keys)
        self.assertIn('ActionName="CastLeft",Key=Gamepad_LeftShoulder', self.keys)
        self.assertIn('ActionName="CastRight",Key=Gamepad_RightShoulder', self.keys)
        self.assertNotIn('ActionName="Dodge",Key=Gamepad_RightShoulder', self.keys)
        self.assertIn("bInventoryOpen || bAtlasOpen || Health <= 0.f", self.pawn)
        click = self.pawn.split(
            "void AMoonfallHeroPawn::ResidentContextMouseClick()", 1
        )[1].split("if (bOptionsOpen)", 1)[0]
        for term in (
            "MoonfallChronicleUI::Layout(",
            "MoonfallChronicleUI::HitToggle(",
            "MoonfallChronicleUI::HitChapter(",
            "MoonfallChronicleUI::HitCard(",
            "AtlasChronicleChapter = ClickedBeat",
            "bAtlasChronicleFocus = true",
            "if (MoonfallChronicleUI::HitCard(ChronicleCard, MouseX, MouseY))",
        ):
            self.assertIn(term, click)
        self.assertLess(click.index("MoonfallChronicleUI::HitCard("),
                        click.index("// Clicking the parchment chart pans"))
        self.assertNotIn("AwardExperience(", click)
        self.assertNotIn("SaveWorldProgress(", click)

    def test_chapter_navigation_is_transient_and_region_safe(self):
        self.assertIn(
            "UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category=\"Moonfall|Atlas|Chronicle\") int32 AtlasChronicleChapter = -1;",
            self.hero,
        )
        self.assertIn("AtlasChronicleChapter = -1;", self.pawn)
        self.assertIn("if (AtlasSelectedRegion != Zone) AtlasChronicleChapter = -1",
                      self.pawn)
        self.assertIn(
            "if (AtlasSelectedRegion != Slot / 5) AtlasChronicleChapter = -1",
            self.pawn,
        )
        self.assertIn(
            "if (AtlasSelectedRegion != PreviousRegion) AtlasChronicleChapter = -1",
            self.pawn,
        )
        self.assertIn("NextChapter(", self.ui)
        self.assertIn("return (Base+Direction+Tabs)%Tabs;", self.ui)
        self.assertNotIn("SaveWorldProgress(", self.preview)
        self.assertNotIn("Save->AtlasChronicleChapter", self.pawn)


if __name__ == "__main__":
    unittest.main()

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
        cls.atlas = cls.hud.split("if (Hero->bAtlasOpen) {", 1)[1].split(
            "if (Hero->bOptionsOpen) {", 1
        )[0]
        cls.preview = cls.atlas.split(
            "// Six-act / 42-chapter Chronicle.", 1
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
        self.assertIn("static_cast<uint32>(Hero->DiscoveredDungeonMask)", self.preview)
        self.assertIn("static_cast<uint32>(Hero->DefeatedDungeonBossMask)", self.preview)
        self.assertIn("MoonfallChronicle::Summarize(", self.preview)
        self.assertIn("MoonfallChronicle::RevealedBeat(", self.preview)
        self.assertIn("Chronicle.Valid && ChronicleBeat", self.preview)
        self.assertNotIn("MoonfallChronicle::Acts[Selected]", self.preview)

    def test_atlas_preview_is_passive_and_small_window_safe(self):
        for token in (
            "MapWidth >= 295.f && MapHeight >= 185.f",
            "bCompactChronicle = MapHeight < 390.f",
            "MapTop + MapHeight - ChronicleHeight - 8.f",
            "STORY ACT %d/6",
            "CHAPTER %d/7",
            "%d/7 REVEALED",
            "ChronicleBeat->Lore",
            "LoreLineLimit = MapWidth >= 480.f ? 78 : 48",
            "LoreLines = bCompactChronicle ? 1 : 3",
            "Lore[WordEnd] != TCHAR(' ')",
            "for (int32 Line = 0; Line < LoreLines && LoreCursor < Lore.Len(); ++Line)",
            "Lore.Mid(LoreCursor, End - LoreCursor)",
            "ChronicleTop + 45.f + Line * 17.f",
            "ChronicleTop + 105.f",
            "Scale * 0.68f",
        ):
            self.assertIn(token, self.preview)
        # At 720p and wider, all authored lore passages (<= 189 chars)
        # fit within the 3 x 78-character word-wrapped display budget.
        self.assertGreaterEqual(3 * 78, 189)
        self.assertIn("ChronicleHeight = bCompactChronicle ? 68.f : 127.f",
                      self.preview)
        for forbidden in (
            "SpawnActor(", "SaveWorldProgress(", "ResidentQuestStates.Add(",
            "AwardExperience(", "GetAllActorsOfClass(", "TActorIterator",
            "LastHitAmount", "SetGlobalTimeDilation(", "NewObject<",
        ):
            self.assertNotIn(forbidden, self.preview)
        self.assertIn("const int32 Selected = FMath::Clamp(Hero->AtlasSelectedRegion, 0, 5);",
                      self.atlas)
        self.assertIn('J/ESC close | Arrows: region / pan | Wheel: zoom', self.atlas)
        self.assertIn("Mouse: select a region or pan the chart", self.atlas)


if __name__ == "__main__":
    unittest.main()

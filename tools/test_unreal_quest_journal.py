"""Source regression for active-contract world tracker and saved NPC catalog."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "MoonfallUnreal" / "Source" / "MoonfallUnreal"

class WorldQuestJournal(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.hud = (SRC / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        cls.hero = (SRC / "MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        cls.arena = (SRC / "MoonfallArena.cpp").read_text(encoding="utf-8")
        cls.catalog = (SRC / "MoonfallQuestJournal.h").read_text(encoding="utf-8")
        cls.layout = (SRC / "MoonfallQuestLayout.h").read_text(encoding="utf-8")
        cls.npc = (SRC / "MoonfallWorldNPC.cpp").read_text(encoding="utf-8")

    def test_persisted_contracts_prioritized_without_world_scanning(self):
        section = self.hud.split("// World-wide quest tracker", 1)[1].split(
            "// Compact circular resource HUD", 1)[0]
        for text in ("QuestResidentIds", "QuestSummary", "QuestSummary.TrackedIndex >= 0",
                     "MoonfallQuestJournal::Summarize(", "ResidentQuestStates.Find(QuestResidentIds[I])",
                     "MoonfallQuestJournal::Cost(Index)", "WORLD QUEST TRACKER  |  CONTRACTS",
                     "WORLD QUEST TRACKER  |  EXPLORATION", "FMath::Min(Hero->Salvage, Cost)",
                     "QuestSummary.Active", "QuestSummary.Completed"):
            self.assertIn(text, section)
        self.assertNotIn("TActorIterator", section)
        self.assertNotIn("SpawnActor", section)
        self.assertNotIn("ResidentQuestStates.Add", section)
        self.assertNotIn("SaveWorldProgress", section)
        self.assertIn("for (int32 I = 0; I < MoonfallQuestJournal::ResidentCount; ++I)", section)
        self.assertIn("static const TArray<FName> QuestResidentIds = []()", self.hud)
        self.assertEqual(self.hud.count("static const TArray<FName> QuestResidentIds = []()"), 1)
        # The same cached FNames identify NPC dialogue and persisted world
        # tracking, rather than constructing separate catalogs per frame.
        self.assertIn("QuestResidentIds.Find(Resident->ResidentId)", self.hud)
        self.assertIn("ResidentQuestStates.Find(QuestResidentIds[I])", section)
        # Accepted quests stay visible in the sanctuary; Firstlight tutorial
        # only takes over when there are no active contracts.
        self.assertIn("if (!bThreeQuestList && bSanctuaryObjective)", section)
        self.assertLess(section.index("if (!bThreeQuestList && bSanctuaryObjective)"),
                        section.index("else if (!bThreeQuestList && Cleared >= 30)"))
        self.assertIn("else if (!bThreeQuestList && Discovered < 30)", section)
        self.assertIn("else if (!bThreeQuestList) {", section)
        self.assertIn("const bool bThreeQuestList = QuestSummary.TrackedCount > 0;", section)
        self.assertIn("bSanctuaryObjective && !bThreeQuestList", section)
        self.assertNotIn("!bSanctuaryObjective && QuestSummary.TrackedCount > 0", section)
        # Tracker includes the same XP forecast as the resident dialog.
        self.assertIn("MoonfallQuestXP::Preview(", section)
        self.assertIn("Cost, Hero->Level, Hero->Experience, Hero->ExperienceToNextLevel", section)
        self.assertIn("RowXP.RewardXP", section)
        self.assertIn("RowXP.LevelsGained > 0", section)
        self.assertIn("MAX LEVEL", section)
        self.assertIn("%d ACTIVE | %d DONE", section)
        self.assertNotIn("const FString XPLabel", section)
        self.assertEqual(section.count("MoonfallQuestXP::Preview("), 1)
        # No duplicate reward forecast, menu strings, or dungeon candidate
        # scans when a saved contract is active.
        self.assertIn("if (!bThreeQuestList && !bSanctuaryObjective)", section)
        self.assertLess(section.index("if (!bThreeQuestList && !bSanctuaryObjective)"),
                        section.index("for (int32 Zone = 0; Zone < 6 && NextBossZone == INDEX_NONE; ++Zone)"))
        self.assertLess(section.index("const bool bThreeQuestList = QuestSummary.TrackedCount > 0;"),
                        section.index("if (!bThreeQuestList && !bSanctuaryObjective)"))
        # Status remains readable without flashes or any above-character text.
        self.assertIn("const bool bTrackingContract", section)
        self.assertIn("const bool bReadyToTurnIn", section)
        self.assertIn("const FLinearColor TrackerAccent", section)
        self.assertIn("DrawRect(TrackerAccent, Left, Top, 4.f, TrackerHeight)", section)
        self.assertIn("MoonfallQuestLayout::Compute(", section)
        self.assertIn("const MoonfallQuestLayout::FMetrics QuestLayout", section)
        self.assertIn("const float TrackerHeight = static_cast<float>(QuestLayout.PanelHeight)", section)
        self.assertIn("const float Top = static_cast<float>(QuestLayout.Top)", section)
        self.assertIn("QuestSummary.TrackedCount > 0", section)
        self.assertIn("QuestLayout.PanelHeight", section)
        self.assertIn("for (int32 Row = 0; Row < QuestSummary.TrackedCount; ++Row)", section)
        self.assertIn("QuestSummary.TrackedIndices[Row]", section)
        self.assertIn("const float RowTop = Top + 28.f + Row * QuestLayout.RowStride", section)
        self.assertIn("if (bThreeQuestList)", section)
        self.assertIn("RowXP.LevelsGained > 0", section)
        self.assertIn("UTF8_TO_TCHAR(Entry.QuestTitle)", section)
        self.assertIn("UTF8_TO_TCHAR(Entry.Name)", section)
        self.assertIn("UTF8_TO_TCHAR(Entry.Location)", section)
        self.assertIn('FString::Printf(TEXT("%s @ %s")', section)
        self.assertIn('FString::Printf(TEXT("%d/%d SALVAGE")', section)
        self.assertIn('DrawText(XPText, FLinearColor', section)
        self.assertIn("Left + Width - 166.f", section)
        self.assertIn("Left + Width - 81.f", section)
        self.assertIn("Left + 12.f, RowTop, Width - 24.f, QuestLayout.RowHeight", section)
        self.assertNotIn('%s @ %s | %d/%d SALVAGE | %s', section)
        self.assertIn("bReady ? TEXT(\"TURN IN\") : TEXT(\"ACTIVE\")", section)
        self.assertIn("Top + TrackerHeight - 43.f", section)
        self.assertIn("Top + TrackerHeight - 32.f", section)
        self.assertIn("Top + TrackerHeight - 22.f", section)
        self.assertIn("const float SalvageProgress = FMath::Clamp(", section)
        self.assertIn("static_cast<float>(Hero->Salvage) / FMath::Max(1, Cost)", section)
        self.assertIn("ProgressWidth * SalvageProgress, 3.f", section)
        self.assertIn("RowTop + QuestLayout.ProgressOffset", section)
        self.assertIn("FMath::Min(Hero->Salvage, Cost)", section)
        # Three independent text lines keep title/action, NPC/location,
        # and salvage/XP separate even on 600px-wide windows.
        self.assertIn("RowTop + QuestLayout.TitleOffset", section)
        self.assertIn("RowTop + QuestLayout.ResidentOffset", section)
        self.assertIn("RowTop + QuestLayout.RewardOffset", section)
        self.assertIn("RowTop + QuestLayout.ProgressOffset", section)
        self.assertIn("const FLinearColor RowAccent = bReady", section)
        self.assertNotIn("ProjectWorldLocationToScreen", section)
        self.assertIn("GetTotalDiscoveredDungeons()", section)
        self.assertIn("GetTotalDefeatedDungeonBosses()", section)

    def test_compact_window_keeps_all_three_quests_and_health_orb_separate(self):
        section = self.hud.split("// World-wide quest tracker", 1)[1].split(
            "// Compact circular resource HUD", 1)[0]
        self.assertIn(
            "Canvas->SizeX >= 600.f && Canvas->SizeY >= 420.f", section
        )
        self.assertIn(
            "const float Top = static_cast<float>(QuestLayout.Top);", section
        )
        self.assertIn(
            "QuestLayout.PanelHeight", section
        )
        self.assertIn(
            "for (int32 Row = 0; Row < QuestSummary.TrackedCount; ++Row)", section
        )
        self.assertIn("bCompact = ViewHeight < 520", self.layout)
        self.assertIn("Stride = bCompact ? 60 : 70", self.layout)
        self.assertIn("Rows > 0 ? 85 + Stride * Rows : 124", self.layout)
        self.assertIn("ValidRowSpacing(", self.layout)
        self.assertIn("ClearsHealthOrb(", self.layout)
        # The fixed right-hand reward and status columns leave ample
        # space for bounded title and destination labels in compact mode.
        for width in (600, 640, 760, 1280, 1920):
            panel_width = min(560, width - 40)
            self.assertGreaterEqual(panel_width - 166, 350)
            self.assertGreaterEqual(panel_width - 81, 435)
        # These are policy checks against the existing orb's fixed placement,
        # not claims of an actual Unreal-rendered screenshot or hit test.
        for width, height in ((600, 420), (640, 480), (760, 520),
                              (1280, 720), (1920, 1080), (3840, 2160)):
            top = 18 if height < 520 else 78
            stride = 60 if height < 520 else 70
            radius = max(50, min(80, min(width, height) * 0.092))
            orb_top = height - 2 * radius - 25
            for quest_count in (1, 2, 3):
                tracker_bottom = top + 85 + stride * quest_count
                with self.subTest(width=width, height=height,
                                  quest_count=quest_count):
                    self.assertLess(tracker_bottom + 8, orb_top)
                    self.assertLess(tracker_bottom, height)
                    title_y, resident_y, reward_y, bar_y = (
                        (4, 22, 40, 52) if height < 520 else (6, 28, 49, 62)
                    )
                    self.assertGreaterEqual(resident_y - title_y, 18)
                    self.assertGreaterEqual(reward_y - resident_y, 18)
                    self.assertGreaterEqual(bar_y - reward_y, 12)
                    self.assertLessEqual(bar_y + 3, stride - 4)


    def test_npc_dialogue_matches_world_quest_title(self):
        panel = self.hud.split("// Native Canvas resident menu.", 1)[1].split(
            "if (Hero->bAtlasOpen)", 1)[0]
        self.assertIn("QuestResidentIds.Find(Resident->ResidentId)", panel)
        self.assertIn("ResidentCatalogIndex != INDEX_NONE", panel)
        self.assertIn(
            "MoonfallQuestJournal::Residents[ResidentCatalogIndex].QuestTitle", panel
        )
        self.assertIn('FString::Printf(TEXT("Quest: %s"), *ContractTitle)', panel)
        self.assertIn("T + 37.f", panel)
        self.assertIn("W < 355.f ? 0.60f : 0.72f", panel)
        # The existing faction, bond, contract rank and reward preview must
        # remain usable rather than being replaced by the new quest title.
        for expected in ("GetSocialDisposition(Hero)", "Trust %d",
                         "MoonfallQuestXP::RankName(", "MoonfallQuestXP::Preview(",
                         "FText", "Salvage"):
            if expected != "FText":
                self.assertIn(expected, panel)

    def test_catalog_matches_authored_crossroads_and_all_persisted_ids(self):
        entries = re.findall(
            r'\{"([^"]+)", "([^"]+)", "([^"]+)", "([^"]+)", (\d+)\}',
            self.catalog,
        )
        self.assertEqual(len(entries), 25)
        known = set()
        titles = set()
        for ident, name, title, region, race in entries:
            self.assertNotIn(ident, known)
            self.assertNotIn(title, titles)
            self.assertTrue(name and title and region)
            self.assertLessEqual(len(title), 26)
            known.add(ident)
            titles.add(title)
            self.assertTrue(0 <= int(race) <= 11)
            self.assertEqual(self.hero.count('FName(TEXT("' + ident + '"))'), 4, ident)
        self.assertIn("CrossroadsGuide", known)
        # The Guide is a cost-10 exception even though its race is Wood Elf.
        self.assertIn('return ResidentId == FName(TEXT("CrossroadsGuide")) ? 10', self.npc)
        self.assertIn("10 + (MoonfallRaces::Validated(RaceId) % 4) * 5", self.npc)
        authored = self.arena.split("const FResident Residents[] = {", 1)[1].split("\n };", 1)[0]
        authored_rows = re.findall(r'\{TEXT\("([^"]+)"\), (\d+), TEXT\(', authored)
        self.assertEqual(len(authored_rows), 12)
        catalog = {key:(name,int(race)) for key,name,title,region,race in entries}
        for name, race in authored_rows:
            ident = name.replace(" ", "")
            self.assertEqual(catalog[ident], (name,int(race)))
        self.assertEqual(catalog["CrossroadsGuide"], ("Wandering Guide",2))
        self.assertIn('Guide->RaceId = 2;', self.arena)

    def test_unwanted_enemy_instruction_overlay_not_added(self):
        journal_section = self.hud.split("// World-wide quest tracker", 1)[1].split(
            "// Compact circular resource HUD", 1)[0]
        self.assertNotIn("GUARD / PARRY / DODGE", journal_section)
        self.assertNotIn("GetAttackWarningRemaining", journal_section)

if __name__ == "__main__":
    unittest.main()

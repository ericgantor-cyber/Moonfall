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
        self.assertIn("static const TArray<FName> QuestResidentIds = []()", section)
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
        self.assertIn("QuestSummary.TrackedCount > 0", section)
        self.assertIn("85.f + 49.f * QuestSummary.TrackedCount : 124.f", section)
        self.assertIn("for (int32 Row = 0; Row < QuestSummary.TrackedCount; ++Row)", section)
        self.assertIn("QuestSummary.TrackedIndices[Row]", section)
        self.assertIn("const float RowTop = Top + 28.f + Row * 49.f", section)
        self.assertIn("if (bThreeQuestList)", section)
        self.assertIn("RowXP.LevelsGained > 0", section)
        self.assertIn("UTF8_TO_TCHAR(Entry.QuestTitle)", section)
        self.assertIn("UTF8_TO_TCHAR(Entry.Name)", section)
        self.assertIn("UTF8_TO_TCHAR(Entry.Location)", section)
        self.assertIn('%s @ %s | %d/%d SALVAGE | %s', section)
        self.assertIn("bReady ? TEXT(\"TURN IN\") : TEXT(\"ACTIVE\")", section)
        self.assertIn("Top + TrackerHeight - 43.f", section)
        self.assertIn("Top + TrackerHeight - 32.f", section)
        self.assertIn("Top + TrackerHeight - 22.f", section)
        self.assertIn("const float SalvageProgress = FMath::Clamp(", section)
        self.assertIn("static_cast<float>(Hero->Salvage) / FMath::Max(1, Cost)", section)
        self.assertIn("ProgressWidth * SalvageProgress, 2.f", section)
        self.assertIn("RowTop + 40.f", section)
        self.assertIn("FMath::Min(Hero->Salvage, Cost)", section)
        self.assertNotIn("ProjectWorldLocationToScreen", section)
        self.assertIn("GetTotalDiscoveredDungeons()", section)
        self.assertIn("GetTotalDefeatedDungeonBosses()", section)

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

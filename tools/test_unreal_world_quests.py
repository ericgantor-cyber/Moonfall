"""Verify twelve Moonfall regional quests use existing saved dungeon progress."""
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "MoonfallUnreal" / "Source" / "MoonfallUnreal"


class RegionalWorldQuests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.hud = (SOURCE / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        cls.header = (SOURCE / "MoonfallWorldQuest.h").read_text(encoding="utf-8")
        cls.hero = (SOURCE / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        cls.input = (ROOT / "MoonfallUnreal" / "Config" / "DefaultInput.ini").read_text(
            encoding="utf-8"
        )
        cls.section = cls.hud.split("// World-wide quest tracker", 1)[1].split(
            "// Compact circular resource HUD", 1
        )[0]

    def test_twelve_original_named_regional_goals(self):
        block = self.header.split("GoalTitles[GoalCount] = {", 1)[1].split("};", 1)[0]
        names = re.findall(r'"([^"]+)"', block)
        self.assertEqual(len(names), 12)
        self.assertEqual(len(names), len(set(names)))
        self.assertTrue(all(6 <= len(name) <= 30 for name in names))
        regions = re.findall(
            r'"([^"]+)"', self.header.split("RegionNames[ZoneCount] = {", 1)[1].split(
                "};", 1
            )[0]
        )
        self.assertEqual(len(regions), 6)
        self.assertEqual(len(set(regions)), 6)
        self.assertIn("GoalCount = ZoneCount * GoalsPerZone", self.header)
        self.assertIn("GoalCount == 12", self.header)
        self.assertIn("Progress == 5", self.header)
        self.assertIn("Stage == 1 && !Touched", self.header)
        self.assertIn("Out.Indices[Out.Count++] = Goal", self.header)
        self.assertIn("return Out", self.header)

    def test_regional_goals_fill_only_free_slots(self):
        section = self.section
        for expected in (
            "#include \"MoonfallWorldQuest.h\"",
            "Hero->GetDiscoveredDungeonCount(Zone)",
            "Hero->GetDefeatedDungeonBossCount(Zone)",
            "MoonfallWorldQuest::Select(ZoneDiscovered, ZoneDefeated,",
            "MoonfallQuestJournal::MaxTracked - QuestSummary.TrackedCount",
            "const int32 VisibleQuestCount = QuestSummary.TrackedCount",
            "bThreeQuestList ? VisibleQuestCount : 0",
            "for (int32 GoalRow = 0; GoalRow < RegionalGoals.Count; ++GoalRow)",
            "const int32 Row = QuestSummary.TrackedCount + GoalRow",
            "GoalTitles[Goal]", "RegionNames[Goal / 2]",
            "MoonfallWorldQuest::Progress(", "MoonfallWorldQuest::Type(Goal)",
            "WORLD QUEST TRACKER  |  REGIONAL QUESTS", "WORLD GOAL",
            "ProgressWidth * (static_cast<float>(Progress) / 5.f)",
        ):
            if expected.startswith("#include"):
                self.assertIn(expected, self.hud)
            else:
                self.assertIn(expected, section)
        self.assertIn("QuestSummary.TrackedCount < MoonfallQuestJournal::MaxTracked", section)
        self.assertIn("!bSanctuaryObjective || QuestSummary.TrackedCount > 0", section)
        self.assertIn("if (!bThreeQuestList && bSanctuaryObjective)", section)
        self.assertNotIn("TActorIterator", section)
        self.assertNotIn("GetAllActorsOfClass", section)
        # New world goals do not mutate saves or create unearned XP.
        start = section.index("// Any unused slot shows a named regional quest")
        end = section.index("} else {\n   DrawText(Objective", start)
        renderer = section[start:end]
        for banned in ("AwardExperience(", "SaveWorldProgress(", "ResidentQuestStates.Add(",
                       "SpawnActor", "MoonfallQuestXP::Reward("):
            self.assertNotIn(banned, renderer)

    def test_dungeon_progress_source_and_keymapping_preserved(self):
        self.assertIn("int32 DiscoveredDungeonMask = 0;", self.hero)
        self.assertIn("int32 DefeatedDungeonBossMask = 0;", self.hero)
        self.assertIn("GetDiscoveredDungeonCount(int32 Zone)", self.hero)
        self.assertIn("GetDefeatedDungeonBossCount(int32 Zone)", self.hero)
        self.assertIn('ActionName="Dodge",Key=Q', self.input)
        self.assertIn('ActionName="Atlas",Key=J', self.input)
        self.assertIn('ActionName="Inventory",Key=B', self.input)
        self.assertIn('ActionName="CharacterStats",Key=C', self.input)
        self.assertNotIn('ActionName="QuestJournal",Key=Q', self.input)


if __name__ == "__main__":
    unittest.main()

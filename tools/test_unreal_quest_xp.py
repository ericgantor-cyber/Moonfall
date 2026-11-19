"""Source-level regression for atomic, once-only resident quest XP."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1] / "MoonfallUnreal" / "Source" / "MoonfallUnreal"

class QuestXPSourceRegression(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.hero = (ROOT / "MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        cls.npc = (ROOT / "MoonfallWorldNPC.cpp").read_text(encoding="utf-8")
        cls.save = (ROOT / "MoonfallPersistentSave.h").read_text(encoding="utf-8")

    def test_uses_shared_reward_policy_and_existing_leveling(self):
        self.assertIn('#include "MoonfallQuestXP.h"', self.hero)
        method = self.hero.split("bool AMoonfallHeroPawn::CompleteResidentQuest(",1)[1]
        self.assertIn("MoonfallQuestXP::Reward(Cost, Level)",method)
        self.assertIn("if (RewardXP > 0) AwardExperience(RewardXP);",method)
        self.assertIn("void AMoonfallHeroPawn::AwardExperience(int32 Amount)",self.hero)
        self.assertIn("bSucceeded = Resident->TurnInResidentQuest(this);",self.hero)
        self.assertIn("Contract complete. +%d XP",self.hero)

    def test_no_double_reward_and_failed_save_undoes_xp_and_levels(self):
        method = self.hero.split("bool AMoonfallHeroPawn::CompleteResidentQuest(",1)[1]
        self.assertIn("GetResidentQuestState(Resident->ResidentId) != 1",method)
        self.assertLess(method.index("GetResidentQuestState(Resident->ResidentId) != 1"),
                        method.index("AwardExperience(RewardXP)"))
        self.assertIn("bDeferQuestLevelHealthNotification = true;",method)
        self.assertIn("bDeferQuestLevelHealthNotification = false;",method)
        self.assertIn("if (Level > PreviousLevel) OnHealthChanged(Health, MaxHealth);",method)
        self.assertLess(method.index("if (!SaveWorldProgress())"),
                        method.index("if (Level > PreviousLevel) OnHealthChanged"))
        self.assertIn("if (!bDeferQuestLevelHealthNotification) OnHealthChanged(Health, MaxHealth);", self.hero)
        self.assertIn("ResidentQuestStates.Add(Id, 2);",method)
        self.assertLess(method.index("AwardExperience(RewardXP)"),
                        method.index("if (!SaveWorldProgress())"))
        failed = method.split("if (!SaveWorldProgress()) {",1)[1].split("return false;",1)[0]
        for assignment in ("Salvage = PreviousSalvage", "ResidentQuestStates.Add(Id, 1)",
            "Level = PreviousLevel", "Experience = PreviousXP",
            "ExperienceToNextLevel = PreviousNextXP", "MaxHealth = PreviousMaxHealth",
            "Health = PreviousHealth", "Mana = PreviousMana",
            "SpellPowerMultiplier = PreviousSpellPower"):
            self.assertIn(assignment,failed)
        self.assertIn("Save->ResidentQuestStates = ResidentQuestStates;",self.hero)
        self.assertIn("Save->Level = FMath::Clamp(Level, 1, 100);",self.hero)
        self.assertIn("Save->Experience = FMath::Clamp(Experience, 0, 1000000);",self.hero)
        self.assertIn("ResidentQuestStates",self.save)

    def test_no_new_quest_saves_or_combat_overlays(self):
        self.assertIn("return Hero->CompleteResidentQuest(this);",self.npc)
        self.assertNotIn("GetAttackWarningRemaining",self.hero)
        self.assertNotIn("GUARD / PARRY / DODGE",self.hero)

if __name__ == "__main__":
    unittest.main()

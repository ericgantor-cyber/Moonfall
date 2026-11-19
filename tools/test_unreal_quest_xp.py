"""Source-level regression for atomic, once-only resident quest XP."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1] / "MoonfallUnreal" / "Source" / "MoonfallUnreal"

class QuestXPSourceRegression(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.hero = (ROOT / "MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        cls.npc = (ROOT / "MoonfallWorldNPC.cpp").read_text(encoding="utf-8")
        cls.save = (ROOT / "MoonfallPersistentSave.h").read_text(encoding="utf-8")
        cls.hud = (ROOT / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        cls.policy = (ROOT / "MoonfallQuestXP.h").read_text(encoding="utf-8")

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


    def test_resident_menu_and_starting_tracker_preview_rewards(self):
        self.assertIn('#include "MoonfallQuestXP.h"', self.hud)
        menu = self.hud.split("if (const AMoonfallWorldNPC* Resident = Hero->FocusedResident.Get())", 1)[1].split("if (Hero->bAtlasOpen)", 1)[0]
        self.assertIn("MoonfallQuestXP::Preview(", menu)
        self.assertIn("MoonfallQuestXP::RankName(", menu)
        self.assertIn("SalvageCost, Hero->Level, Hero->Experience, Hero->ExperienceToNextLevel", menu)
        self.assertIn("XPPlan.LevelsGained > 0", menu)
        self.assertIn("XPPlan.RewardXP", menu)
        self.assertIn('RewardLine = TEXT("CONTRACT COMPLETED | REWARD CLAIMED");', menu)
        self.assertIn("if (H >= 330.f)", menu)
        self.assertIn("SALVAGE %d/%d | +%d XP", menu)
        self.assertIn("ResidentFeedbackMessage", menu)
        self.assertIn('MoonfallQuestXP::Reward(GuideSalvageCost, Hero->Level)', self.hud)
        self.assertIn('Quest ready: turn in for +%d XP', self.hud)
        self.assertNotIn("GUARD / PARRY / DODGE", self.hud)
        self.assertNotIn("GetAttackWarningRemaining", self.hud)

    def test_all_regional_residents_are_whitelisted_for_quest_and_bond_saves(self):
        regional = {
            "waystation_ashen_scout", "waystation_ashen_artisan",
            "waystation_moonveil_scout", "waystation_moonveil_scholar",
            "waystation_hollows_scout", "waystation_hollows_tinker",
            "waystation_foundry_scout", "waystation_foundry_tinker",
            "waystation_frost_scout", "waystation_frost_scholar",
            "waystation_eclipse_scout", "waystation_eclipse_elder",
        }
        lists = []
        for label in ("SaveResidentIds", "AllowedResidentIds", "ValidIds", "ResidentIds"):
            match = re.search(
                r"static const FName " + label + r"\[\] = \{(.*?)\};",
                self.hero, flags=re.S,
            )
            self.assertIsNotNone(match, label)
            keys = re.findall(r'FName\(TEXT\("([^"]+)"\)\)', match.group(1))
            self.assertEqual(len(keys), len(set(keys)), label)
            self.assertEqual(len(keys), 25, label)
            self.assertTrue(regional.issubset(keys), label)
            self.assertIn("CrossroadsGuide", keys)
            lists.append(set(keys))
        self.assertTrue(all(keys == lists[0] for keys in lists))
        self.assertIn("ResidentQuestStates.Num() > 64", self.hero)
        self.assertIn("ResidentBonds", self.save)
        self.assertIn("ResidentQuestStates", self.save)
        self.assertIn("!IsNamedMoonfallResident(Entry.Key)", self.hero)
        self.assertIn("Save->ResidentQuestStates = ResidentQuestStates;", self.hero)

    def test_policy_covers_rank_and_exact_forecast_without_new_rng(self):
        for signature in ("constexpr const char* RankName(int SalvageCost)",
                          "struct FProjection", "constexpr FProjection Preview(",
                          "int LevelsGained;", "int RemainingXP;"):
            self.assertIn(signature, self.policy)
        self.assertIn("while (XPAfter >= Threshold && LevelAfter < 100)", self.policy)
        self.assertIn("Threshold = 100 + (LevelAfter - 1) * 55", self.policy)
        self.assertIn("if (LevelAfter >= 100) XPAfter = 0;", self.policy)
        self.assertNotIn("FMath::Rand", self.policy)
        self.assertIn("SaveWorldProgress()", self.hero)

if __name__ == "__main__":
    unittest.main()

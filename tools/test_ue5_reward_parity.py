#!/usr/bin/env python3
"""Static UE5 reward parity contract. Does not compile Unreal or play the game."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1] / "MoonfallUnreal" / "Source" / "MoonfallUnreal"

class UE5RewardParity(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.hero_h = (ROOT / "MoonfallHeroPawn.h").read_text()
        cls.hero = (ROOT / "MoonfallHeroPawn.cpp").read_text()
        cls.enemy = (ROOT / "MoonfallEnemy.cpp").read_text()
        cls.save = (ROOT / "MoonfallRewardSaveGame.h").read_text()
        cls.hud = (ROOT / "MoonfallCombatHUD.cpp").read_text()

    def test_persistent_reward_schema_and_engine_limit(self):
        self.assertIn("class MOONFALLUNREAL_API UMoonfallRewardSaveGame : public USaveGame", self.save)
        for field in ("int64 Gold = 0", "int64 ConfirmedKills = 0", "TArray<FName> CollectedKeys"):
            self.assertIn(field, self.save)
        self.assertIn("SaveGameToSlot(Save, RewardSaveSlot, RewardSaveUser)", self.hero)
        self.assertIn("LoadGameFromSlot(RewardSaveSlot, RewardSaveUser)", self.hero)
        self.assertIn("LoadRewardProgress();", self.hero)
        self.assertIn("MAX_int64 - Gold", self.hero)
        self.assertNotIn("99999999", self.hero[self.hero.index("int64 AMoonfallHeroPawn::AddGold"):self.hero.index("bool AMoonfallHeroPawn::HasQuestKey")])

    def test_key_whitelist_duplicate_guard_and_milestone(self):
        for key in ("wardens_crest", "outlaw_seal", "fallen_gate_key"):
            self.assertIn(key, self.hero)
        self.assertIn("!IsSupportedQuestKey(KeyId) || HasQuestKey(KeyId)", self.hero)
        self.assertIn("CollectedKeys.Num() < 128", self.hero)
        self.assertIn("if (ConfirmedRewardKills == 5)", self.hero)
        self.assertIn("SaveRewardProgress();", self.hero.split("bool AMoonfallHeroPawn::CollectQuestKey", 1)[1])

    def test_gold_is_from_confirmed_death_not_hud_or_damage(self):
        death = self.enemy.split("OnEnemyDefeated(bLastHitWasShatter);", 1)[1]
        self.assertIn("Hero->AwardEnemyCurrency(bElite, bBoss);", death)
        self.assertIn("Hero->SpawnGroundEquipmentDrop(bElite, bBoss, GetActorLocation());", death)
        self.assertEqual(self.enemy.count("Hero->AwardEnemyCurrency(bElite, bBoss);"), 1)
        self.assertIn("const int64 BaseGold = 3 + ConfirmedRewardKills / 4;", self.hero)
        self.assertIn("AddGold(BaseGold);", self.hero)

    def test_equipment_remains_manual_and_hud_shows_rewards(self):
        self.assertIn("void PickupNearestGroundLoot();", self.hero_h)
        self.assertIn("Loot->TryCollect()", self.hero)
        self.assertIn("Hero->Gold", self.hud)
        self.assertIn("Hero->CollectedKeys.Num()", self.hud)
        self.assertIn("void AMoonfallHeroPawn::EquipSelectedInventoryItem()", self.hero)

if __name__ == "__main__":
    unittest.main()

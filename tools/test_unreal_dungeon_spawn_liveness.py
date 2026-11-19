"""UE5 dungeon encounter liveness regression checks (no Unreal runtime required)."""
import unittest
from pathlib import Path

SRC = Path(__file__).resolve().parents[1] / "MoonfallUnreal" / "Source" / "MoonfallUnreal"
SOURCE = (SRC / "MoonfallDungeonLayout.cpp").read_text(encoding="utf-8")

class DungeonSpawnLiveness(unittest.TestCase):
    def test_bonus_spawn_does_not_count_invalid_or_dead_actors(self):
        code = SOURCE.split("bool AMoonfallDungeonLayout::TrySpawnBonusGroup(", 1)[1].split(
            "int32 AMoonfallDungeonLayout::GetOrLockEncounterLevel(", 1)[0]
        self.assertIn("Enemy->FinishSpawning(SpawnTransform);", code)
        self.assertIn("if (!IsValid(Enemy)) continue;", code)
        self.assertIn("Enemy->IsActorBeingDestroyed()", code)
        self.assertIn("!FMath::IsFinite(Enemy->Health) || Enemy->Health <= 0.f", code)
        self.assertLess(code.index("Enemy->FinishSpawning(SpawnTransform);"),
                        code.index("!FMath::IsFinite(Enemy->Health)"))
        self.assertLess(code.index("Enemy->Destroy();", code.index("Enemy->FinishSpawning(SpawnTransform);")),
                        code.index("GroupActors.Add(Enemy);"))
        self.assertIn("if (!bAllSpawned || GroupActors.Num() != GroupSize)", code)
        self.assertIn("for (AMoonfallEnemy* Enemy : GroupActors)", code)
        self.assertLess(code.index("GroupActors.Num() != GroupSize"),
                        code.index("SpawnedBonusGroupsMask |= GroupBit;"))

    def test_boss_callback_does_not_consume_encounter_when_dead(self):
        code = SOURCE.split("bool AMoonfallDungeonLayout::StartFinalBossChamber(", 1)[1].split(
            "bool AMoonfallDungeonLayout::TrySpawnBonusGroup(", 1)[0]
        begin = code.index("Boss->OnDungeonBossIdentityReady(")
        announce = code.index("OnDungeonBossSpawned(Hero, ZoneIndex, DungeonIndex);")
        finish = code.index("return true;", announce)
        identity = code[begin:announce]
        callback = code[announce:finish]
        for stage in (identity, callback):
            self.assertIn("!IsValid(Boss) || Boss->IsActorBeingDestroyed()", stage)
            self.assertIn("!FMath::IsFinite(Boss->Health) || Boss->Health <= 0.f", stage)
            self.assertIn("Boss->Destroy();", stage)
        self.assertLess(identity.index("!FMath::IsFinite(Boss->Health)"),
                        len(identity))
        self.assertIn("return false;", callback)

    def test_authored_counts_saves_and_boss_identity_remain(self):
        for line in ("GroupSize < 5 || GroupSize > 11", "GetOrLockEncounterLevel(Hero)",
                     "if (Spawned != Count)", "SpawnedEncounterCount += Spawned",
                     "if (Hero->HasDefeatedDungeonBoss(ZoneIndex, DungeonIndex)) return true;",
                     "Boss->DungeonBossTitle = FText::FromString(",
                     "SpawnedBonusGroupsMask |= GroupBit;"):
            self.assertIn(line, SOURCE)
        self.assertNotIn("GetAttackWarningRemaining(", SOURCE)
        self.assertNotIn("GUARD / PARRY / DODGE", SOURCE)

if __name__ == "__main__":
    unittest.main()

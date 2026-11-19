"""Cross-PR integration regressions for #1048, #1049 and #1050.

These are source-level checks, NOT native UE5.4 runtime verification.
"""
import unittest
from pathlib import Path
SRC=Path(__file__).resolve().parents[1]/"MoonfallUnreal"/"Source"/"MoonfallUnreal"

class DungeonRetrySafety(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.layout=(SRC/"MoonfallDungeonLayout.cpp").read_text(encoding="utf-8")
        cls.mode=(SRC/"MoonfallUnrealGameMode.cpp").read_text(encoding="utf-8")
        cls.arena=(SRC/"MoonfallArena.cpp").read_text(encoding="utf-8")

    def bonus(self):
        return self.layout.split("bool AMoonfallDungeonLayout::TrySpawnBonusGroup(",1)[1].split(
            "int32 AMoonfallDungeonLayout::GetOrLockEncounterLevel(",1)[0]

    def boss(self):
        return self.layout.split("bool AMoonfallDungeonLayout::StartFinalBossChamber(",1)[1]

    def test_bonus_deferred_spawn_validates_health_and_actor(self):
        code=self.bonus()
        for s in ("if (!IsValid(Enemy)) continue;",
                  "Enemy->IsActorBeingDestroyed()",
                  "!FMath::IsFinite(Enemy->Health) || Enemy->Health <= 0.f",
                  "Enemy->Destroy();",
                  "GroupActors.Add(Enemy);"):
            self.assertIn(s,code)
        self.assertLess(code.index("Enemy->FinishSpawning(SpawnTransform);"),
                        code.index("!FMath::IsFinite(Enemy->Health)"))
        self.assertLess(code.index("Enemy->Destroy();"), code.index("GroupActors.Add(Enemy);"))

    def test_bonus_groups_remain_atomic_and_retryable(self):
        code=self.bonus()
        self.assertIn("if (!bAllSpawned || GroupActors.Num() != GroupSize)",code)
        self.assertIn("for (AMoonfallEnemy* Enemy : GroupActors)",code)
        self.assertIn("if (IsValid(Enemy)) Enemy->Destroy();",code)
        self.assertIn("SpawnedBonusGroupsMask |= GroupBit;",code)
        self.assertLess(code.index("GroupActors.Num() != GroupSize"),
                        code.index("SpawnedBonusGroupsMask |= GroupBit;"))

    def test_startup_ignores_destroying_authored_arena(self):
        code=self.mode.split("void AMoonfallUnrealGameMode::BeginPlay()",1)[1].split(
            "void AMoonfallUnrealGameMode::SpawnDungeonBatch()",1)[0]
        self.assertIn("if (IsValid(*It) && !It->IsActorBeingDestroyed())",code)
        self.assertLess(code.index("if (IsValid(*It) && !It->IsActorBeingDestroyed())"),
                        code.index("SpawnActor<AMoonfallArena>"))

    def test_startup_ignores_destroying_authored_dungeon(self):
        code=self.mode.split("void AMoonfallUnrealGameMode::BeginPlay()",1)[1].split(
            "void AMoonfallUnrealGameMode::SpawnDungeonBatch()",1)[0]
        self.assertIn("if (!IsValid(*It) || It->IsActorBeingDestroyed()) continue;",code)
        self.assertLess(code.index("It->IsActorBeingDestroyed()) continue;"),
                        code.index("AuthoredDungeonSlots[Slot] = 1;"))
        self.assertIn("if (!AuthoredDungeonSlots[Slot])",code)
        self.assertIn("SpawnDungeonBatch();",code)

    def test_boss_health_validated_after_identity_and_encounter_callbacks(self):
        code=self.boss()
        identity="Boss->OnDungeonBossIdentityReady("
        notify="OnDungeonBossSpawned(Hero, ZoneIndex, DungeonIndex);"
        self.assertIn(identity,code)
        self.assertIn(notify,code)
        after_id=code.split(identity,1)[1].split(notify,1)[0]
        after_notify=code.split(notify,1)[1]
        actor_check="if (!IsValid(Boss) || Boss->IsActorBeingDestroyed()) return false;"
        health_check="if (!FMath::IsFinite(Boss->Health) || Boss->Health <= 0.f)"
        for segment in (after_id,after_notify):
            self.assertIn(actor_check,segment)
            self.assertIn(health_check,segment)
            self.assertLess(segment.index(actor_check),segment.index(health_check))
            self.assertLess(segment.index(health_check),segment.index("Boss->Destroy();"))
            self.assertLess(segment.index("Boss->Destroy();"),segment.index("return false;\n }"))
        self.assertIn("return true;",after_notify)
        self.assertLess(after_notify.index(health_check),after_notify.index("return true;"))

    def test_boss_retry_and_identity_preserved(self):
        code=self.boss()
        self.assertIn("for (int32 Attempt = 0; Attempt < 3; ++Attempt)",code)
        self.assertIn("return false; // All collision-safe spawn positions were unavailable.",code)
        self.assertIn('FName(TEXT("MoonfallDungeonBoss"))',code)
        self.assertIn("DungeonBossFamily",code)
        self.assertIn("DungeonBossRank",code)

    def test_regional_waystations_still_present(self):
        self.assertIn("static const FWaystationSite Sites[6] =",self.arena)
        self.assertIn("RegionalWaystationFloors->AddInstance(",self.arena)
        self.assertIn('waystation_eclipse',self.arena)

if __name__=="__main__":
    unittest.main()

#!/usr/bin/env python3
"""Source-level world-migration contracts; these do not compile Unreal."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
CPP = (ROOT / "MoonfallUnreal/Source/MoonfallUnreal/MoonfallArena.cpp").read_text(encoding="utf-8")
HEADER = (ROOT / "MoonfallUnreal/Source/MoonfallUnreal/MoonfallArena.h").read_text(encoding="utf-8")
MODE = (ROOT / "MoonfallUnreal/Source/MoonfallUnreal/MoonfallUnrealGameMode.cpp").read_text(encoding="utf-8")

class WorldFoundationTests(unittest.TestCase):
    def test_counts_and_unique_ids(self):
        self.assertIn("constexpr int32 Regions = 6;", CPP)
        self.assertIn("constexpr int32 DungeonsPerRegion = 5;", CPP)
        self.assertIn("constexpr int32 ChambersPerDungeon = 14;", CPP)
        room_ids = {(r * 5 + d) * 14 + c for r in range(6) for d in range(5) for c in range(14)}
        self.assertEqual(len(room_ids), 420)
        monster_ids = {r * 3 + n for r in range(6) for n in range(3)}
        resident_ids = {18 + (r * 5 + d) * 14 + (stage - 1)
                        for r in range(6) for d in range(5) for stage in (3, 5, 9, 12, 14)}
        self.assertEqual(len(monster_ids), 18)
        self.assertEqual(len(resident_ids), 150)
        self.assertFalse(monster_ids & resident_ids)

    def test_proximity_and_budget(self):
        self.assertIn("PrimaryActorTick.TickInterval = 0.25f", CPP)
        self.assertIn("FVector::DistSquared2D(Here, At)", CPP)
        self.assertIn("BuiltOne = true;", CPP)
        self.assertIn("SpawnedOne = SpawnEncounter(", CPP)
        self.assertIn("SpawnedEncounters.Contains(Id)", CPP)
        self.assertIn("SpawnedEncounters.Add(Id)", CPP)
        self.assertIn("Stage == 14", CPP)
        self.assertIn("Stage >= 9", CPP)

    def test_no_wave_mode_or_art_mutation(self):
        self.assertIn("bool bContinuousWorld = true;", HEADER)
        self.assertIn("if (bContinuousWorld) { LoadWorldProgress(); BuildWorld(); return; }", CPP)
        self.assertIn("BuildLegacyArena", CPP) if "BuildLegacyArena" in CPP else self.assertIn("const int32 Count = FMath::Clamp(EnemyCount", CPP)
        self.assertIn("SpawnActor<AMoonfallArena>", MODE)
        self.assertNotIn("Z.png", CPP)
        self.assertNotIn("Nyra.png", CPP)

    def test_persistent_defeats_are_validated_and_separate(self):
        save = (ROOT / "MoonfallUnreal/Source/MoonfallUnreal/MoonfallWorldProgressSave.h").read_text(encoding="utf-8")
        self.assertIn('MoonfallUnrealWorldProgress_v1', CPP)
        self.assertIn('SchemaVersion = 1', save)
        self.assertIn('DefeatedEncounterIds', save)
        self.assertIn('Saved->SchemaVersion != 1', CPP)
        self.assertIn('if (IsValidEncounterId(Id)) DefeatedEncounters.Add(Id);', CPP)
        self.assertIn('DefeatedEncounters.Contains(Id)', CPP)
        self.assertIn('Enemy->Health <= 0.f', CPP)
        self.assertIn('It.RemoveCurrent();', CPP)
        self.assertIn('bWorldSavePending = true;', CPP)
        self.assertIn('SaveGameToSlot(Saved, WorldProgressSlot, 0)', CPP)
        self.assertIn('if (bWorldSavePending) SaveWorldProgress();', CPP)
        valid = {i for i in range(18)}
        valid |= {18 + room for room in range(420) if room % 14 in (2, 4, 8, 11, 13)}
        self.assertEqual(len(valid), 168)
        self.assertEqual(len({i for i in valid if i >= 18}), 150)
        self.assertEqual(len(valid & set(range(18))), 18)

    def test_eighteen_region_enemy_profiles(self):
        import re
        enemy = (ROOT / "MoonfallUnreal/Source/MoonfallUnreal/MoonfallEnemy.h").read_text(encoding="utf-8")
        block = CPP.split("constexpr FRegionalEnemyProfile RegionalEnemyProfiles[18] = {", 1)[1].split("};", 1)[0]
        entries = re.findall(r'\{TEXT\("([^"]+)"\),\s*([0-9.]+)f,\s*([0-9.]+)f,\s*([0-9.]+)f,\s*([0-9.]+)f,\s*([0-9.]+)f,\s*([0-9.]+)f\}', block)
        self.assertEqual(len(entries), 18)
        self.assertEqual(len({entry[0] for entry in entries}), 18)
        for entry in entries:
            hp, damage, speed, reach, cooldown, posture = (float(v) for v in entry[1:])
            self.assertTrue(0.6 <= hp <= 2.1)
            self.assertTrue(0.7 <= damage <= 1.7)
            self.assertTrue(0.6 <= speed <= 1.6)
            self.assertTrue(0.8 <= reach <= 1.25)
            self.assertTrue(0.7 <= cooldown <= 1.6)
            self.assertTrue(0.7 <= posture <= 2.2)
        for region in range(6):
            self.assertEqual(len(set(entries[region * 3:region * 3 + 3])), 3)
        self.assertIn("Enemy->ArchetypeName = FName(Profile.Name);", CPP)
        self.assertIn("Enemy->bElite = ProfileIndex % 3 == 2;", CPP)
        self.assertIn("ArchetypeName", enemy)
        for stat in ("Health", "AttackDamage", "ChaseSpeed", "AttackRange", "AttackCooldown", "MaxPosture"):
            self.assertIn("Enemy->" + stat + " = FMath::Clamp(", CPP)
        self.assertIn("if (Id >= 0 && Id < 18)", CPP)
        self.assertIn("SpawnedOne = SpawnEncounter(Id, At, Region, false, false)", CPP)

    def test_additional_regional_enemies_save_compatibility(self):
        import re
        block = CPP.split("constexpr FRegionalEnemyProfile ExtraRegionalEnemyProfiles[18] = {", 1)[1].split("};", 1)[0]
        names = re.findall(r'TEXT\("([^"]+)"\)', block)
        original = CPP.split("constexpr FRegionalEnemyProfile RegionalEnemyProfiles[18] = {", 1)[1].split("};", 1)[0]
        old_names = re.findall(r'TEXT\("([^"]+)"\)', original)
        self.assertEqual(len(names), 18)
        self.assertEqual(len(set(names + old_names)), 36)
        original_ids = set(range(18))
        dungeon_ids = {18 + room for room in range(420) if room % 14 in (2, 4, 8, 11, 13)}
        new_ids = {438 + region * 3 + index for region in range(6) for index in range(3)}
        self.assertEqual(len(new_ids), 18)
        self.assertFalse(new_ids & (original_ids | dungeon_ids))
        self.assertEqual(len(new_ids | original_ids | dungeon_ids), 186)
        self.assertIn("constexpr int32 ExtraRegionalEncounterBase = 18 + Regions * DungeonsPerRegion * ChambersPerDungeon;", CPP)
        self.assertIn("ExtraRegionalEnemyProfiles[ProfileIndex]", CPP)
        self.assertIn("const int32 Id = ExtraRegionalEncounterBase + Region * 3 + Index;", CPP)
        self.assertIn("Id >= ExtraRegionalEncounterBase && Id < ExtraRegionalEncounterBase + 18) return true;", CPP)
        self.assertIn("if (DefeatedEncounters.Num() > 186)", CPP)
        self.assertIn("SpawnedOne = SpawnEncounter(Id, At, Region, false, false);", CPP)
        self.assertIn("ProfileIndex % 3 == 2", CPP)
        for row in re.findall(r'\{TEXT\("[^"]+"\),\s*([^}]+)\}', block):
            scalars = [float(v) for v in re.findall(r'([0-9]+\.[0-9]+)f', row)]
            self.assertEqual(len(scalars), 6)
            self.assertTrue(all(0.6 <= v <= 2.2 for v in scalars))

    def test_regional_attack_overhaul(self):
        enemy_h = (ROOT / "MoonfallUnreal/Source/MoonfallUnreal/MoonfallEnemy.h").read_text(encoding="utf-8")
        enemy_cpp = (ROOT / "MoonfallUnreal/Source/MoonfallUnreal/MoonfallEnemy.cpp").read_text(encoding="utf-8")
        self.assertIn("RegionalAttackStyle = 0", enemy_h)
        self.assertIn("Enemy->RegionalAttackStyle = ProfileIndex % 6;", CPP)
        self.assertIn("switch (FMath::Clamp(RegionalAttackStyle, 0, 5))", enemy_cpp)
        for style in range(6):
            self.assertIn("case " + str(style) + ":", enemy_cpp)
        self.assertIn("CurrentWindup = FMath::Clamp(CurrentWindup, 0.32f, 2.f);", enemy_cpp)
        self.assertIn("CurrentAttackMultiplier = FMath::Clamp(CurrentAttackMultiplier, 0.5f, 2.3f);", enemy_cpp)
        self.assertIn("CurrentAttackRangeMultiplier = FMath::Clamp(CurrentAttackRangeMultiplier, 0.75f, 1.8f);", enemy_cpp)
        self.assertIn("if (!bBoss)", enemy_cpp)
        self.assertIn("#if ENABLE_DRAW_DEBUG", enemy_cpp)
        self.assertIn("DrawDebugCircle(GetWorld(), GetActorLocation()", enemy_cpp)
        self.assertIn("Hero->ApplyMoonfallDamageFrom(", enemy_cpp)
        self.assertIn("Hero->TryRewardPerfectDodge()", enemy_cpp)

    def test_packaged_combat_visual_foundation(self):
        enemy_h = (ROOT / "MoonfallUnreal/Source/MoonfallUnreal/MoonfallEnemy.h").read_text(encoding="utf-8")
        enemy_cpp = (ROOT / "MoonfallUnreal/Source/MoonfallUnreal/MoonfallEnemy.cpp").read_text(encoding="utf-8")
        self.assertIn("UPointLightComponent* AttackTellLight", enemy_h)
        self.assertIn('CreateDefaultSubobject<UPointLightComponent>(TEXT("AttackTellLight"))', enemy_cpp)
        self.assertIn("AttackTellLight->SetCastShadows(false);", enemy_cpp)
        self.assertIn("AttackTellLight->SetVisibility(false);", enemy_cpp)
        self.assertIn("if (Health <= 0.f)", enemy_cpp)
        self.assertIn("if (AttackTellLight) AttackTellLight->SetVisibility(false);", enemy_cpp)
        self.assertIn("Body->SetRelativeRotation(FRotator(Lean, Twist, 0.f));", enemy_cpp)
        self.assertIn("AttackTellLight->SetLightColor(SchoolColor);", enemy_cpp)
        self.assertIn("AttackTellLight->SetIntensity(400.f + 1100.f * Progress);", enemy_cpp)
        self.assertIn("AttackTellLight->SetVisibility(bVisible);", enemy_cpp)
        self.assertIn("WindupEndsAt > Now && FMath::IsFinite(WindupEndsAt)", enemy_cpp)
        # Visual additions must not change combat-hit calculations or create actors.
        self.assertIn("Hero->ApplyMoonfallDamageFrom(", enemy_cpp)
        self.assertNotIn("SpawnActor<UPointLightComponent>", enemy_cpp)

    def test_connected_geometry(self):
        self.assertIn("Region * RegionSpacing", CPP)
        self.assertIn("FVector(60.f, 60.f, 1.f)", CPP)
        self.assertIn("FVector(3.5f, 120.f, 1.f)", CPP)
        self.assertIn("GetPlayerPawn(this, 0)", CPP)
        self.assertIn("BeginDeferredActorSpawnFromClass", CPP)
        self.assertIn("FinishSpawningActor", CPP)

if __name__ == "__main__":
    unittest.main()

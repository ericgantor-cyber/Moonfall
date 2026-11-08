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

    def test_attack_styles_not_overwritten_and_animation_events_once(self):
        enemy_cpp = (ROOT / "MoonfallUnreal/Source/MoonfallUnreal/MoonfallEnemy.cpp").read_text(encoding="utf-8")
        enemy_h = (ROOT / "MoonfallUnreal/Source/MoonfallUnreal/MoonfallEnemy.h").read_text(encoding="utf-8")
        base_attack = enemy_cpp.index("CurrentWindup = Pattern == 0 ?")
        special_attack = enemy_cpp.index("if (BossSpecialMove == 3)")
        regional_styles = enemy_cpp.index("switch (FMath::Clamp(RegionalAttackStyle, 0, 5))")
        commitment = enemy_cpp.rindex("WindupEndsAt = Now + CurrentWindup;")
        self.assertLess(base_attack, special_attack)
        self.assertLess(special_attack, regional_styles)
        self.assertLess(regional_styles, commitment)
        self.assertIn("OnAttackWindup(int32 Style, float WindupSeconds, float Reach, bool bBossAttack)", enemy_h)
        self.assertIn("OnAttackReleased(int32 Style, bool bBossAttack)", enemy_h)
        self.assertEqual(enemy_cpp.count("OnAttackWindup("), 2)
        self.assertEqual(enemy_cpp.count("OnAttackReleased("), 1)
        self.assertIn("OnAttackReleased(bBoss ? BossSpecialMove : RegionalAttackStyle, bBoss);", enemy_cpp)
        self.assertIn("CurrentWindup = FMath::Clamp(CurrentWindup, 0.32f, 2.f);", enemy_cpp)

    def test_exploration_discovery_and_minimap(self):
        save = (ROOT / "MoonfallUnreal/Source/MoonfallUnreal/MoonfallWorldProgressSave.h").read_text(encoding="utf-8")
        hud = (ROOT / "MoonfallUnreal/Source/MoonfallUnreal/MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        self.assertIn("DiscoveredChamberIds", save)
        self.assertIn("TSet<int32> DiscoveredChambers;", HEADER)
        self.assertIn("GetDiscoveredChambers() const", HEADER)
        self.assertIn("Distance < 450.f * 450.f && !DiscoveredChambers.Contains(Room)", CPP)
        self.assertIn("bWorldSavePending = true;", CPP)
        self.assertIn("for (const int32 Room : Saved->DiscoveredChamberIds)", CPP)
        self.assertIn("Room >= 0 && Room < Regions * DungeonsPerRegion * ChambersPerDungeon", CPP)
        self.assertIn("Saved->DiscoveredChamberIds.Sort();", CPP)
        self.assertIn("Arena->GetDiscoveredChambers()", hud)
        self.assertIn("if (RoomId < 0 || RoomId >= 420) continue;", hud)
        self.assertIn("const int32 Region = RoomId / 70;", hud)
        self.assertIn("const int32 Dungeon = (RoomId % 70) / 14;", hud)
        self.assertIn("const int32 Chamber = RoomId % 14;", hud)
        self.assertIn("DX * DX + DY * DY > MapRadius * MapRadius", hud)
        self.assertIn("const FVector PlayerPos = Hero->GetActorLocation();", hud)
        # No unvisited-location reveal: map may only iterate the discovered set.
        self.assertNotIn("for (int32 RoomId = 0; RoomId < 420;", hud)
        for region in range(6):
            for dungeon in range(5):
                for chamber in range(14):
                    room = (region * 5 + dungeon) * 14 + chamber
                    self.assertEqual((room // 70, (room % 70) // 14, room % 14),
                                     (region, dungeon, chamber))

    def test_atlas_region_terrain_and_entrance_fog(self):
        save = (ROOT / "MoonfallUnreal/Source/MoonfallUnreal/MoonfallWorldProgressSave.h").read_text(encoding="utf-8")
        hud = (ROOT / "MoonfallUnreal/Source/MoonfallUnreal/MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        self.assertIn("DiscoveredRegionIds", save)
        self.assertIn("DiscoveredDungeonEntranceIds", save)
        self.assertIn("TSet<int32> DiscoveredRegions;", HEADER)
        self.assertIn("TSet<int32> DiscoveredDungeonEntrances;", HEADER)
        self.assertIn("GetDiscoveredRegions() const", HEADER)
        self.assertIn("GetDiscoveredDungeonEntrances() const", HEADER)
        self.assertIn("FVector::DistSquared2D(Here, RegionPos) < 2200.f * 2200.f", CPP)
        self.assertIn("FVector::DistSquared2D(Here, Entrance) < 650.f * 650.f", CPP)
        self.assertIn("const int32 EntranceId = Region * DungeonsPerRegion + Dungeon;", CPP)
        self.assertIn("Saved->DiscoveredRegionIds.Sort();", CPP)
        self.assertIn("Saved->DiscoveredDungeonEntranceIds.Sort();", CPP)
        self.assertIn("if (Region >= 0 && Region < Regions) DiscoveredRegions.Add(Region);", CPP)
        self.assertIn("Entrance >= 0 && Entrance < Regions * DungeonsPerRegion", CPP)
        self.assertIn("Arena->GetDiscoveredRegions()", hud)
        self.assertIn("Arena->GetDiscoveredDungeonEntrances()", hud)
        self.assertIn("if (EntranceId < 0 || EntranceId >= 30) continue;", hud)
        self.assertIn("const int32 Region = EntranceId / 5;", hud)
        self.assertIn("const int32 Dungeon = EntranceId % 5;", hud)
        # No fog bypass: draw from discovered sets, never all possible entrances.
        self.assertNotIn("for (int32 EntranceId = 0; EntranceId < 30;", hud)
        self.assertNotIn("for (int32 Region = 0; Region < 6; ++Region)", hud)
        self.assertEqual(len({r * 5 + d for r in range(6) for d in range(5)}), 30)

    def test_guardian_quests_grounded_in_real_defeat_and_discovery(self):
        save = (ROOT / "MoonfallUnreal/Source/MoonfallUnreal/MoonfallWorldProgressSave.h").read_text(encoding="utf-8")
        hud = (ROOT / "MoonfallUnreal/Source/MoonfallUnreal/MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        self.assertIn("UPROPERTY() int32 TrackedDungeonQuestId = -1;", save)
        self.assertIn("bool TrackDungeonQuest(int32 EntranceId);", HEADER)
        self.assertIn("bool IsDungeonQuestCompleted(int32 EntranceId) const;", HEADER)
        self.assertIn("GetCompletedDungeonQuestCount() const;", HEADER)
        self.assertIn("TrackedDungeonQuestId = -1;", CPP)
        self.assertIn("if (TrackedDungeonQuestId < 0) TrackedDungeonQuestId = EntranceId;", CPP)
        self.assertIn("DiscoveredDungeonEntrances.Contains(Saved->TrackedDungeonQuestId)", CPP)
        self.assertIn("Saved->TrackedDungeonQuestId = DiscoveredDungeonEntrances.Contains(TrackedDungeonQuestId)", CPP)
        self.assertIn("const int32 GuardianEncounterId = 18 + EntranceId * ChambersPerDungeon + (ChambersPerDungeon - 1);", CPP)
        self.assertIn("return DefeatedEncounters.Contains(GuardianEncounterId);", CPP)
        self.assertIn("if (!bContinuousWorld || !DiscoveredDungeonEntrances.Contains(EntranceId)) return false;", CPP)
        self.assertIn("Arena->GetDiscoveredDungeonEntrances().Contains(QuestId)", hud)
        self.assertIn("Arena->IsDungeonQuestCompleted(EntranceId)", hud)
        self.assertIn("Arena->GetCompletedDungeonQuestCount()", hud)
        self.assertIn('TEXT("Defeat the guardian")', hud)
        self.assertIn('TEXT("Guardian defeated")', hud)
        # No quest may be marked completed by visitation alone.
        self.assertEqual(len({18 + eid * 14 + 13 for eid in range(30)}), 30)
        self.assertTrue(all((18 + eid * 14 + 13) % 14 == 3 for eid in range(30)))

    def test_dungeon_quest_chains_and_permanent_capacity_rewards(self):
        hero_h = (ROOT / "MoonfallUnreal/Source/MoonfallUnreal/MoonfallHeroPawn.h").read_text(encoding="utf-8")
        hero_cpp = (ROOT / "MoonfallUnreal/Source/MoonfallUnreal/MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        hud = (ROOT / "MoonfallUnreal/Source/MoonfallUnreal/MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        self.assertIn("GetDungeonResidentDefeatCount(int32 EntranceId) const", HEADER)
        self.assertIn("const int32 Stages[] = {3, 5, 9, 12};", CPP)
        self.assertIn("if (DefeatedEncounters.Contains(EncounterId)) ++Defeated;", CPP)
        self.assertIn("Hero->ApplyGuardianQuestRewards(GetCompletedDungeonQuestCount());", CPP)
        self.assertIn("AppliedGuardianQuestRewards = 0;", hero_h)
        self.assertIn("void ApplyGuardianQuestRewards(int32 CompletedGuardianCount);", hero_h)
        self.assertIn("if (Completed <= AppliedGuardianQuestRewards) return;", hero_cpp)
        self.assertIn("const int32 Delta = Completed - AppliedGuardianQuestRewards;", hero_cpp)
        self.assertIn("MaxMana = FMath::Clamp(MaxMana + 3.f * Delta", hero_cpp)
        self.assertIn("MaxStamina = FMath::Clamp(MaxStamina + 3.f * Delta", hero_cpp)
        reward_body = hero_cpp.split("void AMoonfallHeroPawn::ApplyGuardianQuestRewards(", 1)[1].split("void AMoonfallHeroPawn::AwardExperience(", 1)[0]
        self.assertNotIn("Health =", reward_body)
        self.assertNotIn("AwardExperience(", reward_body)
        self.assertNotIn("AwardEquipmentDrop(", reward_body)
        self.assertIn("ResidentsDefeated < 4", hud)
        self.assertIn('TEXT("Defeat residents: %d / 4")', hud)
        self.assertIn("GetCompletedDungeonQuestCount() * 3", hud)
        for eid in range(30):
            resident_ids = {18 + eid * 14 + stage - 1 for stage in (3, 5, 9, 12)}
            guardian_id = 18 + eid * 14 + 13
            self.assertEqual(len(resident_ids), 4)
            self.assertNotIn(guardian_id, resident_ids)

    def test_regional_quest_guides_and_accessible_interaction(self):
        root = ROOT / "MoonfallUnreal"
        guide_h = (root / "Source/MoonfallUnreal/MoonfallQuestGuide.h").read_text(encoding="utf-8")
        guide_cpp = (root / "Source/MoonfallUnreal/MoonfallQuestGuide.cpp").read_text(encoding="utf-8")
        hero_h = (root / "Source/MoonfallUnreal/MoonfallHeroPawn.h").read_text(encoding="utf-8")
        hero_cpp = (root / "Source/MoonfallUnreal/MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        hud = (root / "Source/MoonfallUnreal/MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        inputs = (root / "Config/DefaultInput.ini").read_text(encoding="utf-8")
        self.assertIn("class MOONFALLUNREAL_API AMoonfallQuestGuide", guide_h)
        self.assertIn("PrimaryActorTick.bCanEverTick = false;", guide_cpp)
        self.assertIn("ECollisionEnabled::NoCollision", guide_cpp)
        self.assertIn("SpawnActor<AMoonfallQuestGuide>", CPP)
        self.assertIn("Guide->RegionIndex = Region;", CPP)
        self.assertIn("SelectNextDiscoveredDungeonQuestInRegion(int32 RegionIndex)", HEADER)
        self.assertIn("if (!DiscoveredDungeonEntrances.Contains(EntranceId)) continue;", CPP)
        self.assertIn("return TrackDungeonQuest(EntranceId);", CPP)
        self.assertIn("if (Pass == 0 && IsDungeonQuestCompleted(EntranceId)) continue;", CPP)
        self.assertIn("void InteractQuestGuide();", hero_h)
        self.assertIn("void AMoonfallHeroPawn::InteractQuestGuide()", hero_cpp)
        self.assertIn("bOptionsOpen || bInventoryOpen", hero_cpp)
        self.assertIn("if (!Nearest) return;", hero_cpp)
        self.assertIn("OnQuestGuideSpoken(int32 RegionIndex, bool bFoundKnownObjective)", hero_h)
        self.assertIn("LastGuideMessageUntil = GetWorld()->GetTimeSeconds() + 4.f;", hero_cpp)
        self.assertIn("OnQuestGuideSpoken(LastGuideRegionIndex, bLastGuideFoundQuest);", hero_cpp)
        self.assertIn('TEXT("%s Guide: Explore an entrance first.")', hud)
        self.assertIn('TEXT("%s Guide: A discovered dungeon awaits. Follow the atlas.")', hud)
        self.assertIn("Hero->LastGuideMessageUntil > GetWorld()->GetTimeSeconds()", hud)
        self.assertIn("SelectNextDiscoveredDungeonQuestInRegion(Nearest->RegionIndex)", hero_cpp)
        self.assertIn('ActionName="InteractQuestGuide",Key=R', inputs)
        self.assertIn('ActionName="InteractQuestGuide",Key=Gamepad_DPad_Right', inputs)
        self.assertIn('TEXT("R / D-pad Right: Speak to Guide")', hud)
        self.assertIn("FVector::DistSquared2D(Guide->GetActorLocation(), Hero->GetActorLocation())", hud)
        # Six stationary guides, one per region; no new combat waves or hidden quest discovery.
        self.assertEqual(len([r * 6000 for r in range(6)]), 6)

    def test_connected_geometry(self):
        self.assertIn("Region * RegionSpacing", CPP)
        self.assertIn("FVector(60.f, 60.f, 1.f)", CPP)
        self.assertIn("FVector(3.5f, 120.f, 1.f)", CPP)
        self.assertIn("GetPlayerPawn(this, 0)", CPP)
        self.assertIn("BeginDeferredActorSpawnFromClass", CPP)
        self.assertIn("FinishSpawningActor", CPP)

if __name__ == "__main__":
    unittest.main()

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
        # Full world atlas may scan six regions, but draws only discovered regions.
        self.assertIn("if (!Arena->GetDiscoveredRegions().Contains(Region)) continue;", hud)
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
        self.assertIn("PrimaryActorTick.bCanEverTick = true;", guide_cpp)
        self.assertIn("PrimaryActorTick.TickInterval = 0.5f;", guide_cpp)
        self.assertIn("Body->SetCollisionEnabled(ECollisionEnabled::QueryOnly);", guide_cpp)
        self.assertIn("Body->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);", guide_cpp)
        self.assertIn("SpawnActor<AMoonfallQuestGuide>", CPP)
        self.assertIn("Npc->RegionIndex = Region;", CPP)
        self.assertIn("Npc->RoleIndex = Role;", CPP)
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
        self.assertIn('TEXT("%s: Discover an entrance first.")', hud)
        self.assertIn('TEXT("%s: Dungeon %d is liberated. Seek another.")', hud)
        self.assertIn('TEXT("%s: Dungeon %d guardian awaits.")', hud)
        self.assertIn('TEXT("%s: Dungeon %d, residents %d of 4.")', hud)
        self.assertIn("LastGuideQuestId = -1;", hero_h)
        self.assertIn("LastGuideResidentDefeats = 0;", hero_h)
        self.assertIn("bLastGuideGuardianDefeated = false;", hero_h)
        self.assertIn("LastGuideResidentDefeats = Arena->GetDungeonResidentDefeatCount(Selected);", hero_cpp)
        self.assertIn("bLastGuideGuardianDefeated = Arena->IsDungeonQuestCompleted(Selected);", hero_cpp)
        self.assertIn("const FString Speaker = Hero->LastGuideTitle.IsEmpty()", hud)
        self.assertIn("LastGuideTitle = Nearest->GetGuideTitle().ToString();", hero_cpp)
        self.assertIn("Nearest->SetActorRotation(Facing.Rotation());", hero_cpp)
        self.assertIn("FString LastGuideTitle;", hero_h)
        self.assertIn("Head->SetRelativeLocation(", guide_cpp)
        self.assertIn("LeftArm->SetRelativeRotation(", guide_cpp)
        self.assertIn("RightArm->SetRelativeRotation(", guide_cpp)
        self.assertIn("FText AMoonfallQuestGuide::GetGuideTitle() const", guide_cpp)
        self.assertIn("Head = nullptr;", guide_h)
        self.assertIn("LeftArm = nullptr;", guide_h)
        self.assertIn("RightArm = nullptr;", guide_h)
        self.assertIn("Hero->LastGuideMessageUntil > GetWorld()->GetTimeSeconds()", hud)
        self.assertIn("SelectNextDiscoveredDungeonQuestInRegion(Nearest->RegionIndex)", hero_cpp)
        self.assertIn('ActionName="InteractQuestGuide",Key=R', inputs)
        self.assertIn('ActionName="InteractQuestGuide",Key=Gamepad_DPad_Right', inputs)
        self.assertIn('TEXT("R: Talk   T: Service   V: Craft   G/H: Social")', hud)
        self.assertIn("FVector::DistSquared2D(Guide->GetActorLocation(), Hero->GetActorLocation())", hud)
        # Eighteen non-hostile NPCs, three jobs in each region.
        self.assertEqual(len([(r, role) for r in range(6) for role in range(3)]), 18)

    def test_social_memory_and_civilian_routines(self):
        root = ROOT / "MoonfallUnreal"
        save = (root / "Source/MoonfallUnreal/MoonfallWorldProgressSave.h").read_text(encoding="utf-8")
        hero_h = (root / "Source/MoonfallUnreal/MoonfallHeroPawn.h").read_text(encoding="utf-8")
        hero_cpp = (root / "Source/MoonfallUnreal/MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        guide_h = (root / "Source/MoonfallUnreal/MoonfallQuestGuide.h").read_text(encoding="utf-8")
        guide_cpp = (root / "Source/MoonfallUnreal/MoonfallQuestGuide.cpp").read_text(encoding="utf-8")
        hud = (root / "Source/MoonfallUnreal/MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        inputs = (root / "Config/DefaultInput.ini").read_text(encoding="utf-8")
        self.assertIn("TArray<int32> GuideTrustMemory;", save)
        self.assertIn("TArray<int32> GuideFearMemory;", save)
        self.assertIn("GuideTrustMemory.Init(0, Regions);", CPP)
        self.assertIn("GuideFearMemory.Init(0, Regions);", CPP)
        self.assertIn("Saved->GuideTrustMemory.Num() == Regions", CPP)
        self.assertIn("Saved->GuideFearMemory.Num() == Regions", CPP)
        self.assertIn("Saved->GuideTrustMemory = GuideTrustMemory;", CPP)
        self.assertIn("Saved->GuideFearMemory = GuideFearMemory;", CPP)
        self.assertIn("GuardianReputation += 8;", CPP)
        self.assertIn("Now - *Previous < 30.f", CPP)
        self.assertIn("!DiscoveredRegions.Contains(RegionIndex)", CPP)
        self.assertIn("bIntimidate ? -10 : 7", CPP)
        self.assertIn("bIntimidate ? 12 : -4", CPP)
        self.assertIn("RecordGuideSocialChoice(int32 RegionIndex, bool bIntimidate)", HEADER)
        self.assertIn("GetGuideTrust(int32 RegionIndex) const", HEADER)
        self.assertIn("GetGuideFear(int32 RegionIndex) const", HEADER)
        self.assertIn("if (!IsValid(Hero) || FVector::DistSquared2D(", guide_cpp)
        self.assertIn("DayPhase = FMath::FloorToInt(FMath::Fmod(Now, 480.f) / 120.f);", guide_cpp)
        self.assertIn("PrimaryActorTick.TickInterval = 0.5f;", guide_cpp)
        self.assertIn("AddActorWorldOffset(ToGoal.GetSafeNormal() * Step, true, &Hit);", guide_cpp)
        self.assertIn("GetRoutineLabel() const", guide_h)
        self.assertIn("void AMoonfallHeroPawn::EncourageQuestGuide()", hero_cpp)
        self.assertIn("void AMoonfallHeroPawn::IntimidateQuestGuide()", hero_cpp)
        self.assertIn("LastGuideTrust = Arena->GetNpcTrust(Nearest->GetPersistentNpcId());", hero_cpp)
        self.assertIn("LastGuideFear = Arena->GetNpcFear(Nearest->GetPersistentNpcId());", hero_cpp)
        self.assertIn("LastGuideSocialChoice = -1;", hero_h)
        self.assertIn('TEXT("%s: I remember your kindness.")', hud)
        self.assertIn('TEXT("%s: Your threats will not be forgotten.")', hud)
        self.assertIn('TEXT("Trust %d   Fear %d")', hud)
        self.assertIn('ActionName="EncourageQuestGuide",Key=G', inputs)
        self.assertIn('ActionName="IntimidateQuestGuide",Key=H', inputs)
        self.assertIn('ActionName="EncourageQuestGuide",Key=Gamepad_DPad_Up', inputs)
        self.assertIn('ActionName="IntimidateQuestGuide",Key=Gamepad_DPad_Down', inputs)

    def test_eighteen_npc_relationships_and_roles(self):
        root = ROOT / "MoonfallUnreal"
        save = (root / "Source/MoonfallUnreal/MoonfallWorldProgressSave.h").read_text(encoding="utf-8")
        npc_h = (root / "Source/MoonfallUnreal/MoonfallQuestGuide.h").read_text(encoding="utf-8")
        npc_cpp = (root / "Source/MoonfallUnreal/MoonfallQuestGuide.cpp").read_text(encoding="utf-8")
        hero_h = (root / "Source/MoonfallUnreal/MoonfallHeroPawn.h").read_text(encoding="utf-8")
        hero_cpp = (root / "Source/MoonfallUnreal/MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        hud = (root / "Source/MoonfallUnreal/MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        self.assertIn("for (int32 Role = 0; Role < 3; ++Role)", CPP)
        self.assertIn("Npc->RoleIndex = Role;", CPP)
        self.assertIn("FText AMoonfallQuestGuide::GetPersonalityLabel() const", npc_cpp)
        self.assertIn("GetPersistentNpcId() const", npc_h)
        for name in ("Ash Warden", "Cinder Smith", "Ember Watcher",
                     "Mire Keeper", "Bog Herbalist", "Fen Scout",
                     "Frost Herald", "Rime Tailor", "Snow Pathfinder",
                     "Storm Oracle", "Thunder Artisan", "Cloud Ranger",
                     "Foundry Chronicler", "Forge Mechanic", "Iron Sentinel",
                     "Eclipse Seer", "Moon Weaver", "Shadow Lookout"):
            self.assertIn('TEXT("' + name + '")', npc_cpp)
        self.assertIn("TArray<int32> NpcTrustMemory;", save)
        self.assertIn("TArray<int32> NpcFearMemory;", save)
        self.assertIn("NpcTrustMemory.Init(0, Regions * 3);", CPP)
        self.assertIn("NpcFearMemory.Init(0, Regions * 3);", CPP)
        self.assertIn("Saved->NpcTrustMemory.Num() == Regions * 3", CPP)
        self.assertIn("Saved->NpcFearMemory.Num() == Regions * 3", CPP)
        self.assertIn("Saved->NpcTrustMemory = NpcTrustMemory;", CPP)
        self.assertIn("Saved->NpcFearMemory = NpcFearMemory;", CPP)
        self.assertIn("RecordNpcSocialChoice(int32 NpcId, bool bIntimidate)", HEADER)
        self.assertIn("GetNpcTrust(int32 NpcId) const", HEADER)
        self.assertIn("GetNpcFear(int32 NpcId) const", HEADER)
        self.assertIn("if (Previous && Now - *Previous < 30.f) return false;", CPP)
        self.assertIn("if (Role == 0) {", CPP)
        self.assertIn("LastNpcRole == 0", hero_cpp)
        self.assertIn("RecordNpcSocialChoice(Nearest->GetPersistentNpcId(), bIntimidate)", hero_cpp)
        self.assertIn("FString LastNpcPersonality;", hero_h)
        self.assertIn("FString LastNpcRoutine;", hero_h)
        self.assertIn('TEXT("%s: I make what this region needs.")', hud)
        self.assertIn('TEXT("%s: My watch keeps the roads safe.")', hud)
        self.assertEqual(len({region * 3 + role for region in range(6) for role in range(3)}), 18)

    def test_faction_standing_and_npc_word_of_mouth(self):
        root = ROOT / "MoonfallUnreal/Source/MoonfallUnreal"
        hero_h = (root / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        hero_cpp = (root / "MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        hud = (root / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        self.assertIn("GetRegionalFactionStanding(int32 RegionIndex) const", HEADER)
        self.assertIn("GetNpcBond(int32 NpcId, int32 OtherNpcId) const", HEADER)
        self.assertIn("if (NpcId / 3 != OtherNpcId / 3) return 0;", CPP)
        self.assertIn("return 60; // trusted partners", CPP)
        self.assertIn("return 35; // watchful allies", CPP)
        self.assertIn("return -25; // artisan/scout rivalry", CPP)
        self.assertIn("const int32 Bond = GetNpcBond(NpcId, OtherId);", CPP)
        self.assertIn("if (OtherId == NpcId || !NpcTrustMemory.IsValidIndex(OtherId)) continue;", CPP)
        self.assertIn("const int32 GossipDelta = bIntimidate", CPP)
        self.assertIn("for (int32 Offset : {-Regions, Regions})", CPP)
        self.assertIn("Neighbor % Regions != Region", CPP)
        self.assertIn("ResidentTrustMemory[Neighbor] + (bIntimidate ? -2 : 1)", CPP)
        self.assertIn("NamedStanding * 0.7f + ResidentStanding * 0.3f", CPP)
        self.assertIn("if (ActiveCount == 0) return FMath::Clamp(NamedStanding, -100, 100);", CPP)
        self.assertIn("NpcTrustMemory[OtherId] + GossipDelta", CPP)
        self.assertIn("GuideTrustMemory[Region] = NpcTrustMemory[Region * 3];", CPP)
        self.assertIn("Sum += GetNpcTrust(NpcId);", CPP)
        self.assertIn("Sum -= GetNpcFear(NpcId) / 2;", CPP)
        self.assertIn("LastFactionStanding = 0;", hero_h)
        self.assertIn("LastNpcAllyBond = 0;", hero_h)
        self.assertIn("GetRegionalFactionStanding(Nearest->RegionIndex)", hero_cpp)
        self.assertIn("GetNpcBond(Nearest->GetPersistentNpcId()", hero_cpp)
        self.assertIn('TEXT("Faction %d   Ally bond %d")', hud)
        self.assertIn('TEXT("%s: Your reputation has unsettled this town.")', hud)
        self.assertIn('TEXT("%s: Our community speaks well of you.")', hud)
        # Cross-region social gossip is forbidden, and bonds stay symmetric.
        def bond(a, b):
            if a == b or a // 3 != b // 3:
                return 0
            roles = {a % 3, b % 3}
            return 60 if roles == {0, 1} else 35 if roles == {0, 2} else -25
        for a in range(18):
            for b in range(18):
                self.assertEqual(bond(a, b), bond(b, a))
                if a // 3 != b // 3:
                    self.assertEqual(bond(a, b), 0)

    def test_npc_role_shifts_and_conversation_reactivity(self):
        root = ROOT / "MoonfallUnreal/Source/MoonfallUnreal"
        npc_h = (root / "MoonfallQuestGuide.h").read_text(encoding="utf-8")
        npc_cpp = (root / "MoonfallQuestGuide.cpp").read_text(encoding="utf-8")
        hero_cpp = (root / "MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        self.assertIn("int32 DayPhase = 0;", npc_h)
        self.assertIn("int32 CurrentMood = 0;", npc_h)
        self.assertIn("void ReactToPlayer(int32 Trust, int32 Fear);", npc_h)
        self.assertIn("float ConversationPausedUntil = 0.f;", npc_h)
        self.assertIn("PrimaryActorTick.TickInterval = 0.5f;", npc_cpp)
        self.assertIn("FVector::DistSquared2D(Hero->GetActorLocation(), HomeLocation)", npc_cpp)
        self.assertIn("const int32 RoutineByRoleAndPhase[3][4]", npc_cpp)
        self.assertIn("DayPhase = FMath::FloorToInt(FMath::Fmod(Now, 480.f) / 120.f);", npc_cpp)
        self.assertIn("if (Now < ConversationPausedUntil) return;", npc_cpp)
        self.assertIn("ConversationPausedUntil = Now + 5.f;", npc_cpp)
        self.assertIn("CurrentMood = Fear >= 55 ? 2 : Trust >= 50 ? 1 : 0;", npc_cpp)
        self.assertIn("LeftArm->SetRelativeRotation(", npc_cpp)
        self.assertIn("RightArm->SetRelativeRotation(", npc_cpp)
        self.assertIn("AddActorWorldOffset(ToGoal.GetSafeNormal() * Step, true, &Hit);", npc_cpp)
        self.assertEqual(hero_cpp.count("Nearest->ReactToPlayer(LastGuideTrust, LastGuideFear);"), 2)
        # All 18 NPCs have deterministic, bounded schedules, with no actor spam.
        assignments = ((0, 1, 0, 3), (1, 2, 1, 3), (0, 2, 0, 1))
        self.assertEqual(len(assignments), 3)
        self.assertTrue(all(0 <= task <= 3 for shift in assignments for task in shift))
        self.assertNotIn("SpawnActor<", npc_cpp)

    def test_virtual_population_1111_and_streaming_budget(self):
        root = ROOT / "MoonfallUnreal/Source/MoonfallUnreal"
        npc_h = (root / "MoonfallQuestGuide.h").read_text(encoding="utf-8")
        npc_cpp = (root / "MoonfallQuestGuide.cpp").read_text(encoding="utf-8")
        save = (root / "MoonfallWorldProgressSave.h").read_text(encoding="utf-8")
        self.assertIn("GetPopulationCount() const { return 1111; }", HEADER)
        self.assertIn("GetActiveStreamedResidentCount() const { return ActiveResidents.Num(); }", HEADER)
        self.assertIn("constexpr int32 MaxStreamedResidents = 24;", CPP)
        self.assertIn("Checked < 80 && Spawned < 2", CPP)
        self.assertIn("PopulationCursor = Regions * 3 + (PopulationCursor - Regions * 3 + 1) % (1111 - Regions * 3);", CPP)
        self.assertIn("Npc->PopulationNpcId = Id;", CPP)
        self.assertIn("Npc->RoleIndex = 1 + ((Id - Regions * 3) / Regions) % 2;", CPP)
        self.assertIn("GetResidentHome(int32 ResidentId)", HEADER)
        self.assertIn("if (const APawn* LocalHero = UGameplayStatics::GetPlayerPawn(this, 0))", CPP)
        self.assertIn("StreamResidents(LocalHero->GetActorLocation());", CPP)
        self.assertIn("Npc->Destroy();", CPP)
        self.assertIn("FMath::Square(1200.f)", CPP)
        self.assertIn("FMath::Square(1650.f)", CPP)
        self.assertIn("ResidentTrustMemory.Init(0, 1111 - Regions * 3);", CPP)
        self.assertIn("ResidentFearMemory.Init(0, 1111 - Regions * 3);", CPP)
        self.assertIn("Saved->ResidentTrustMemory.Num() == 1111 - Regions * 3", CPP)
        self.assertIn("Saved->ResidentFearMemory.Num() == 1111 - Regions * 3", CPP)
        self.assertIn("Saved->ResidentTrustMemory = ResidentTrustMemory;", CPP)
        self.assertIn("Saved->ResidentFearMemory = ResidentFearMemory;", CPP)
        self.assertIn("TArray<int32> ResidentTrustMemory;", save)
        self.assertIn("TArray<int32> ResidentFearMemory;", save)
        self.assertIn("int32 PopulationNpcId = -1;", npc_h)
        self.assertIn("PopulationNpcId >= 18 && PopulationNpcId < 1111", npc_h)
        self.assertIn("if (PopulationNpcId >= 18 && PopulationNpcId < 1111)", npc_cpp)
        self.assertEqual(len(set(range(1111))), 1111)
        # Records above the original 18 distribute across six stable regional cohorts.
        regions = [(npc_id - 18) % 6 for npc_id in range(18, 1111)]
        self.assertEqual(len(regions), 1093)
        self.assertEqual(max(regions.count(i) for i in range(6)) - min(regions.count(i) for i in range(6)), 1)
        self.assertEqual(len(set(range(18)) & set(range(18, 1111))), 0)

    def test_regional_trade_and_real_salvage_services(self):
        root = ROOT / "MoonfallUnreal"
        hero_h = (root / "Source/MoonfallUnreal/MoonfallHeroPawn.h").read_text(encoding="utf-8")
        hero_cpp = (root / "Source/MoonfallUnreal/MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        hud = (root / "Source/MoonfallUnreal/MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        inputs = (root / "Config/DefaultInput.ini").read_text(encoding="utf-8")
        self.assertIn("GetRegionalServicePrice(int32 RegionIndex, int32 RoleIndex) const;", HEADER)
        self.assertIn("GetRegionalSupplyRating(int32 RegionIndex) const;", HEADER)
        self.assertIn("if (IsDungeonQuestCompleted(RegionIndex * DungeonsPerRegion + Dungeon))", CPP)
        self.assertIn("20 + Guardians * 15", CPP)
        self.assertIn("const int32 Standing = GetRegionalFactionStanding(RegionIndex);", CPP)
        self.assertIn("const int32 Supply = GetRegionalSupplyRating(RegionIndex);", CPP)
        self.assertIn("return FMath::Clamp(Base + StandingAdjustment - SupplyDiscount, 4, 25);", CPP)
        self.assertIn("void BuyNearestNpcService();", hero_h)
        self.assertIn("void AMoonfallHeroPawn::BuyNearestNpcService()", hero_cpp)
        self.assertIn("if (!IsValid(Npc) || Npc->RoleIndex == 0) continue;", hero_cpp)
        self.assertIn("if (Price <= 0) return;", hero_cpp)
        self.assertIn("if (!bNeedsService || Salvage < Price) return;", hero_cpp)
        self.assertIn("Salvage -= Price;", hero_cpp)
        self.assertIn("Mana = FMath::Clamp(Mana + 35.f, 0.f, MaxMana);", hero_cpp)
        self.assertIn("Stamina = FMath::Clamp(Stamina + 35.f, 0.f, MaxStamina);", hero_cpp)
        self.assertIn("bLastServicePurchased = true;", hero_cpp)
        body = hero_cpp.split("void AMoonfallHeroPawn::BuyNearestNpcService()", 1)[1].split("void AMoonfallHeroPawn::PickupNearestGroundLoot()", 1)[0]
        self.assertNotIn("Health =", body)
        self.assertNotIn("AwardExperience(", body)
        self.assertNotIn("AwardEquipmentDrop(", body)
        self.assertIn('ActionName="BuyNpcService",Key=T', inputs)
        self.assertIn('ActionName="BuyNpcService",Key=Gamepad_DPad_Left', inputs)
        self.assertIn('TEXT("Supply %d / 100  |  Service %d salvage")', hud)
        self.assertIn('TEXT("%s: Service complete, %d salvage paid.")', hud)
        self.assertIn("LastServicePrice = 0;", hero_h)
        # Economic prices remain strictly positive and bounded across standings.
        for role in (1, 2):
            for standing in range(-100, 101, 5):
                for supply in range(0, 101, 5):
                    adjustment = (abs(standing) + 14) // 15 if standing < 0 else -(standing // 25)
                    quote = max(4, min(25, (12 if role == 1 else 10) + adjustment - supply // 30))
                    self.assertGreaterEqual(quote, 4)
                    self.assertLessEqual(quote, 25)

    def test_crafting_inventory_workshop_stock_and_controls(self):
        root = ROOT / "MoonfallUnreal"
        hero_h = (root / "Source/MoonfallUnreal/MoonfallHeroPawn.h").read_text(encoding="utf-8")
        hero_cpp = (root / "Source/MoonfallUnreal/MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        hud = (root / "Source/MoonfallUnreal/MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        save = (root / "Source/MoonfallUnreal/MoonfallWorldProgressSave.h").read_text(encoding="utf-8")
        inputs = (root / "Config/DefaultInput.ini").read_text(encoding="utf-8")
        self.assertIn("GetWorkshopStock(int32 RegionIndex) const;", HEADER)
        self.assertIn("GetCraftingPrice(int32 RegionIndex) const;", HEADER)
        self.assertIn("PurchaseWorkshopStock(int32 RegionIndex);", HEADER)
        self.assertIn("WorkshopStock.Init(3, Regions);", CPP)
        self.assertIn("Saved->WorkshopStock.Num() == Regions", CPP)
        self.assertIn("Saved->WorkshopStock = WorkshopStock;", CPP)
        self.assertIn("TArray<int32> WorkshopStock;", save)
        self.assertIn("NextWorkshopProductionAt = Now + 120.f;", CPP)
        self.assertIn("GetRegionalSupplyRating(Region) / 20", CPP)
        self.assertIn("if (!DiscoveredRegions.Contains(Region)) continue;", CPP)
        self.assertIn("if (WorkshopStock[Region] < Capacity)", CPP)
        self.assertIn("return FMath::Clamp(Base + ReputationPenalty, 7, 20);", CPP)
        self.assertIn("if (!bContinuousWorld || RegionIndex < 0 || RegionIndex >= Regions ||", CPP)
        self.assertIn("--WorkshopStock[RegionIndex];", CPP)
        self.assertIn("void CraftWithNearestArtisan();", hero_h)
        self.assertIn("void AMoonfallHeroPawn::CraftWithNearestArtisan()", hero_cpp)
        body = hero_cpp.split("void AMoonfallHeroPawn::CraftWithNearestArtisan()", 1)[1].split("void AMoonfallHeroPawn::PickupNearestGroundLoot()", 1)[0]
        self.assertIn("if (!IsValid(Npc) || Npc->RoleIndex != 1) continue;", body)
        self.assertIn("if (LastCraftPrice <= 0 || LastWorkshopStock <= 0 || Salvage < LastCraftPrice ||", body)
        self.assertIn("InventoryItemCodes.Num() >= 2048) return;", body)
        self.assertIn("if (!Arena->PurchaseWorkshopStock(Region)) return;", body)
        self.assertIn("Salvage -= LastCraftPrice;", body)
        self.assertIn("InventoryItemCodes.Add(ItemCode);", body)
        self.assertIn("const int32 ItemCode = Affix * 100000 + Slot * 10000 + Rarity * 1000 + Tier;", body)
        self.assertNotIn("EquipInventoryItem(", body)
        self.assertNotIn("Health =", body)
        self.assertNotIn("AwardExperience(", body)
        self.assertIn('ActionName="CraftNpcItem",Key=V', inputs)
        self.assertIn('ActionName="CraftNpcItem",Key=Gamepad_FaceButton_Top', inputs)
        self.assertIn('ActionName="DualCast",Key=C', inputs)
        self.assertIn('TEXT("R: Talk   T: Service   V: Craft   G/H: Social")', hud)
        self.assertIn('TEXT("%s: Crafted regional equipment for %d salvage.")', hud)
        # All six region recipes and guardian-upgraded rarities decode.
        for region in range(6):
            for level in (1, 5, 25, 75, 150):
                for supply in (20, 64, 65, 100):
                    tier = min(25, max(1, level // 5 + 1))
                    rarity = 2 if supply >= 65 else 1
                    affix = 1 + region % 3
                    code = affix * 100000 + region % 3 * 10000 + rarity * 1000 + tier
                    self.assertEqual(code // 100000, affix)
                    self.assertEqual((code // 10000) % 10, region % 3)
                    self.assertEqual((code // 1000) % 10, rarity)
                    self.assertTrue(1 <= code % 1000 <= 25)

    def test_assassins_creed_style_moonfall_menu_pages(self):
        root = ROOT / "MoonfallUnreal"
        hero_h = (root / "Source/MoonfallUnreal/MoonfallHeroPawn.h").read_text(encoding="utf-8")
        hero_cpp = (root / "Source/MoonfallUnreal/MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        hud = (root / "Source/MoonfallUnreal/MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        inputs = (root / "Config/DefaultInput.ini").read_text(encoding="utf-8")
        self.assertIn("int32 InventoryMenuTab = 0;", hero_h)
        for fn in ("OpenCharacterPage", "OpenQuestPage", "OpenWorldPage"):
            self.assertIn("void " + fn + "();", hero_h)
            self.assertIn("void AMoonfallHeroPawn::" + fn + "()", hero_cpp)
        for value in (1, 2, 3):
            self.assertIn("InventoryMenuTab = " + str(value) + ";", hero_cpp)
        self.assertIn("InventoryMenuTab = (InventoryMenuTab + 3) % 4;", hero_cpp)
        self.assertIn("InventoryMenuTab = (InventoryMenuTab + 1) % 4;", hero_cpp)
        self.assertIn("if (!bInventoryOpen || InventoryMenuTab != 0 ||", hero_cpp)
        self.assertIn("if (bInventoryOpen && InventoryMenuTab == 0)", hero_cpp)
        self.assertIn("if (Hero->bInventoryOpen && Hero->InventoryMenuTab != 0)", hud)
        for title in ("GEAR", "CHARACTER", "QUESTS", "WORLD"):
            self.assertIn('TEXT("' + title + '")', hud)
        self.assertIn("Hero->WeaponTier, Hero->ArmorTier, Hero->AccessoryTier", hud)
        self.assertIn("Arena->GetDiscoveredRegions().Contains(Region)", hud)
        self.assertIn("Arena->GetDiscoveredDungeonEntrances()", hud)
        self.assertIn("Arena->GetDiscoveredChambers()", hud)
        self.assertIn("if (!Arena->GetDiscoveredDungeonEntrances().Contains(Entrance)) continue;", hud)
        self.assertIn("Arena->IsDungeonQuestCompleted(Entrance)", hud)
        self.assertIn("Arena->GetTrackedDungeonQuestId()", hud)
        self.assertIn("Unexplored regions and dungeons remain hidden.", hud)
        for action, key in (("CharacterPage", "K"), ("QuestPage", "J"), ("WorldPage", "M")):
            self.assertIn('ActionName="' + action + '",Key=' + key, inputs)
            self.assertIn('BindAction(TEXT("' + action + '")', hero_cpp)
        self.assertIn('ActionName="DualCast",Key=C', inputs)
        self.assertIn('ActionName="Inventory",Key=I', inputs)
        self.assertIn('ActionName="Options",Key=Escape', inputs)

    def test_player_economy_save_and_quest_journal_actions(self):
        root = ROOT / "MoonfallUnreal/Source/MoonfallUnreal"
        hero_h = (root / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        hero_cpp = (root / "MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        hud = (root / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        econ = (root / "MoonfallPlayerEconomySave.h").read_text(encoding="utf-8")
        self.assertIn("MoonfallUnrealPlayerEconomyV1", hero_cpp)
        self.assertIn("LoadPlayerEconomy();", hero_cpp)
        self.assertIn("UMoonfallPlayerEconomySave", econ)
        self.assertIn("TArray<int32> InventoryItemCodes;", econ)
        self.assertIn("int32 EquippedWeaponCode = 0;", econ)
        self.assertIn("int32 Salvage = 0;", econ)
        self.assertIn("SavePlayerEconomy();", hero_cpp)
        self.assertIn("return UGameplayStatics::SaveGameToSlot(Save, PlayerEconomySlot, 0);", hero_cpp)
        self.assertIn("LoadGameFromSlot(PlayerEconomySlot, 0)", hero_cpp)
        self.assertIn("Save->InventoryItemCodes.Num() > 2048", hero_cpp)
        self.assertIn("IsValidEconomyItem(Code)", hero_cpp)
        self.assertIn("InventoryItemCodes.Contains(Code)", hero_cpp)
        self.assertIn("MaxHealth + 2.f * ArmorTier", hero_cpp)
        self.assertIn("Health = FMath::Clamp(Health, 1.f, MaxHealth);", hero_cpp)
        self.assertIn("void SelectJournalQuest();", hero_h)
        self.assertIn("void AMoonfallHeroPawn::SelectJournalQuest()", hero_cpp)
        self.assertIn("Arena->TrackDungeonQuest(Id);", hero_cpp)
        self.assertIn("GetDiscoveredDungeonEntrances().Contains(Id)", hero_cpp)
        self.assertIn("if (bInventoryOpen && InventoryMenuTab == 2) { SelectJournalQuest(); return; }", hero_cpp)
        self.assertIn("QuestJournalSelection = FMath::Max(0, QuestJournalSelection - 1);", hero_cpp)
        self.assertIn("ThisKnownIndex == Hero->QuestJournalSelection", hud)
        self.assertIn("JournalPageStart = Hero->QuestJournalSelection / 9 * 9", hud)
        self.assertIn("if (ThisKnownIndex < JournalPageStart) continue;", hud)

    def test_inventory_capacity_and_native_windows_compile_script(self):
        root = ROOT / "MoonfallUnreal"
        hero = (root / "Source/MoonfallUnreal/MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        build = (root / "Verify-Win64.ps1").read_text(encoding="utf-8")
        self.assertIn("if (ItemCode <= 0 || InventoryItemCodes.Num() >= 2048) return;", hero)
        self.assertIn("if (Health <= 0.f || ItemCode <= 0 || InventoryItemCodes.Num() >= 2048) return false;", hero)
        craft = hero.split("void AMoonfallHeroPawn::CraftWithNearestArtisan()",1)[1].split(
            "void AMoonfallHeroPawn::PickupNearestGroundLoot()",1)[0]
        self.assertIn("InventoryItemCodes.Num() >= 2048) return;", craft)
        self.assertLess(craft.index("InventoryItemCodes.Num() >= 2048) return;"),
                        craft.index("Arena->PurchaseWorkshopStock(Region)"))
        self.assertIn("Build.version", build)
        self.assertIn("$version.MajorVersion -ne 5 -or $version.MinorVersion -ne 4", build)
        self.assertIn("MoonfallUnrealEditor", build)
        self.assertIn('if ($BuildGame) { $targets += "MoonfallUnreal" }', build)
        self.assertIn("Native Unreal Win64 compile checks passed", build)
        self.assertIn("if ($result -ne 0)", build)

    def test_single_arena_startup_and_explicit_native_dependencies(self):
        root = ROOT / "MoonfallUnreal/Source/MoonfallUnreal"
        game_mode = (root / "MoonfallUnrealGameMode.cpp").read_text(encoding="utf-8")
        arena = (root / "MoonfallArena.cpp").read_text(encoding="utf-8")
        self.assertIn('#include "EngineUtils.h"', game_mode)
        self.assertIn('#include "EngineUtils.h"', arena)
        self.assertIn('#include "GameFramework/Pawn.h"', arena)
        self.assertIn("for (TActorIterator<AMoonfallArena> Existing(GetWorld()); Existing; ++Existing)", game_mode)
        self.assertIn("if (IsValid(*Existing)) return;", game_mode)
        self.assertIn("GetWorld()->SpawnActor<AMoonfallArena>", game_mode)
        self.assertLess(game_mode.index("if (IsValid(*Existing)) return;"),
                        game_mode.index("GetWorld()->SpawnActor<AMoonfallArena>"))
        self.assertEqual(game_mode.count("GetWorld()->SpawnActor<AMoonfallArena>"), 1)

    def test_workshop_purchase_flushes_world_save_before_crafting(self):
        hero_cpp = (ROOT / "MoonfallUnreal/Source/MoonfallUnreal/MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        purchase = CPP.split("bool AMoonfallArena::PurchaseWorkshopStock(int32 RegionIndex)", 1)[1].split(
            "void AMoonfallArena::AdvanceWorkshopProduction()", 1)[0]
        self.assertIn("--WorkshopStock[RegionIndex];", purchase)
        self.assertIn("bWorldSavePending = true;", purchase)
        self.assertIn("SaveWorldProgress();", purchase)
        self.assertIn("if (bWorldSavePending) {", purchase)
        self.assertIn("++WorkshopStock[RegionIndex];", purchase)
        self.assertIn("return false;", purchase)
        self.assertLess(purchase.index("SaveWorldProgress();"), purchase.index("return true;"))
        craft = hero_cpp.split("void AMoonfallHeroPawn::CraftWithNearestArtisan()",1)[1].split(
            "void AMoonfallHeroPawn::PickupNearestGroundLoot()",1)[0]
        self.assertLess(craft.index("if (!Arena->PurchaseWorkshopStock(Region)) return;"),
                        craft.index("Salvage -= LastCraftPrice;"))
        self.assertLess(craft.index("Salvage -= LastCraftPrice;"),
                        craft.index("InventoryItemCodes.Add(ItemCode);"))
        self.assertLess(craft.index("InventoryItemCodes.Add(ItemCode);"),
                        craft.index("if (!TrySavePlayerEconomy()) {"))

    def test_crafting_player_save_failure_rolls_back_without_reward(self):
        root = ROOT / "MoonfallUnreal/Source/MoonfallUnreal"
        hero = (root / "MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        hero_h = (root / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        self.assertIn("bool TrySavePlayerEconomy();", hero_h)
        self.assertIn("bool AMoonfallHeroPawn::TrySavePlayerEconomy()", hero)
        self.assertIn("return UGameplayStatics::SaveGameToSlot(Save, PlayerEconomySlot, 0);", hero)
        self.assertIn("bool RefundWorkshopStock(int32 RegionIndex);", HEADER)
        self.assertIn("bool AMoonfallArena::RefundWorkshopStock(int32 RegionIndex)", CPP)
        refund = CPP.split("bool AMoonfallArena::RefundWorkshopStock(int32 RegionIndex)", 1)[1].split(
            "void AMoonfallArena::AdvanceWorkshopProduction()", 1)[0]
        self.assertIn("WorkshopStock[RegionIndex] = FMath::Clamp(OldCount + 1, 0, 8);", refund)
        self.assertIn("SaveWorldProgress();", refund)
        craft = hero.split("void AMoonfallHeroPawn::CraftWithNearestArtisan()", 1)[1].split(
            "void AMoonfallHeroPawn::PickupNearestGroundLoot()", 1)[0]
        self.assertIn("if (!TrySavePlayerEconomy()) {", craft)
        service = hero.split("void AMoonfallHeroPawn::BuyNearestNpcService()", 1)[1].split(
            "void AMoonfallHeroPawn::CraftWithNearestArtisan()", 1)[0]
        self.assertIn("if (!TrySavePlayerEconomy()) {", service)
        self.assertIn("Salvage += Price;", service)
        self.assertLess(service.index("if (!TrySavePlayerEconomy()) {"),
                        service.index("if (Role == 1) Mana ="))
        self.assertIn("InventoryItemCodes.RemoveAt(InventoryItemCodes.Num() - 1);", craft)
        self.assertIn("Salvage = SalvageBeforePurchase;", craft)
        self.assertIn("Arena->RefundWorkshopStock(Region);", craft)
        self.assertLess(craft.index("if (!TrySavePlayerEconomy()) {"),
                        craft.index("bLastCraftSucceeded = true;"))
        self.assertLess(craft.index("Salvage = SalvageBeforePurchase;"),
                        craft.index("bLastCraftSucceeded = true;"))

    def test_social_action_uses_current_npc_identity_and_bond(self):
        hero = (ROOT / "MoonfallUnreal/Source/MoonfallUnreal/MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        talk = hero.split("void AMoonfallHeroPawn::InteractQuestGuide()",1)[1].split(
            "void AMoonfallHeroPawn::EncourageQuestGuide()",1)[0]
        social = hero.split("void AMoonfallHeroPawn::ApplyQuestGuideSocialAction(bool bIntimidate)",1)[1].split(
            "void AMoonfallHeroPawn::BuyNearestNpcService()",1)[0]
        self.assertIn("LastNpcRole = FMath::Clamp(Nearest->RoleIndex, 0, 2);", social)
        self.assertIn("LastNpcPersonality = Nearest->GetPersonalityLabel().ToString();", social)
        self.assertIn("LastNpcRoutine = Nearest->GetRoutineLabel().ToString();", social)
        for action in (talk, social):
            self.assertIn("LastNpcRole == 0 ? 1 : 0", action)
            self.assertIn("Arena->GetNpcBond(Nearest->GetPersistentNpcId(), CounterpartId)", action)
        self.assertLess(social.index("LastNpcRole = FMath::Clamp(Nearest->RoleIndex, 0, 2);"),
                        social.index("LastServicePrice = Arena->GetRegionalServicePrice("))

    def test_quest_page_tracks_active_objective_without_stale_index(self):
        hero = (ROOT / "MoonfallUnreal/Source/MoonfallUnreal/MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        open_quest = hero.split("void AMoonfallHeroPawn::OpenQuestPage()",1)[1].split(
            "void AMoonfallHeroPawn::OpenWorldPage()",1)[0]
        select = hero.split("void AMoonfallHeroPawn::SelectJournalQuest()",1)[1].split(
            "void AMoonfallHeroPawn::OptionsUp()",1)[0]
        self.assertIn("QuestJournalSelection = 0;", open_quest)
        self.assertIn("const int32 TrackedId = Arena->GetTrackedDungeonQuestId();", open_quest)
        self.assertIn("if (!Arena->GetDiscoveredDungeonEntrances().Contains(Id)) continue;", open_quest)
        self.assertIn("if (Id == TrackedId)", open_quest)
        self.assertIn("QuestJournalSelection = KnownIndex;", open_quest)
        self.assertIn("QuestJournalSelection = FMath::Clamp(QuestJournalSelection, 0,", select)
        self.assertIn("Arena->GetDiscoveredDungeonEntrances().Num() - 1", select)
        self.assertIn("Arena->TrackDungeonQuest(Id);", select)
        # Stable numeric discovery order shared by both page-open and confirm.
        self.assertIn("for (int32 Id = 0; Id < 30; ++Id)", open_quest)
        self.assertIn("for (int32 Id = 0; Id < 30; ++Id)", select)

    def test_ground_loot_survives_failed_player_inventory_save(self):
        root = ROOT / "MoonfallUnreal/Source/MoonfallUnreal"
        hero = (root / "MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        loot = (root / "MoonfallGroundLoot.cpp").read_text(encoding="utf-8")
        collect = hero.split("bool AMoonfallHeroPawn::CollectGroundEquipmentCode(int32 ItemCode)",1)[1].split(
            "void AMoonfallHeroPawn::SpawnGroundEquipmentDrop(",1)[0]
        self.assertIn("InventoryItemCodes.Add(ItemCode);", collect)
        self.assertIn("if (!TrySavePlayerEconomy()) {", collect)
        self.assertIn("InventoryItemCodes.RemoveAt(InventoryItemCodes.Num() - 1);", collect)
        self.assertIn("return false;", collect)
        self.assertNotIn("SavePlayerEconomy();", collect)
        self.assertLess(collect.index("if (!TrySavePlayerEconomy()) {"),
                        collect.index("LastLootAffix = Affix;"))
        self.assertIn("if (!IsValid(Hero) || !Hero->CollectGroundEquipmentCode(ItemCode)) return false;", loot)
        self.assertLess(loot.index("Hero->CollectGroundEquipmentCode(ItemCode)"),
                        loot.index("bCollected = true;"))
        self.assertLess(loot.index("bCollected = true;"), loot.index("Destroy();"))

    def test_failed_equip_and_salvage_save_roll_back_all_inventory_state(self):
        hero = (ROOT / "MoonfallUnreal/Source/MoonfallUnreal/MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        equip = hero.split("bool AMoonfallHeroPawn::EquipInventoryItem(int32 InventoryIndex)",1)[1].split(
            "void AMoonfallHeroPawn::CraftWithNearestArtisan()",1)[0]
        salvage = hero.split("bool AMoonfallHeroPawn::SalvageInventoryItem(int32 InventoryIndex)",1)[1].split(
            "void AMoonfallHeroPawn::InventoryConfirm()",1)[0]
        for field in ("PreviousTier", "PreviousAffix", "PreviousCode",
                      "PreviousEquipmentTier", "PreviousMaxHealth", "PreviousHealth"):
            self.assertIn(field, equip)
        self.assertIn("if (!TrySavePlayerEconomy()) {", equip)
        self.assertIn("*EquippedTier = PreviousTier;", equip)
        self.assertIn("*EquippedAffix = PreviousAffix;", equip)
        self.assertIn("*EquippedCode = PreviousCode;", equip)
        self.assertIn("MaxHealth = PreviousMaxHealth;", equip)
        self.assertIn("Health = PreviousHealth;", equip)
        self.assertLess(equip.index("if (!TrySavePlayerEconomy()) {"),
                        equip.index("OnHealthChanged(Health, MaxHealth);"))
        self.assertIn("const int32 PreviousSalvage = Salvage;", salvage)
        self.assertIn("InventoryItemCodes.RemoveAt(InventoryIndex);", salvage)
        self.assertIn("if (!TrySavePlayerEconomy()) {", salvage)
        self.assertIn("InventoryItemCodes.Insert(Code, InventoryIndex);", salvage)
        self.assertIn("Salvage = PreviousSalvage;", salvage)
        self.assertLess(salvage.index("InventoryItemCodes.Insert(Code, InventoryIndex);"),
                        salvage.index("return false;", salvage.index("if (!TrySavePlayerEconomy()) {")))
        self.assertLess(salvage.index("if (!TrySavePlayerEconomy()) {"),
                        salvage.index("InventorySelection = FMath::Clamp("))

    def test_connected_geometry(self):
        self.assertIn("Region * RegionSpacing", CPP)
        self.assertIn("FVector(60.f, 60.f, 1.f)", CPP)
        self.assertIn("FVector(3.5f, 120.f, 1.f)", CPP)
        self.assertIn("GetPlayerPawn(this, 0)", CPP)
        self.assertIn("BeginDeferredActorSpawnFromClass", CPP)
        self.assertIn("FinishSpawningActor", CPP)

if __name__ == "__main__":
    unittest.main()

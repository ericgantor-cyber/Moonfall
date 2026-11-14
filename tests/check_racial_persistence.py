#!/usr/bin/env python3
"""Static regression contracts for UE5 racial persistence; native compile still required."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1] / "MoonfallUnreal" / "Source" / "MoonfallUnreal"
INPUT = Path(__file__).resolve().parents[1] / "MoonfallUnreal" / "Config" / "DefaultInput.ini"

class TestRacialPersistence(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.cpp = (ROOT / "MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        cls.header = (ROOT / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        cls.save = (ROOT / "MoonfallPersistentSave.h").read_text(encoding="utf-8")
        cls.input = INPUT.read_text(encoding="utf-8")

    def test_race_roster_and_input(self):
        self.assertIn("constexpr FMoonfallRaceProfile MoonfallRaces[]", self.cpp)
        roster = self.cpp.split("constexpr FMoonfallRaceProfile MoonfallRaces[]", 1)[1].split("};", 1)[0]
        for race in ("Human", "High Elf", "Wood Elf", "Dwarf", "Halfling",
                     "Orc", "Goblin", "Troll", "Entkin", "Shadowborn",
                     "Moonborn", "Draconid"):
            self.assertIn('TEXT("' + race + '")', roster)
        self.assertIn('ActionName="RacePower",Key=T', self.input)
        self.assertIn('ActionName="RacePower",Key=Gamepad_FaceButton_Top', self.input)
        self.assertIn("&AMoonfallHeroPawn::TriggerRacePower", self.cpp)
        self.assertIn("void TriggerRacePower() { ActivateRacePower(); }", self.header)

    def test_race_is_chosen_before_passive_or_active_bonuses(self):
        self.assertIn("if (bRaceChosen || NewRaceId < 0 || NewRaceId > 11", self.cpp)
        for method in ("GetRaceMightBonus", "GetRaceInsightBonus",
                       "GetRaceResolveBonus", "GetRaceStaminaRegenBonus",
                       "GetRaceManaRegenBonus"):
            self.assertIn(method + "() const { return bRaceChosen ?", self.cpp)
        self.assertIn("if (!bRaceChosen || !GetWorld()", self.cpp)

    def test_persistent_cooldown_and_save_migration(self):
        self.assertIn("int32 Version = 7", self.save)
        self.assertIn("int64 RacePowerReadyUtc", self.save)
        self.assertIn("FDateTime::UtcNow().ToUnixTimestamp()", self.cpp)
        self.assertIn("Save->RacePowerReadyUtc =", self.cpp)
        self.assertIn("Save->Version < 1 || Save->Version > 7", self.cpp)
        self.assertIn("Save->Version >= 3 && Save->RacePowerReadyUtc > NowUtc", self.cpp)
        self.assertIn("RacePowerReadyAt = GetWorld()", self.cpp)
        self.assertIn("if (!SaveWorldProgress()) {", self.cpp)
        self.assertIn("RacePowerReadyAt = PreviousReadyAt;", self.cpp)
        self.assertIn("RacePowerActiveUntil = PreviousActiveUntil;", self.cpp)
        self.assertIn("OnRacePowerActivated(FName(MoonfallRacePowerNames[RaceId]));", self.cpp)

    def test_malformed_saves_cannot_restore_unknown_quests_or_scan_unbounded_inventory(self):
        # Both are source contracts until a real UE5 save migration test is run.
        loader = self.cpp.split("bool AMoonfallHeroPawn::LoadWorldProgress()", 1)[1].split(
            "void AMoonfallHeroPawn::RespawnAtCheckpoint()", 1)[0]
        self.assertIn("IsNamedMoonfallResident(Entry.Key)", loader)
        self.assertIn("Entry.Value == 1 || Entry.Value == 2", loader)
        self.assertIn("Save->ResidentQuestStates.Num() <= 64", loader)
        self.assertIn("FMath::Min(Save->InventoryItemCodes.Num(), 10000)", loader)
        self.assertIn("Index < LoadScanCount", loader)
        self.assertIn("ValidMoonfallItemCode(Code)", loader)
        self.assertNotIn("for (int32 Code : Save->InventoryItemCodes)", loader)
        self.assertIn("bool IsNamedMoonfallResident(FName Id);", self.cpp)
        self.assertIn("bool IsNamedMoonfallResident(FName Id) {", self.cpp)

    def test_oversized_persistent_saves_are_write_protected_before_restore(self):
        loader = self.cpp.split("bool AMoonfallHeroPawn::LoadWorldProgress()", 1)[1].split(
            "void AMoonfallHeroPawn::RespawnAtCheckpoint()", 1)[0]
        for expression in (
            "Save->InventoryItemCodes.Num() > 10000",
            "Save->Version >= 6 && Save->ResidentQuestStates.Num() > 64",
        ):
            self.assertIn(expression, loader)
        reject = loader.split("if (Save->InventoryItemCodes.Num() > 10000", 1)[1].split(
            "// Validate the V7 discovered-shrine ledger", 1)[0]
        self.assertIn("bPersistentSaveWritesBlocked = true;", reject)
        self.assertIn("return false;", reject)
        self.assertLess(loader.index("Save->InventoryItemCodes.Num() > 10000"),
                        loader.index("TeleportTo(SavedCheckpoint"))
        self.assertLess(loader.index("Save->InventoryItemCodes.Num() > 10000"),
                        loader.index("ResidentQuestStates.Reset();"))
        self.assertLess(loader.index("Save->InventoryItemCodes.Num() > 10000"),
                        loader.index("InventoryItemCodes.Reset();"))
        saver = self.cpp.split("bool AMoonfallHeroPawn::SaveWorldProgress()", 1)[1].split(
            "bool AMoonfallHeroPawn::LoadWorldProgress()", 1)[0]
        self.assertIn("bPersistentSaveWritesBlocked || !bHasRespawnCheckpointLocation", saver)
        self.assertIn("UGameplayStatics::SaveGameToSlot", saver)

    def test_future_version_save_is_not_overwritten_by_autosave(self):
        self.assertIn("bool bPersistentSaveWritesBlocked = false;", self.header)
        save = self.cpp.split("bool AMoonfallHeroPawn::SaveWorldProgress()", 1)[1].split(
            "bool AMoonfallHeroPawn::LoadWorldProgress()", 1)[0]
        load = self.cpp.split("bool AMoonfallHeroPawn::LoadWorldProgress()", 1)[1].split(
            "void AMoonfallHeroPawn::RespawnAtCheckpoint()", 1)[0]
        self.assertIn("if (bPersistentSaveWritesBlocked || !bHasRespawnCheckpointLocation", save)
        self.assertLess(save.index("if (bPersistentSaveWritesBlocked || !bHasRespawnCheckpointLocation"),
                        save.index("UGameplayStatics::SaveGameToSlot("))
        self.assertIn("Save->Version < 1 || Save->Version > 7", load)
        self.assertIn("bPersistentSaveWritesBlocked = true;", load)
        self.assertIn("bPersistentSaveWritesBlocked = false;", load)
        self.assertLess(load.index("bPersistentSaveWritesBlocked = true;"),
                        load.index("ResidentQuestStates.Reset();"))
        self.assertLess(load.index("bPersistentSaveWritesBlocked = false;"),
                        load.index("ResidentQuestStates.Reset();"))
        self.assertIn("LoadWorldProgress();", self.cpp.split("void AMoonfallHeroPawn::BeginPlay()", 1)[1].split(
            "void AMoonfallHeroPawn::Tick(", 1)[0])
        self.assertIn("if (SaveWorldProgress()) SecondsSinceLastWorldSave = 0.f;", self.cpp)

    def test_checkpoint_respawn_rejects_invalid_or_blocked_spawns(self):
        loader = self.cpp.split("bool AMoonfallHeroPawn::LoadWorldProgress()", 1)[1].split(
            "void AMoonfallHeroPawn::RespawnAtCheckpoint()", 1)[0]
        respawn = self.cpp.split("void AMoonfallHeroPawn::RespawnAtCheckpoint()", 1)[1].split(
            "// One unified roster.", 1)[0]
        helper = self.cpp.split("bool RestoreMoonfallPlayerStart(", 1)[1].split(
            "bool AMoonfallHeroPawn::SaveWorldProgress()", 1)[0]
        self.assertIn('#include "GameFramework/PlayerStart.h"', self.cpp)
        self.assertIn("TActorIterator<APlayerStart>", helper)
        fallback = self.cpp.split("bool AMoonfallHeroPawn::RestoreSafeSpawnLocation()", 1)[1].split(
            "bool AMoonfallHeroPawn::SaveWorldProgress()", 1)[0]
        begin = self.cpp.split("void AMoonfallHeroPawn::BeginPlay()", 1)[1].split(
            "void AMoonfallHeroPawn::Tick(", 1)[0]
        self.assertIn("FTransform InitialSpawnTransform = FTransform::Identity;", self.header)
        self.assertIn("bool bHasInitialSpawnTransform = false;", self.header)
        self.assertIn("bool RestoreSafeSpawnLocation();", self.header)
        self.assertIn("bHasInitialSpawnTransform = !SpawnLocation.ContainsNaN()", begin)
        self.assertIn("InitialSpawnTransform = GetActorTransform();", begin)
        self.assertLess(begin.index("InitialSpawnTransform = GetActorTransform();"),
                        begin.index("LoadWorldProgress();"))
        self.assertIn("if (RestoreMoonfallPlayerStart(this)) return true;", fallback)
        self.assertIn("if (!bHasInitialSpawnTransform) return false;", fallback)
        self.assertIn("Location.ContainsNaN()", fallback)
        self.assertIn("Location.SizeSquared() >= 1.0e14f", fallback)
        self.assertIn("TeleportTo(Location, InitialSpawnTransform.GetRotation().Rotator(), false, false)", fallback)
        self.assertIn("Hero->TeleportTo(Start->GetActorLocation()", helper)
        self.assertIn("Start->GetActorRotation(), false, false", helper)
        self.assertIn("SavedCheckpoint.ContainsNaN()", loader)
        self.assertIn("SavedCheckpoint.SizeSquared() >= 1.0e14f", loader)
        self.assertIn("if (!TeleportTo(SavedCheckpoint, GetActorRotation(), false, false))", loader)
        self.assertIn("if (!RestoreSafeSpawnLocation())", loader)
        self.assertIn("bPersistentSaveWritesBlocked = true;", loader)
        self.assertLess(loader.index("SavedCheckpoint.ContainsNaN()"),
                        loader.index("ResidentQuestStates.Reset();"))
        self.assertLess(loader.index("TeleportTo(SavedCheckpoint"),
                        loader.index("ResidentQuestStates.Reset();"))
        self.assertNotIn("SetActorLocation(Save->CheckpointLocation", loader)
        self.assertIn("if (!RestoreSafeSpawnLocation())", respawn)
        self.assertIn("GetCharacterMovement()->StopMovementImmediately();", respawn)
        self.assertIn("NextSafeCheckpointAt = Now + 5.f;", respawn)

    def test_new_shrines_are_automatic_and_use_stable_ids(self):
        shrine_h = (ROOT / "MoonfallCheckpointShrine.h").read_text(encoding="utf-8")
        shrine_cpp = (ROOT / "MoonfallCheckpointShrine.cpp").read_text(encoding="utf-8")
        arena = (ROOT / "MoonfallArena.cpp").read_text(encoding="utf-8")
        self.assertIn("class MOONFALLUNREAL_API AMoonfallCheckpointShrine", shrine_h)
        self.assertIn("FName ShrineId = NAME_None;", shrine_h)
        self.assertIn("OnComponentBeginOverlap.AddDynamic", shrine_cpp)
        self.assertIn("Hero->DiscoverShrine(this)", shrine_cpp)
        self.assertIn("PrimaryActorTick.bCanEverTick = false;", shrine_cpp)
        self.assertIn("SetCollisionEnabled(ECollisionEnabled::QueryOnly)", shrine_cpp)
        self.assertIn("SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap)", shrine_cpp)
        self.assertIn("GetActorLocation() + FVector(0.f, 0.f, 130.f)", shrine_h)
        self.assertIn("BeginDeferredActorSpawnFromClass", arena)
        for stable_id in ("crossroads_west", "crimson_path", "moonlit_path"):
            self.assertIn('TEXT("' + stable_id + '")', arena)
        self.assertIn("Shrine->ShrineId = FName(Placement.Id);", arena)

    def test_shrine_discovery_retries_failed_entry_without_tick(self):
        shrine_cpp = (ROOT / "MoonfallCheckpointShrine.cpp").read_text(encoding="utf-8")
        shrine_h = (ROOT / "MoonfallCheckpointShrine.h").read_text(encoding="utf-8")
        self.assertIn("PrimaryActorTick.bCanEverTick = false;", shrine_cpp)
        self.assertIn("OnComponentEndOverlap.AddDynamic", shrine_cpp)
        self.assertIn("GetWorldTimerManager().SetTimer(DiscoveryRetryTimer, this,", shrine_cpp)
        self.assertIn("GetWorldTimerManager().IsTimerActive(DiscoveryRetryTimer)", shrine_cpp)
        self.assertIn("DiscoveryRetryAttempts > 8", shrine_cpp)
        self.assertIn("void AMoonfallCheckpointShrine::RetryPendingDiscovery()", shrine_cpp)
        self.assertIn("void AMoonfallCheckpointShrine::OnDiscoveryEndOverlap(", shrine_cpp)
        self.assertIn("OtherActor == PendingDiscoveryHero.Get()", shrine_cpp)
        self.assertIn("GetWorldTimerManager().ClearTimer(DiscoveryRetryTimer)", shrine_cpp)
        self.assertIn("FTimerHandle DiscoveryRetryTimer;", shrine_h)
        self.assertNotIn("void AMoonfallCheckpointShrine::Tick(", shrine_cpp)

    def test_shrine_activation_has_clear_status_and_wall_guard(self):
        shrine_cpp = (ROOT / "MoonfallCheckpointShrine.cpp").read_text(encoding="utf-8")
        discover = self.cpp.split("bool AMoonfallHeroPawn::DiscoverShrine(", 1)[1].split(
            "bool AMoonfallHeroPawn::SaveWorldProgress()", 1)[0]
        startup = self.cpp.split("void AMoonfallHeroPawn::BeginPlay()", 1)[1].split(
            "void AMoonfallHeroPawn::Tick(", 1)[0]
        self.assertIn("bool bWorldProgressInitialized = false;", self.header)
        self.assertIn("bWorldProgressInitialized = true;", startup)
        self.assertLess(startup.index("LoadWorldProgress();"),
                        startup.index("bWorldProgressInitialized = true;"))
        self.assertIn("!bWorldProgressInitialized || bPersistentSaveWritesBlocked", discover)
        self.assertIn("ShrineSight.AddIgnoredActor(this)", discover)
        self.assertIn("ShrineSight.AddIgnoredActor(Shrine)", discover)
        self.assertIn("LineTraceTestByChannel(HeroLocation", discover)
        self.assertIn("ECC_Visibility, ShrineSight", discover)
        self.assertLess(discover.index("LineTraceTestByChannel(HeroLocation"),
                        discover.index("DiscoveredShrineIds.Add(Shrine->ShrineId);"))
        self.assertIn("It->RefreshShrineDisplay(this);", discover)
        self.assertIn('TEXT("ACTIVE RESPAWN SHRINE")', shrine_cpp)
        self.assertIn('TEXT("DISCOVERED SHRINE")', shrine_cpp)
        self.assertIn('TEXT("UNDISCOVERED SHRINE")', shrine_cpp)
        self.assertIn("Hero->ActiveShrineId == ShrineId", shrine_cpp)

    def test_last_discovered_shrine_controls_respawn_not_autosave(self):
        self.assertIn("int32 Version = 7;", self.save)
        self.assertIn("FName ActiveShrineId = NAME_None;", self.save)
        self.assertIn("TArray<FName> DiscoveredShrineIds;", self.save)
        self.assertIn("bool DiscoverShrine(AMoonfallCheckpointShrine* Shrine)", self.header)
        self.assertIn("FVector RespawnCheckpointLocation = FVector::ZeroVector;", self.header)
        discover = self.cpp.split("bool AMoonfallHeroPawn::DiscoverShrine(", 1)[1].split(
            "bool AMoonfallHeroPawn::SaveWorldProgress()", 1)[0]
        saver = self.cpp.split("bool AMoonfallHeroPawn::SaveWorldProgress()", 1)[1].split(
            "bool AMoonfallHeroPawn::LoadWorldProgress()", 1)[0]
        loader = self.cpp.split("bool AMoonfallHeroPawn::LoadWorldProgress()", 1)[1].split(
            "void AMoonfallHeroPawn::RespawnAtCheckpoint()", 1)[0]
        self.assertIn("DiscoveredShrineIds.Num() >= 64", discover)
        self.assertIn("HasDiscoveredShrine(Shrine->ShrineId)", discover)
        self.assertIn("FVector::DistSquared2D(ShrineLocation, HeroLocation)", discover)
        self.assertIn("ActiveShrineId = Shrine->ShrineId;", discover)
        self.assertIn("RespawnCheckpointLocation = NewRespawn;", discover)
        self.assertIn("if (!SaveWorldProgress())", discover)
        self.assertIn("DiscoveredShrineIds.Remove(Shrine->ShrineId);", discover)
        self.assertIn("ActiveShrineId = PreviousShrineId;", discover)
        self.assertIn("OnShrineDiscovered(ActiveShrineId);", discover)
        self.assertIn("Save->CheckpointLocation = RespawnCheckpointLocation;", saver)
        self.assertNotIn("Save->CheckpointLocation = GetActorLocation();", saver)
        self.assertIn("Save->ActiveShrineId = ActiveShrineId;", saver)
        self.assertIn("Save->DiscoveredShrineIds = DiscoveredShrineIds;", saver)
        self.assertIn("if (Save->Version >= 7)", loader)
        self.assertIn("Save->DiscoveredShrineIds.Num() > 64", loader)
        self.assertIn("UniqueShrines.Contains(Id)", loader)
        self.assertIn("RespawnCheckpointLocation = SavedCheckpoint;", loader)
        self.assertIn("ActiveShrineId = Save->Version >= 7 ?", loader)
        self.assertIn("DiscoveredShrineIds = Save->Version >= 7 ?", loader)
        # A loaded save must immediately update shrines already spawned in the world.
        # Respawns re-enter LoadWorldProgress, so previously active labels cannot stay stale.
        self.assertIn("TActorIterator<AMoonfallCheckpointShrine>", loader)
        self.assertIn("It->RefreshShrineDisplay(this);", loader)
        self.assertLess(loader.index("DiscoveredShrineIds = Save->Version >= 7 ?"),
                        loader.index("It->RefreshShrineDisplay(this);"))
        self.assertLess(loader.index("It->RefreshShrineDisplay(this);"),
                        loader.index("ResidentQuestStates.Reset();"))
        self.assertIn("if (SecondsSinceLastWorldSave >= 2.f", self.cpp)

    def test_not_a_new_run(self):
        self.assertIn('MoonfallPersistentWorldV1', self.cpp)
        self.assertIn("if (SecondsSinceLastWorldSave >= 2.f && GetWorld()", self.cpp)
        self.assertIn("if (DefeatElapsedSeconds >= 3.f) RespawnAtCheckpoint();", self.cpp)
        self.assertIn("if (Health <= 0.f || !GetWorld()", self.cpp)
        self.assertIn("GetCharacterMovement()->IsFalling()", self.cpp)
        self.assertIn("GetActorLocation().ContainsNaN()", self.cpp)
        self.assertIn("DamageImmunityUntil = FMath::Max(DamageImmunityUntil, Now + 3.f)", self.cpp)
        self.assertIn("NextSafeCheckpointAt = Now + 5.f;", self.cpp)

if __name__ == "__main__":
    unittest.main()

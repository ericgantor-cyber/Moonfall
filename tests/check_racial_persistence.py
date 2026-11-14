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

    def test_shrine_overlap_cannot_save_half_loaded_progress_on_respawn(self):
        self.assertIn("bool bWorldProgressLoadInProgress = false;", self.header)
        discover = self.cpp.split("bool AMoonfallHeroPawn::DiscoverShrine(", 1)[1].split(
            "bool AMoonfallHeroPawn::SaveWorldProgress()", 1)[0]
        loader = self.cpp.split("bool AMoonfallHeroPawn::LoadWorldProgress()", 1)[1].split(
            "void AMoonfallHeroPawn::RespawnAtCheckpoint()", 1)[0]
        self.assertIn("|| bWorldProgressLoadInProgress", discover)
        self.assertIn("bWorldProgressLoadInProgress = true;", loader)
        self.assertLess(loader.index("bWorldProgressLoadInProgress = true;"),
                        loader.index("if (!TeleportTo(SavedCheckpoint"))
        fallback = loader.split("if (!RestoreSafeSpawnLocation()) {", 1)[1].split(
            "UE_LOG(LogTemp, Warning, TEXT(\"Moonfall checkpoint blocked;", 1)[0]
        self.assertIn("bWorldProgressLoadInProgress = false;", fallback)
        self.assertLess(fallback.index("bWorldProgressLoadInProgress = false;"),
                        fallback.index("return false;"))
        restored = loader.split("InventoryItemCodes.Reset();", 1)[1]
        self.assertIn("bWorldProgressLoadInProgress = false;", restored)
        self.assertLess(restored.index("OnHealthChanged(Health, MaxHealth);"),
                        restored.index("bWorldProgressLoadInProgress = false;"))
        self.assertLess(restored.index("bWorldProgressLoadInProgress = false;"),
                        restored.index("return true;"))

    def test_common_and_traitless_loot_survives_save_reload_and_respawn(self):
        # Regression: enemy drops and pickup accept common and traitless items,
        # but the former load-only validator rejected all trait=0/rarity=0 items.
        valid = self.cpp.split("bool ValidMoonfallItemCode(int32 Code)", 1)[1].split(
            "bool RestoreMoonfallPlayerStart(", 1)[0]
        pickup = self.cpp.split("bool AMoonfallHeroPawn::CollectGroundEquipmentCode(", 1)[1].split(
            "void AMoonfallHeroPawn::SpawnGroundEquipmentDrop(", 1)[0]
        equip = self.cpp.split("bool AMoonfallHeroPawn::EquipInventoryItem(", 1)[1].split(
            "namespace {", 1)[0]
        rolls = self.cpp.split("int32 AMoonfallHeroPawn::RollEquipmentCode(", 1)[1].split(
            "void AMoonfallHeroPawn::AwardEquipmentDrop(", 1)[0]
        load = self.cpp.split("bool AMoonfallHeroPawn::LoadWorldProgress()", 1)[1].split(
            "void AMoonfallHeroPawn::RespawnAtCheckpoint()", 1)[0]
        self.assertIn("Trait >= 0 && Trait <= 3", valid)
        self.assertIn("Rarity >= 0 && Rarity <= 5", valid)
        self.assertIn("(Rarity != 0 || Trait == 0)", valid)
        self.assertIn("Tier >= 1 && Tier <= 25", valid)
        self.assertIn("Code == Trait * 100000 + Slot * 10000 + Rarity * 1000 + Tier", valid)
        self.assertIn("(Rarity == 0 && Affix != 0)", pickup)
        self.assertIn("(Rarity == 0 && Affix != 0)", equip)
        self.assertIn("LastLootAffix = LastLootRarity == 0 ? 0", rolls)
        self.assertIn("LastLootRarity == 1 && FMath::RandRange(0, 99) < 60", rolls)
        self.assertIn("if (ValidMoonfallItemCode(Code)) InventoryItemCodes.Add(Code);", load)
        self.assertIn("ValidMoonfallItemCode(Code) && ((Code / 10000) % 10) == Slot", load)

        def valid_item_code(code):
            if code <= 0:
                return False
            affix, slot, rarity, tier = code // 100000, code // 10000 % 10, code // 1000 % 10, code % 1000
            return (0 <= affix <= 3 and 0 <= slot <= 2 and 0 <= rarity <= 5
                    and 1 <= tier <= 25 and (rarity != 0 or affix == 0)
                    and code == affix * 100000 + slot * 10000 + rarity * 1000 + tier)

        # Representative codes from actual loot encoding, including the two
        # classes previously discarded on load, must all remain valid.
        for code in (1, 12, 1001, 11003, 2007, 21001, 122004, 325025):
            with self.subTest(item_code=code):
                self.assertTrue(valid_item_code(code))
        for code in (0, -1, 26, 100001, 35001, 6001, 1000000, 120026):
            with self.subTest(invalid_code=code):
                self.assertFalse(valid_item_code(code))

    def test_midair_progress_save_keeps_last_discovered_shrine(self):
        saver = self.cpp.split("bool AMoonfallHeroPawn::SaveWorldProgress()", 1)[1].split(
            "bool AMoonfallHeroPawn::LoadWorldProgress()", 1)[0]
        collect = self.cpp.split("bool AMoonfallHeroPawn::CollectGroundEquipmentCode(", 1)[1].split(
            "void AMoonfallHeroPawn::SpawnGroundEquipmentDrop(", 1)[0]
        self.assertNotIn("IsFalling()", saver)
        self.assertNotIn("Save->CheckpointLocation = GetActorLocation()", saver)
        self.assertIn("Save->CheckpointLocation = RespawnCheckpointLocation;", saver)
        self.assertIn("RespawnCheckpointLocation.ContainsNaN()", saver)
        self.assertIn("RespawnCheckpointLocation.SizeSquared() >= 1.0e14f", saver)
        self.assertIn("GetActorLocation().ContainsNaN()", saver)
        self.assertIn("GetActorLocation().SizeSquared() >= 1.0e14f", saver)
        self.assertLess(saver.index("RespawnCheckpointLocation.ContainsNaN()"),
                        saver.index("UGameplayStatics::SaveGameToSlot("))
        self.assertIn("if (!SaveWorldProgress()) {", collect)
        self.assertIn("InventoryItemCodes.RemoveAt(AddedIndex);", collect)

    def test_immediate_durable_writes_reset_the_two_second_autosave_clock(self):
        collect = self.cpp.split("bool AMoonfallHeroPawn::CollectGroundEquipmentCode(", 1)[1].split(
            "void AMoonfallHeroPawn::SpawnGroundEquipmentDrop(", 1)[0]
        discover = self.cpp.split("bool AMoonfallHeroPawn::DiscoverShrine(", 1)[1].split(
            "bool AMoonfallHeroPawn::SaveWorldProgress()", 1)[0]
        tick = self.cpp.split("void AMoonfallHeroPawn::Tick(", 1)[1].split(
            "void AMoonfallHeroPawn::SetupPlayerInputComponent(", 1)[0]
        for section in (collect, discover):
            self.assertEqual(section.count("SecondsSinceLastWorldSave = 0.f;"), 1)
            failure = section.split("if (!SaveWorldProgress()) {", 1)[1].split(
                "return false;", 1)[0]
            self.assertNotIn("SecondsSinceLastWorldSave = 0.f;", failure)
            self.assertLess(section.index("if (!SaveWorldProgress()) {"),
                            section.index("SecondsSinceLastWorldSave = 0.f;"))
            self.assertLess(section.index("return false;", section.index("if (!SaveWorldProgress()) {")),
                            section.index("SecondsSinceLastWorldSave = 0.f;"))
        self.assertIn("if (SaveWorldProgress()) SecondsSinceLastWorldSave = 0.f;", tick)
        self.assertIn("SecondsSinceLastWorldSave >= 2.f", tick)

    def test_ground_loot_pickup_is_transactional_and_durable_on_death(self):
        ground = (ROOT / "MoonfallGroundLoot.cpp").read_text(encoding="utf-8")
        collect = self.cpp.split("bool AMoonfallHeroPawn::CollectGroundEquipmentCode(", 1)[1].split(
            "void AMoonfallHeroPawn::SpawnGroundEquipmentDrop(", 1)[0]
        saver = self.cpp.split("bool AMoonfallHeroPawn::SaveWorldProgress()", 1)[1].split(
            "bool AMoonfallHeroPawn::LoadWorldProgress()", 1)[0]
        loader = self.cpp.split("bool AMoonfallHeroPawn::LoadWorldProgress()", 1)[1].split(
            "void AMoonfallHeroPawn::RespawnAtCheckpoint()", 1)[0]
        pickup = ground.split("bool AMoonfallGroundLoot::TryCollect()", 1)[1]
        self.assertIn("if (InventoryItemCodes.Num() >= 10000) return false;", collect)
        self.assertIn("const int32 AddedIndex = InventoryItemCodes.Add(ItemCode);", collect)
        self.assertIn("if (!SaveWorldProgress()) {", collect)
        self.assertIn("InventoryItemCodes.RemoveAt(AddedIndex);", collect)
        for field in ("Affix", "Slot", "Rarity", "Tier", "Upgrade"):
            self.assertIn("const int32 OldLoot" + field + " = LastLoot" + field + ";", collect)
            self.assertIn("LastLoot" + field + " = OldLoot" + field + ";", collect)
        self.assertLess(collect.index("InventoryItemCodes.Num() >= 10000"),
                        collect.index("InventoryItemCodes.Add(ItemCode);"))
        self.assertLess(collect.index("InventoryItemCodes.Add(ItemCode);"),
                        collect.index("if (!SaveWorldProgress()) {"))
        self.assertLess(collect.index("InventoryItemCodes.RemoveAt(AddedIndex);"),
                        collect.index("return false;", collect.index("if (!SaveWorldProgress()) {")))
        self.assertLess(collect.index("if (!SaveWorldProgress()) {"), collect.rindex("return true;"))
        self.assertIn("if (InventoryItemCodes.Num() > 10000 || ResidentQuestStates.Num() > 64", saver)
        self.assertIn("|| DiscoveredShrineIds.Num() > 64", saver)
        self.assertIn("Save->InventoryItemCodes = InventoryItemCodes;", saver)
        self.assertIn("Save->InventoryItemCodes.Num() > 10000", loader)
        self.assertIn("!Hero->CollectGroundEquipmentCode(ItemCode)", pickup)
        self.assertLess(pickup.index("Hero->CollectGroundEquipmentCode(ItemCode)"),
                        pickup.index("bCollected = true;"))
        self.assertLess(pickup.index("bCollected = true;"), pickup.index("Destroy();"))

    def test_all_world_saves_blocked_while_checkpoint_is_restoring(self):
        saver = self.cpp.split("bool AMoonfallHeroPawn::SaveWorldProgress()", 1)[1].split(
            "bool AMoonfallHeroPawn::LoadWorldProgress()", 1)[0]
        loader = self.cpp.split("bool AMoonfallHeroPawn::LoadWorldProgress()", 1)[1].split(
            "void AMoonfallHeroPawn::RespawnAtCheckpoint()", 1)[0]
        startup = self.cpp.split("void AMoonfallHeroPawn::BeginPlay()", 1)[1].split(
            "void AMoonfallHeroPawn::Tick(", 1)[0]
        self.assertIn("bool bWorldProgressInitialized = false;", self.header)
        self.assertIn("bool bWorldProgressLoadInProgress = false;", self.header)
        self.assertIn("if (!bWorldProgressInitialized || bWorldProgressLoadInProgress", saver)
        self.assertIn("|| bPersistentSaveWritesBlocked || !bHasRespawnCheckpointLocation", saver)
        self.assertLess(saver.index("if (!bWorldProgressInitialized || bWorldProgressLoadInProgress"),
                        saver.index("UGameplayStatics::CreateSaveGameObject("))
        self.assertLess(saver.index("if (!bWorldProgressInitialized || bWorldProgressLoadInProgress"),
                        saver.index("UGameplayStatics::SaveGameToSlot("))
        self.assertLess(startup.index("LoadWorldProgress();"), startup.index("bWorldProgressInitialized = true;"))
        self.assertLess(loader.index("bWorldProgressLoadInProgress = true;"),
                        loader.index("if (!TeleportTo(SavedCheckpoint"))
        self.assertIn("bWorldProgressLoadInProgress = false;", loader)
        self.assertLess(loader.index("OnHealthChanged(Health, MaxHealth);"),
                        loader.rindex("bWorldProgressLoadInProgress = false;"))

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
        self.assertIn("if (!bWorldProgressInitialized || bWorldProgressLoadInProgress", saver)
        self.assertIn("UGameplayStatics::SaveGameToSlot", saver)

    def test_future_version_save_is_not_overwritten_by_autosave(self):
        self.assertIn("bool bPersistentSaveWritesBlocked = false;", self.header)
        save = self.cpp.split("bool AMoonfallHeroPawn::SaveWorldProgress()", 1)[1].split(
            "bool AMoonfallHeroPawn::LoadWorldProgress()", 1)[0]
        load = self.cpp.split("bool AMoonfallHeroPawn::LoadWorldProgress()", 1)[1].split(
            "void AMoonfallHeroPawn::RespawnAtCheckpoint()", 1)[0]
        self.assertIn("if (!bWorldProgressInitialized || bWorldProgressLoadInProgress", save)
        self.assertIn("bPersistentSaveWritesBlocked || !bHasRespawnCheckpointLocation", save)
        self.assertLess(save.index("if (!bWorldProgressInitialized || bWorldProgressLoadInProgress"),
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

    def test_world_save_refuses_shrine_ledger_loader_would_reject(self):
        saver = self.cpp.split("bool AMoonfallHeroPawn::SaveWorldProgress()", 1)[1].split(
            "bool AMoonfallHeroPawn::LoadWorldProgress()", 1)[0]
        loader = self.cpp.split("bool AMoonfallHeroPawn::LoadWorldProgress()", 1)[1].split(
            "void AMoonfallHeroPawn::RespawnAtCheckpoint()", 1)[0]
        # A valid save must survive the same uniqueness and active-ID checks
        # performed during LoadWorldProgress. Never erase or sanitize a ledger.
        self.assertIn("DiscoveredShrineIds.Num() > 64", saver)
        self.assertIn("TSet<FName> UniqueShrines;", saver)
        self.assertIn("for (const FName Id : DiscoveredShrineIds)", saver)
        self.assertIn("Id.IsNone() || UniqueShrines.Contains(Id)", saver)
        self.assertIn("UniqueShrines.Add(Id);", saver)
        self.assertIn("!ActiveShrineId.IsNone() && !UniqueShrines.Contains(ActiveShrineId)", saver)
        self.assertIn("ActiveShrineId.IsNone() && !UniqueShrines.IsEmpty()", saver)
        self.assertIn("UniqueShrines.Contains(Id)", loader)
        self.assertIn("UniqueShrines.Add(Id);", loader)
        self.assertLess(saver.index("DiscoveredShrineIds.Num() > 64"),
                        saver.index("TSet<FName> UniqueShrines;"))
        self.assertLess(saver.index("TSet<FName> UniqueShrines;"),
                        saver.index("UGameplayStatics::CreateSaveGameObject"))
        self.assertLess(saver.index("ActiveShrineId.IsNone() && !UniqueShrines.IsEmpty()"),
                        saver.index("UGameplayStatics::SaveGameToSlot"))
        self.assertNotIn("DiscoveredShrineIds.Reset()", saver)
        self.assertNotIn("ActiveShrineId = NAME_None;", saver)

    def test_blocked_saved_checkpoint_remembers_validated_fallback(self):
        loader = self.cpp.split("bool AMoonfallHeroPawn::LoadWorldProgress()", 1)[1].split(
            "void AMoonfallHeroPawn::RespawnAtCheckpoint()", 1)[0]
        respawn = self.cpp.split("void AMoonfallHeroPawn::RespawnAtCheckpoint()", 1)[1].split(
            "// One unified roster.", 1)[0]
        saver = self.cpp.split("bool AMoonfallHeroPawn::SaveWorldProgress()", 1)[1].split(
            "bool AMoonfallHeroPawn::LoadWorldProgress()", 1)[0]
        self.assertIn("FVector ResolvedCheckpoint = SavedCheckpoint;", loader)
        self.assertIn("if (!TeleportTo(SavedCheckpoint, GetActorRotation(), false, false))", loader)
        self.assertIn("if (!RestoreSafeSpawnLocation()) {", loader)
        self.assertIn("ResolvedCheckpoint = GetActorLocation();", loader)
        self.assertIn("RespawnCheckpointLocation = ResolvedCheckpoint;", loader)
        self.assertNotIn("RespawnCheckpointLocation = SavedCheckpoint;", loader)
        self.assertLess(loader.index("if (!RestoreSafeSpawnLocation()) {"),
                        loader.index("ResolvedCheckpoint = GetActorLocation();"))
        self.assertLess(loader.index("ResolvedCheckpoint = GetActorLocation();"),
                        loader.index("RespawnCheckpointLocation = ResolvedCheckpoint;"))
        self.assertLess(loader.index("RespawnCheckpointLocation = ResolvedCheckpoint;"),
                        loader.index("ActiveShrineId = Save->Version >= 7 ?"))
        self.assertIn("bPersistentSaveWritesBlocked = true;", loader)
        self.assertIn("bWorldProgressLoadInProgress = false;", loader)
        self.assertIn("Save->CheckpointLocation = RespawnCheckpointLocation;", saver)
        self.assertIn("TeleportTo(RespawnCheckpointLocation, GetActorRotation(), false, false)", respawn)
        self.assertIn("ActiveShrineId = Save->Version >= 7 ?", loader)
        self.assertIn("DiscoveredShrineIds = Save->Version >= 7 ?", loader)

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
        self.assertIn("TeleportTo(RespawnCheckpointLocation, GetActorRotation(), false, false)", respawn)
        self.assertIn("bFoundSafeRespawn = RestoreSafeSpawnLocation();", respawn)
        self.assertLess(respawn.index("TeleportTo(RespawnCheckpointLocation"),
                        respawn.index("bFoundSafeRespawn = RestoreSafeSpawnLocation();"))
        self.assertIn("if (!bFoundSafeRespawn) {", respawn)
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

    def test_spawn_inside_shrine_works_after_initial_save_load(self):
        shrine_h = (ROOT / "MoonfallCheckpointShrine.h").read_text(encoding="utf-8")
        shrine_cpp = (ROOT / "MoonfallCheckpointShrine.cpp").read_text(encoding="utf-8")
        startup = self.cpp.split("void AMoonfallHeroPawn::BeginPlay()", 1)[1].split(
            "void AMoonfallHeroPawn::Tick(", 1)[0]
        shrine_begin = shrine_cpp.split("void AMoonfallCheckpointShrine::BeginPlay()", 1)[1].split(
            "void AMoonfallCheckpointShrine::OnDiscoveryOverlap(", 1)[0]
        proximity = shrine_cpp.split("void AMoonfallCheckpointShrine::CheckHeroAtSpawn(", 1)[1].split(
            "void AMoonfallCheckpointShrine::TryDiscoverOrQueueRetry(", 1)[0]
        self.assertIn("void CheckHeroAtSpawn(AMoonfallHeroPawn* Hero);", shrine_h)
        self.assertIn("for (TActorIterator<AMoonfallCheckpointShrine> It(GetWorld()); It; ++It)", startup)
        self.assertIn("It->CheckHeroAtSpawn(this);", startup)
        self.assertLess(startup.index("LoadWorldProgress();"),
                        startup.index("bWorldProgressInitialized = true;"))
        self.assertLess(startup.index("bWorldProgressInitialized = true;"),
                        startup.index("It->CheckHeroAtSpawn(this);"))
        self.assertIn("CheckHeroAtSpawn(Hero);", shrine_begin)
        self.assertIn("RefreshShrineDisplay(Hero);", proximity)
        self.assertIn("Hero->HasDiscoveredShrine(ShrineId)", proximity)
        self.assertIn("FVector::DistSquared(HeroLocation, ShrineLocation)", proximity)
        self.assertIn("HeroLocation.ContainsNaN()", proximity)
        self.assertIn("ShrineLocation.ContainsNaN()", proximity)
        self.assertIn("TryDiscoverOrQueueRetry(Hero);", proximity)
        self.assertNotIn("SetActorTickEnabled(true)", shrine_cpp)
        self.assertIn("PrimaryActorTick.bCanEverTick = false;", shrine_cpp)

    def test_discovered_shrines_are_not_reactivated(self):
        shrine_cpp = (ROOT / "MoonfallCheckpointShrine.cpp").read_text(encoding="utf-8")
        discover = self.cpp.split("bool AMoonfallHeroPawn::DiscoverShrine(", 1)[1].split(
            "bool AMoonfallHeroPawn::SaveWorldProgress()", 1)[0]
        check = shrine_cpp.split("void AMoonfallCheckpointShrine::CheckHeroAtSpawn(", 1)[1].split(
            "void AMoonfallCheckpointShrine::TryDiscoverOrQueueRetry(", 1)[0]
        attempt = shrine_cpp.split("void AMoonfallCheckpointShrine::TryDiscoverOrQueueRetry(", 1)[1].split(
            "void AMoonfallCheckpointShrine::RetryPendingDiscovery()", 1)[0]
        self.assertIn("bPersistentSaveWritesBlocked || HasDiscoveredShrine(Shrine->ShrineId)", discover)
        self.assertIn("if (Hero->HasDiscoveredShrine(ShrineId) ||", check)
        self.assertIn("if (Hero->HasDiscoveredShrine(ShrineId))", attempt)
        self.assertNotIn("bPreviouslyDiscovered", discover)

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

    def test_fallback_respawn_does_not_mislabel_old_shrine_as_active(self):
        shrine_cpp = (ROOT / "MoonfallCheckpointShrine.cpp").read_text(encoding="utf-8")
        hero_header = self.header
        helper = self.cpp.split(
            "bool AMoonfallHeroPawn::IsCurrentRespawnLocation(", 1)[1].split(
            "bool AMoonfallHeroPawn::DiscoverShrine(", 1)[0]
        loader = self.cpp.split("bool AMoonfallHeroPawn::LoadWorldProgress()", 1)[1].split(
            "void AMoonfallHeroPawn::RespawnAtCheckpoint()", 1)[0]
        # Restore the discovered/active ID for migration but never claim the
        # physical shrine is the respawn point after collision-safe fallback.
        self.assertIn("bool IsCurrentRespawnLocation(const FVector& Location) const;", hero_header)
        self.assertIn("bHasRespawnCheckpointLocation", helper)
        self.assertIn("!Location.ContainsNaN()", helper)
        self.assertIn("!RespawnCheckpointLocation.ContainsNaN()", helper)
        self.assertIn("FVector::DistSquared(RespawnCheckpointLocation, Location)", helper)
        self.assertIn("Hero->ActiveShrineId == ShrineId", shrine_cpp)
        self.assertIn("Hero->IsCurrentRespawnLocation(GetRespawnLocation())", shrine_cpp)
        label = shrine_cpp.split("void AMoonfallCheckpointShrine::RefreshShrineDisplay(", 1)[1].split(
            "void AMoonfallCheckpointShrine::CheckHeroAtSpawn(", 1)[0]
        self.assertLess(label.index("Hero->IsCurrentRespawnLocation(GetRespawnLocation())"),
                        label.index('TEXT("ACTIVE RESPAWN SHRINE")'))
        self.assertIn('TEXT("DISCOVERED SHRINE")', label)
        self.assertIn("RespawnCheckpointLocation = ResolvedCheckpoint;", loader)
        self.assertIn("ActiveShrineId = Save->Version >= 7 ?", loader)

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
        self.assertIn("!bWorldProgressInitialized || bWorldProgressLoadInProgress", discover)
        self.assertIn("|| bPersistentSaveWritesBlocked", discover)
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

    def test_new_shrine_requires_unblocked_capsule_sized_respawn(self):
        discover = self.cpp.split("bool AMoonfallHeroPawn::DiscoverShrine(", 1)[1].split(
            "bool AMoonfallHeroPawn::SaveWorldProgress()", 1)[0]
        self.assertIn('#include "Components/CapsuleComponent.h"', self.cpp)
        self.assertIn("const UCapsuleComponent* Capsule = GetCapsuleComponent();", discover)
        self.assertIn("if (!Capsule) return false;", discover)
        self.assertIn("Capsule->GetScaledCapsuleSize(CapsuleRadius, CapsuleHalfHeight);", discover)
        self.assertIn("FMath::IsFinite(CapsuleRadius)", discover)
        self.assertIn("FMath::IsFinite(CapsuleHalfHeight)", discover)
        self.assertIn("CapsuleHalfHeight < CapsuleRadius", discover)
        self.assertIn("SpawnClearance.AddIgnoredActor(this);", discover)
        self.assertIn("SpawnClearance.AddIgnoredActor(Shrine);", discover)
        self.assertIn("OverlapBlockingTestByChannel(NewRespawn, GetActorQuat()", discover)
        self.assertIn("ECC_Pawn, FCollisionShape::MakeCapsule(CapsuleRadius, CapsuleHalfHeight)", discover)
        self.assertIn("SpawnClearance)) return false;", discover)
        self.assertLess(discover.index("LineTraceTestByChannel(HeroLocation"),
                        discover.index("OverlapBlockingTestByChannel(NewRespawn"))
        self.assertLess(discover.index("OverlapBlockingTestByChannel(NewRespawn"),
                        discover.index("DiscoveredShrineIds.Add(Shrine->ShrineId);"))
        self.assertLess(discover.index("OverlapBlockingTestByChannel(NewRespawn"),
                        discover.index("ActiveShrineId = Shrine->ShrineId;"))
        self.assertLess(discover.index("OverlapBlockingTestByChannel(NewRespawn"),
                        discover.index("RespawnCheckpointLocation = NewRespawn;"))
        self.assertLess(discover.index("OverlapBlockingTestByChannel(NewRespawn"),
                        discover.index("if (!SaveWorldProgress()) {"))

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
        self.assertIn("RespawnCheckpointLocation = ResolvedCheckpoint;", loader)
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

    def test_memory_only_respawn_replaces_blocked_checkpoint_after_safe_fallback(self):
        # With no loadable disk slot, a blocked in-memory shrine must not be
        # saved again on the next two-second autosave after a safe rescue.
        respawn = self.cpp.split("void AMoonfallHeroPawn::RespawnAtCheckpoint()", 1)[1].split(
            "// One unified roster.", 1)[0]
        saver = self.cpp.split("bool AMoonfallHeroPawn::SaveWorldProgress()", 1)[1].split(
            "bool AMoonfallHeroPawn::LoadWorldProgress()", 1)[0]
        self.assertIn("if (!LoadWorldProgress()) {", respawn)
        self.assertIn("bFoundSafeRespawn = RestoreSafeSpawnLocation();", respawn)
        fallback = respawn.split("bFoundSafeRespawn = RestoreSafeSpawnLocation();", 1)[1].split(
            "if (!bFoundSafeRespawn) {", 1)[0]
        self.assertIn("if (bFoundSafeRespawn) {", fallback)
        self.assertIn("RespawnCheckpointLocation = GetActorLocation();", fallback)
        self.assertIn("bHasRespawnCheckpointLocation = true;", fallback)
        self.assertLess(fallback.index("if (bFoundSafeRespawn) {"),
                        fallback.index("RespawnCheckpointLocation = GetActorLocation();"))
        self.assertLess(fallback.index("RespawnCheckpointLocation = GetActorLocation();"),
                        fallback.index("bHasRespawnCheckpointLocation = true;"))
        self.assertLess(respawn.index("RespawnCheckpointLocation = GetActorLocation();"),
                        respawn.index("Health = FMath::Max(1.f, MaxHealth);"))
        self.assertIn("Save->CheckpointLocation = RespawnCheckpointLocation;", saver)
        self.assertNotIn("bPersistentSaveWritesBlocked = false;", respawn)

    def test_failed_checkpoint_requires_collision_safe_location_before_revive(self):
        respawn = self.cpp.split("void AMoonfallHeroPawn::RespawnAtCheckpoint()", 1)[1].split(
            "// One unified roster.", 1)[0]
        self.assertIn("if (!LoadWorldProgress()) {", respawn)
        self.assertIn("bHasRespawnCheckpointLocation", respawn)
        self.assertIn("!RespawnCheckpointLocation.ContainsNaN()", respawn)
        self.assertIn("RespawnCheckpointLocation.SizeSquared() < 1.0e14f", respawn)
        self.assertIn("TeleportTo(RespawnCheckpointLocation, GetActorRotation(), false, false)", respawn)
        self.assertIn("if (!bFoundSafeRespawn)", respawn)
        self.assertIn("bFoundSafeRespawn = RestoreSafeSpawnLocation();", respawn)
        failed = respawn.split("if (!bFoundSafeRespawn) {", 1)[1].split("Health = FMath::Max", 1)[0]
        self.assertIn("return;", failed)
        self.assertNotIn("Health =", failed)
        self.assertNotIn("DamageImmunityUntil =", failed)
        self.assertLess(respawn.index("return;"), respawn.index("Health = FMath::Max(1.f, MaxHealth);"))
        self.assertLess(respawn.index("Health = FMath::Max(1.f, MaxHealth);"),
                        respawn.index("OnHealthChanged(Health, MaxHealth);"))
        self.assertLess(respawn.index("OnHealthChanged(Health, MaxHealth);"),
                        respawn.index("DamageImmunityUntil = FMath::Max"))
        self.assertIn("DefeatElapsedSeconds = 0.f;", respawn)
        self.assertNotIn("SetActorLocation(", respawn)
        self.assertNotIn("bPersistentSaveWritesBlocked = false;", respawn)

    def test_respawn_rechecks_nearby_shrines_after_reviving(self):
        # A teleport can deliver the pawn into an undiscovered shrine while
        # Health is still zero. BeginOverlap rejects it, so recovery must
        # trigger exactly one additional proximity check after revival.
        respawn = self.cpp.split("void AMoonfallHeroPawn::RespawnAtCheckpoint()", 1)[1].split(
            "// One unified roster.", 1)[0]
        shrine_h = (ROOT / "MoonfallCheckpointShrine.h").read_text(encoding="utf-8")
        shrine_cpp = (ROOT / "MoonfallCheckpointShrine.cpp").read_text(encoding="utf-8")
        startup = self.cpp.split("void AMoonfallHeroPawn::BeginPlay()", 1)[1].split(
            "void AMoonfallHeroPawn::Tick(", 1)[0]
        self.assertIn("bool bFoundSafeRespawn =", respawn)
        self.assertIn("if (!bFoundSafeRespawn) {", respawn)
        self.assertIn("return;", respawn)
        self.assertIn("Health = FMath::Max(1.f, MaxHealth);", respawn)
        self.assertIn("DamageImmunityUntil = FMath::Max", respawn)
        self.assertIn("for (TActorIterator<AMoonfallCheckpointShrine> It(GetWorld()); It; ++It)", respawn)
        self.assertIn("if (IsValid(*It)) It->CheckHeroAtSpawn(this);", respawn)
        self.assertLess(respawn.index("if (!bFoundSafeRespawn) {"),
                        respawn.index("if (IsValid(*It)) It->CheckHeroAtSpawn(this);"))
        self.assertLess(respawn.index("Health = FMath::Max(1.f, MaxHealth);"),
                        respawn.index("It->CheckHeroAtSpawn(this);"))
        self.assertLess(respawn.index("DamageImmunityUntil = FMath::Max"),
                        respawn.index("It->CheckHeroAtSpawn(this);"))
        self.assertEqual(respawn.count("It->CheckHeroAtSpawn(this);"), 1)
        self.assertIn("It->CheckHeroAtSpawn(this);", startup)
        self.assertIn("void CheckHeroAtSpawn(AMoonfallHeroPawn* Hero);", shrine_h)
        self.assertIn("TryDiscoverOrQueueRetry(Hero);", shrine_cpp)
        self.assertIn("PrimaryActorTick.bCanEverTick = false;", shrine_cpp)

    def test_not_a_new_run(self):
        self.assertIn('MoonfallPersistentWorldV1', self.cpp)
        self.assertIn("if (SecondsSinceLastWorldSave >= 2.f && GetWorld()", self.cpp)
        self.assertIn("if (DefeatElapsedSeconds >= 3.f) RespawnAtCheckpoint();", self.cpp)
        self.assertIn("if (Health <= 0.f || !GetWorld()", self.cpp)
        saver = self.cpp.split("bool AMoonfallHeroPawn::SaveWorldProgress()", 1)[1].split(
            "bool AMoonfallHeroPawn::LoadWorldProgress()", 1)[0]
        # A saved shrine never moves with the hero; jumping should not stop
        # a two-second progress save or a transactional ground-loot pickup.
        self.assertNotIn("GetCharacterMovement()->IsFalling()", saver)
        self.assertIn("Save->CheckpointLocation = RespawnCheckpointLocation;", saver)
        self.assertIn("GetActorLocation().ContainsNaN()", saver)
        self.assertIn("GetActorLocation().ContainsNaN()", self.cpp)
        self.assertIn("DamageImmunityUntil = FMath::Max(DamageImmunityUntil, Now + 3.f)", self.cpp)
        self.assertIn("NextSafeCheckpointAt = Now + 5.f;", self.cpp)

if __name__ == "__main__":
    unittest.main()

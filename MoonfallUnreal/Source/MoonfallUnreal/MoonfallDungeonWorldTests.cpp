// These tests run inside a real UE5.4 Editor build, not GitHub source CI.
// Command: UnrealEditor-Cmd.exe MoonfallUnreal.uproject -unattended -NullRHI
//          -ExecCmds="Automation RunTests Moonfall.Dungeons; Quit" -log
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "MoonfallUnrealGameMode.h"
#include "MoonfallDungeonLayout.h"
#include "MoonfallHeroPawn.h"
#include "MoonfallPersistentSave.h"
#include "MoonfallEquipmentSlots.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UObjectGlobals.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoonfallDungeonPlacementTest,
  "Moonfall.Dungeons.All30EntranceLocations",
  EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMoonfallDungeonPlacementTest::RunTest(const FString& Parameters)
{
 const AMoonfallUnrealGameMode* Mode = GetDefault<AMoonfallUnrealGameMode>();
 if (!TestNotNull(TEXT("Dungeon game mode default"), Mode)) return false;
 TArray<FVector> Locations;
 for (int32 Zone = 0; Zone < 6; ++Zone) {
  for (int32 Dungeon = 0; Dungeon < 5; ++Dungeon) {
   FTransform Transform;
   if (!TestTrue(TEXT("Valid dungeon must have entrance transform"),
     Mode->GetDungeonEntranceTransform(Zone, Dungeon, Transform))) continue;
   const FVector Position = Transform.GetLocation();
   const int32 Slot = Zone * 5 + Dungeon;
   const float ExpectedX = (static_cast<float>(Slot) - 14.5f) * 1350.f;
   TestTrue(TEXT("Dungeon X follows continuous public road"),
     FMath::IsNearlyEqual(Position.X, ExpectedX, 0.1f));
   TestTrue(TEXT("Dungeon entry shares road Y and world height"),
     FMath::IsNearlyEqual(Position.Y, -3100.f, 0.1f)
       && FMath::IsNearlyZero(Position.Z, 0.1f));
   TestTrue(TEXT("Dungeon orientation faces south"),
     Transform.GetRotation().Equals(FRotator(0.f, -90.f, 0.f).Quaternion(), 0.0001f));
   for (const FVector& Other : Locations)
    TestTrue(TEXT("Adjacent dungeon lanes never overlap"),
      FMath::Abs(Other.X - Position.X) >= 1300.f);
   Locations.Add(Position);
  }
 }
 TestEqual(TEXT("All thirty dungeon positions are distinct"), Locations.Num(), 30);
 FTransform Invalid;
 TestFalse(TEXT("Zone -1 rejected"), Mode->GetDungeonEntranceTransform(-1, 0, Invalid));
 TestFalse(TEXT("Zone 6 rejected"), Mode->GetDungeonEntranceTransform(6, 0, Invalid));
 TestFalse(TEXT("Dungeon -1 rejected"), Mode->GetDungeonEntranceTransform(0, -1, Invalid));
 TestFalse(TEXT("Dungeon 5 rejected"), Mode->GetDungeonEntranceTransform(0, 5, Invalid));
 return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoonfallDungeonRoomSeedTest,
  "Moonfall.Dungeons.SeededRoomArchetypes",
  EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMoonfallDungeonRoomSeedTest::RunTest(const FString& Parameters)
{
 const AMoonfallDungeonLayout* Layout = GetDefault<AMoonfallDungeonLayout>();
 if (!TestNotNull(TEXT("Dungeon layout default"), Layout)) return false;
 TestEqual(TEXT("Entrance chamber"), Layout->GetChamberType(0), 0);
 TestEqual(TEXT("Final boss chamber"), Layout->GetChamberType(13), 6);
 TestEqual(TEXT("Negative chamber invalid"), Layout->GetChamberType(-1), INDEX_NONE);
 TestEqual(TEXT("Chamber fourteen invalid"), Layout->GetChamberType(14), INDEX_NONE);
 int32 Hazards = 0;
 for (int32 Chamber = 1; Chamber < 13; ++Chamber) {
  const int32 Kind = Layout->GetChamberType(Chamber);
  TestTrue(TEXT("All interior archetypes in valid range"), Kind >= 1 && Kind <= 5);
  TestEqual(TEXT("A fixed layout seed returns stable chamber type"),
    Kind, Layout->GetChamberType(Chamber));
  if (Kind == 5) ++Hazards;
 }
 TestEqual(TEXT("Exactly three unique hazardous chambers"), Hazards, 3);
 return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoonfallDungeonBossVictoryLedgerTest,
  "Moonfall.Dungeons.BossVictoryLedgerDefaults",
  EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMoonfallDungeonBossVictoryLedgerTest::RunTest(const FString& Parameters)
{
 // Native test of the persistent V10 schema and all 30 victory-index
 // boundaries. Combat, disk save round-trips and spawn AI need separate
 // in-world smoke verification.
 const UMoonfallPersistentSave* Save = GetDefault<UMoonfallPersistentSave>();
 const AMoonfallHeroPawn* Hero = GetDefault<AMoonfallHeroPawn>();
 if (!TestNotNull(TEXT("Persistent save default"), Save)
     || !TestNotNull(TEXT("Hero default"), Hero)) return false;
 TestEqual(TEXT("Latest default save remains V12"), Save->Version, 12);
 TestEqual(TEXT("Fresh save has no boss victories"),
   Save->DefeatedDungeonBossMask, 0);
 TestEqual(TEXT("Fresh hero has no boss victories"),
   Hero->DefeatedDungeonBossMask, 0);
 for (int32 Zone = 0; Zone < 6; ++Zone) {
  TestEqual(TEXT("Fresh zone victory count is zero"),
    Hero->GetDefeatedDungeonBossCount(Zone), 0);
  for (int32 Dungeon = 0; Dungeon < 5; ++Dungeon)
   TestFalse(TEXT("Unbeaten dungeon has no saved boss victory"),
     Hero->HasDefeatedDungeonBoss(Zone, Dungeon));
 }
 TestEqual(TEXT("Fresh campaign victory count"),
   Hero->GetTotalDefeatedDungeonBosses(), 0);
 TestEqual(TEXT("Fresh cleared-zone count"),
   Hero->GetClearedDungeonZoneCount(), 0);
 TestFalse(TEXT("Negative zone rejected"), Hero->HasDefeatedDungeonBoss(-1, 0));
 TestFalse(TEXT("Zone beyond six rejected"), Hero->HasDefeatedDungeonBoss(6, 0));
 TestFalse(TEXT("Negative dungeon rejected"), Hero->HasDefeatedDungeonBoss(0, -1));
 TestFalse(TEXT("Dungeon beyond five rejected"), Hero->HasDefeatedDungeonBoss(0, 5));
 TestEqual(TEXT("Invalid zone cannot yield victories"),
   Hero->GetDefeatedDungeonBossCount(6), 0);
 return !HasAnyErrors();
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoonfallDungeonBossSaveRoundTripTest,
  "Moonfall.Dungeons.BossVictorySaveRoundTrip",
  EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMoonfallDungeonBossSaveRoundTripTest::RunTest(const FString& Parameters)
{
 // Serialize to memory only. Never touch the player's on-disk world slot.
 UMoonfallPersistentSave* Original = Cast<UMoonfallPersistentSave>(
   UGameplayStatics::CreateSaveGameObject(UMoonfallPersistentSave::StaticClass()));
 if (!TestNotNull(TEXT("Create V12 save object"), Original)) return false;
 constexpr uint32 FirstDungeon = 1u;
 constexpr uint32 LastDungeon = 1u << 29;
 constexpr uint32 MidDungeon = 1u << 14;
 Original->DiscoveredDungeonMask = static_cast<int32>(
   FirstDungeon | MidDungeon | LastDungeon);
 Original->DefeatedDungeonBossMask = static_cast<int32>(
   FirstDungeon | LastDungeon);
 Original->Salvage = 4321;
 Original->Level = 17;
 TArray<uint8> Bytes;
 if (!TestTrue(TEXT("Serialize boss ledger in memory"),
       UGameplayStatics::SaveGameToMemory(Original, Bytes))) return false;
 if (!TestTrue(TEXT("Serialized bytes are present"), Bytes.Num() > 0))
  return false;
 UMoonfallPersistentSave* Restored = Cast<UMoonfallPersistentSave>(
   UGameplayStatics::LoadGameFromMemory(Bytes));
 if (!TestNotNull(TEXT("Restore V12 save from bytes"), Restored)) return false;
 TestEqual(TEXT("Save format still V12"), Restored->Version, 12);
 TestEqual(TEXT("Discovered first, middle and final dungeons survive"),
   Restored->DiscoveredDungeonMask, Original->DiscoveredDungeonMask);
 TestEqual(TEXT("First and 30th dungeon victories survive"),
   Restored->DefeatedDungeonBossMask, Original->DefeatedDungeonBossMask);
 TestTrue(TEXT("Final victory uses bit 29"),
   (static_cast<uint32>(Restored->DefeatedDungeonBossMask) & LastDungeon) != 0u);
 TestFalse(TEXT("Undefeated middle dungeon stays unbeaten"),
   (static_cast<uint32>(Restored->DefeatedDungeonBossMask) & MidDungeon) != 0u);
 TestEqual(TEXT("Reward balance survives"), Restored->Salvage, 4321);
 TestEqual(TEXT("Character level survives"), Restored->Level, 17);

 // Check every dungeon ledger position separately. One-hot round trips
 // detect dropped bits and accidental cross-zone victories, not just endpoints.
 for (int32 Slot = 0; Slot < 30; ++Slot) {
  const uint32 VictoryBit = 1u << Slot;
  Original->DiscoveredDungeonMask = static_cast<int32>(VictoryBit);
  Original->DefeatedDungeonBossMask = static_cast<int32>(VictoryBit);
  Bytes.Reset();
  const FString Label = FString::Printf(TEXT("Dungeon boss slot %d"), Slot);
  if (!TestTrue(*(Label + TEXT(" saves to memory")),
        UGameplayStatics::SaveGameToMemory(Original, Bytes))) return false;
  UMoonfallPersistentSave* OneVictory = Cast<UMoonfallPersistentSave>(
    UGameplayStatics::LoadGameFromMemory(Bytes));
  if (!TestNotNull(*FString::Printf(TEXT("%s restores"), *Label), OneVictory))
   return false;
  TestEqual(*(Label + TEXT(" discovery remains isolated")),
    OneVictory->DiscoveredDungeonMask, static_cast<int32>(VictoryBit));
  TestEqual(*(Label + TEXT(" victory remains isolated")),
    OneVictory->DefeatedDungeonBossMask, static_cast<int32>(VictoryBit));
 }

 // Sparse, alternating and fully cleared campaigns catch errors which
 // one-hot tests cannot detect when several bits are set simultaneously.
 const uint32 AllDungeons = (1u << 30) - 1u;
 const uint32 Scenarios[] = {0u, 0x15555555u, 0x2AAAAAAAu, AllDungeons};
 for (const uint32 VictoryMask : Scenarios) {
  Original->DiscoveredDungeonMask = static_cast<int32>(AllDungeons);
  Original->DefeatedDungeonBossMask = static_cast<int32>(VictoryMask);
  Bytes.Reset();
  const FString Label = FString::Printf(TEXT("Victory pattern 0x%08x"), VictoryMask);
  if (!TestTrue(*(Label + TEXT(" serializes")),
        UGameplayStatics::SaveGameToMemory(Original, Bytes))) return false;
  UMoonfallPersistentSave* Pattern = Cast<UMoonfallPersistentSave>(
    UGameplayStatics::LoadGameFromMemory(Bytes));
  if (!TestNotNull(*(Label + TEXT(" restores")), Pattern)) return false;
  TestEqual(*(Label + TEXT(" keeps all discovered entrances")),
    Pattern->DiscoveredDungeonMask, static_cast<int32>(AllDungeons));
  TestEqual(*(Label + TEXT(" keeps exact victories")),
    Pattern->DefeatedDungeonBossMask, static_cast<int32>(VictoryMask));
 }

 // Emulate serialized V9 data and check it retains existing progress with
 // zero new boss victories. This does not substitute for loading a genuine
 // historical on-disk V9 fixture through LoadWorldProgress.
 Original->Version = 9;
 Original->DiscoveredDungeonMask = static_cast<int32>(FirstDungeon | MidDungeon);
 Original->DefeatedDungeonBossMask = 0;
 Original->Salvage = 9876;
 Original->Level = 23;
 Bytes.Reset();
 if (!TestTrue(TEXT("V9-compatible save serializes"),
       UGameplayStatics::SaveGameToMemory(Original, Bytes))) return false;
 UMoonfallPersistentSave* Legacy = Cast<UMoonfallPersistentSave>(
   UGameplayStatics::LoadGameFromMemory(Bytes));
 if (!TestNotNull(TEXT("V9-compatible save restores"), Legacy)) return false;
 TestEqual(TEXT("Legacy version remains V9 before game migration"), Legacy->Version, 9);
 TestEqual(TEXT("Legacy discovered entrances retained"),
   Legacy->DiscoveredDungeonMask, static_cast<int32>(FirstDungeon | MidDungeon));
 TestEqual(TEXT("Legacy has no invented boss victories"),
   Legacy->DefeatedDungeonBossMask, 0);
 TestEqual(TEXT("Legacy currency retained"), Legacy->Salvage, 9876);
 TestEqual(TEXT("Legacy level retained"), Legacy->Level, 23);
 return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoonfallV13EquipmentSaveRoundTripTest,
  "Moonfall.Dungeons.EquipmentV13SaveRoundTrip",
  EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMoonfallV13EquipmentSaveRoundTripTest::RunTest(const FString& Parameters)
{
 // Native UE5 SaveGame property serialization. Nothing is written to disk.
 // A historical V12 file fixture and real HeroPawn::LoadWorldProgress still
 // require a separate controlled Windows gameplay migration test.
 UMoonfallPersistentSave* Legacy = Cast<UMoonfallPersistentSave>(
   UGameplayStatics::CreateSaveGameObject(UMoonfallPersistentSave::StaticClass()));
 if (!TestNotNull(TEXT("Create legacy V12 save"), Legacy)) return false;
 TestEqual(TEXT("Default save starts at V12"),
   Legacy->Version, MoonfallEquipmentSlots::LegacySaveVersion);
 TestTrue(TEXT("V12 has no nine-slot equipment data"), Legacy->ExpandedEquippedCodes.IsEmpty());
 Legacy->EquippedWeaponCode = 1001;
 Legacy->EquippedArmorCode = 11002;
 Legacy->EquippedAccessoryCode = 22003;
 Legacy->InventoryItemCodes.Add(1001);
 Legacy->InventoryItemCodes.Add(11002);
 Legacy->InventoryItemCodes.Add(22003);
 Legacy->Level = 27;
 Legacy->Salvage = 8765;
 TArray<uint8> Bytes;
 if (!TestTrue(TEXT("Serialize V12 equipment in memory"),
      UGameplayStatics::SaveGameToMemory(Legacy, Bytes))) return false;
 UMoonfallPersistentSave* RestoredV12 = Cast<UMoonfallPersistentSave>(
   UGameplayStatics::LoadGameFromMemory(Bytes));
 if (!TestNotNull(TEXT("Restore V12 equipment in memory"), RestoredV12)) return false;
 TestEqual(TEXT("V12 version survives"), RestoredV12->Version, 12);
 TestEqual(TEXT("V12 weapon survives"), RestoredV12->EquippedWeaponCode, 1001);
 TestEqual(TEXT("V12 armor survives"), RestoredV12->EquippedArmorCode, 11002);
 TestEqual(TEXT("V12 accessory survives"), RestoredV12->EquippedAccessoryCode, 22003);
 TestTrue(TEXT("V12 has no invented extra equipped items"),
   RestoredV12->ExpandedEquippedCodes.IsEmpty());
 TestEqual(TEXT("V12 inventory remains intact"), RestoredV12->InventoryItemCodes.Num(), 3);
 TestEqual(TEXT("V12 character level survives"), RestoredV12->Level, 27);
 TestEqual(TEXT("V12 salvage survives"), RestoredV12->Salvage, 8765);

 // V13 keeps exact legacy fields and a complete nine-item snapshot.
 UMoonfallPersistentSave* Expanded = Cast<UMoonfallPersistentSave>(
   UGameplayStatics::CreateSaveGameObject(UMoonfallPersistentSave::StaticClass()));
 if (!TestNotNull(TEXT("Create V13 equipment save"), Expanded)) return false;
 Expanded->Version = MoonfallEquipmentSlots::ExpandedSaveVersion;
 const int32 Codes[MoonfallEquipmentSlots::SlotCount] = {
   1001, 11002, 22003, 31004, 42005, 53006, 64007, 75008, 81009
 };
 for (const int32 Code : Codes) Expanded->InventoryItemCodes.Add(Code);
 Expanded->ExpandedEquippedCodes.Init(0, MoonfallEquipmentSlots::SlotCount);
 for (int32 Slot = 0; Slot < MoonfallEquipmentSlots::SlotCount; ++Slot)
  Expanded->ExpandedEquippedCodes[Slot] = Codes[Slot];
 Expanded->EquippedWeaponCode = Codes[0];
 Expanded->EquippedArmorCode = Codes[1];
 Expanded->EquippedAccessoryCode = Codes[2];
 Expanded->Level = 38;
 Expanded->Salvage = 4321;
 const MoonfallEquipmentSlots::LegacyCodes LegacyCodes{{
   Codes[0], Codes[1], Codes[2]
 }};
 MoonfallEquipmentSlots::ExpandedCodes Snapshot{};
 for (int32 Slot = 0; Slot < MoonfallEquipmentSlots::SlotCount; ++Slot)
  Snapshot[Slot] = Expanded->ExpandedEquippedCodes[Slot];
 if (!TestTrue(TEXT("V13 preflight accepts nine owned canonical codes"),
     MoonfallEquipmentSlots::ValidateV13(Snapshot, LegacyCodes,
       Expanded->InventoryItemCodes.GetData(), Expanded->InventoryItemCodes.Num())))
  return false;
 Bytes.Reset();
 if (!TestTrue(TEXT("Serialize V13 equipment in memory"),
      UGameplayStatics::SaveGameToMemory(Expanded, Bytes))) return false;
 UMoonfallPersistentSave* RestoredV13 = Cast<UMoonfallPersistentSave>(
   UGameplayStatics::LoadGameFromMemory(Bytes));
 if (!TestNotNull(TEXT("Restore V13 equipment in memory"), RestoredV13)) return false;
 TestEqual(TEXT("V13 version survives"),
   RestoredV13->Version, MoonfallEquipmentSlots::ExpandedSaveVersion);
 TestEqual(TEXT("V13 legacy weapon survives"), RestoredV13->EquippedWeaponCode, Codes[0]);
 TestEqual(TEXT("V13 legacy armor survives"), RestoredV13->EquippedArmorCode, Codes[1]);
 TestEqual(TEXT("V13 legacy accessory survives"), RestoredV13->EquippedAccessoryCode, Codes[2]);
 TestEqual(TEXT("V13 snapshot contains exactly nine slots"),
   RestoredV13->ExpandedEquippedCodes.Num(), MoonfallEquipmentSlots::SlotCount);
 if (RestoredV13->ExpandedEquippedCodes.Num() != MoonfallEquipmentSlots::SlotCount)
  return false;
 for (int32 Slot = 0; Slot < MoonfallEquipmentSlots::SlotCount; ++Slot) {
  const FString Label = FString::Printf(TEXT("V13 equipped slot %d round-trips"), Slot);
  TestEqual(*Label, RestoredV13->ExpandedEquippedCodes[Slot], Codes[Slot]);
 }
 TestEqual(TEXT("V13 inventory remains intact"),
   RestoredV13->InventoryItemCodes.Num(), MoonfallEquipmentSlots::SlotCount);
 TestEqual(TEXT("V13 character level survives"), RestoredV13->Level, 38);
 TestEqual(TEXT("V13 salvage survives"), RestoredV13->Salvage, 4321);
 // V12 downgrade must never discard worn extra-slot equipment.
 MoonfallEquipmentSlots::LegacyCodes Downgrade{};
 TestFalse(TEXT("V13 cannot silently downgrade with six worn extras"),
   MoonfallEquipmentSlots::TryDowngradeToV12(Snapshot, Downgrade));
 return !HasAnyErrors();
}

#endif

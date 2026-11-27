// These tests run inside a real UE5.4 Editor build, not GitHub source CI.
// Command: UnrealEditor-Cmd.exe MoonfallUnreal.uproject -unattended -NullRHI
//          -ExecCmds="Automation RunTests Moonfall.Dungeons; Quit" -log
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "HAL/PlatformMisc.h"
#include "MoonfallUnrealGameMode.h"
#include "MoonfallDungeonLayout.h"
#include "MoonfallHeroPawn.h"
#include "MoonfallPersistentSave.h"
#include "MoonfallGearAdvisorPolicy.h"
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
 // Native test of the current persistent schema and all 30 victory-index
 // boundaries. Combat, disk save round-trips and spawn AI need separate
 // in-world smoke verification.
 const UMoonfallPersistentSave* Save = GetDefault<UMoonfallPersistentSave>();
 const AMoonfallHeroPawn* Hero = GetDefault<AMoonfallHeroPawn>();
 if (!TestNotNull(TEXT("Persistent save default"), Save)
     || !TestNotNull(TEXT("Hero default"), Hero)) return false;
 TestEqual(TEXT("Current save schema is V17"), Save->Version, 17);
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
 if (!TestNotNull(TEXT("Create current save object"), Original)) return false;
 constexpr uint32 FirstDungeon = 1u;
 constexpr uint32 LastDungeon = 1u << 29;
 constexpr uint32 MidDungeon = 1u << 14;
 Original->DiscoveredDungeonMask = static_cast<int32>(
   FirstDungeon | MidDungeon | LastDungeon);
 Original->DefeatedDungeonBossMask = static_cast<int32>(
   FirstDungeon | LastDungeon);
 Original->EquippedWeaponCode = 101005;
 Original->EquippedArmorCode = 111005;
 Original->EquippedAccessoryCode = 121005;
 Original->InventoryItemCodes = {101005,111005,121005};
 Original->EquippedSlotCodes = {101005,111005,121005,0,0,0,0,0,0};
 Original->Salvage = 4321;
 Original->Level = 17;
 TArray<uint8> Bytes;
 if (!TestTrue(TEXT("Serialize boss ledger in memory"),
       UGameplayStatics::SaveGameToMemory(Original, Bytes))) return false;
 if (!TestTrue(TEXT("Serialized bytes are present"), Bytes.Num() > 0))
  return false;
 UMoonfallPersistentSave* Restored = Cast<UMoonfallPersistentSave>(
   UGameplayStatics::LoadGameFromMemory(Bytes));
 if (!TestNotNull(TEXT("Restore current save from bytes"), Restored)) return false;
 TestEqual(TEXT("Current save format survives"), Restored->Version, 17);
 TestTrue(TEXT("Nine equipment slot IDs survive memory serialization"), Restored->EquippedSlotCodes == Original->EquippedSlotCodes);
 TestEqual(TEXT("Legacy weapon mirror survives"), Restored->EquippedWeaponCode, Original->EquippedWeaponCode);
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


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoonfallEquipmentSaveRoundTripTest,
  "Moonfall.Dungeons.EquipmentSaveRoundTrip",
  EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMoonfallEquipmentSaveRoundTripTest::RunTest(const FString& Parameters)
{
 // Verify the requested mode matches the actual compiled editor module.
 // This fails closed if an incremental build reused the wrong configuration.
 const FString RequestedPreview=FPlatformMisc::GetEnvironmentVariable(TEXT("MOONFALL_V19_EDITOR_PREVIEW"));
 if(!TestEqual(TEXT("Compiled equipment preview matches requested mode"),
     MoonfallEquipmentLayout::bV19EditorPreview,RequestedPreview==TEXT("1")))return false;
 if(!TestEqual(TEXT("Compiled active equipment slot count"),
     MoonfallEquipmentLayout::ActiveSlotCount,RequestedPreview==TEXT("1")?24:9))return false;
 // Exercise the serialized layouts in memory, not live equipping or disk saves.
 UMoonfallPersistentSave* Original = Cast<UMoonfallPersistentSave>(
   UGameplayStatics::CreateSaveGameObject(UMoonfallPersistentSave::StaticClass()));
 if (!TestNotNull(TEXT("Create equipment save"), Original)) return false;
 TestEqual(TEXT("Fresh equipment save has nine slots"), Original->EquippedSlotCodes.Num(), 9);
 for (const int32 Code : Original->EquippedSlotCodes)
  TestEqual(TEXT("Fresh equipment slot is empty"), Code, 0);
 const TArray<int32> AllCodes = {101005,111006,121007,131008,141009,151010,161011,171012,181013};
 auto RoundTrip = [this, Original](const TArray<int32>& Codes, int32 Version, bool bValidateMigration=false) -> bool {
  Original->Version = Version;
  Original->EquippedSlotCodes = Codes;
  TArray<uint8> Bytes;
  if (!TestTrue(TEXT("Serialize equipment record to memory"),
        UGameplayStatics::SaveGameToMemory(Original, Bytes))) return false;
  UMoonfallPersistentSave* Restored = Cast<UMoonfallPersistentSave>(
    UGameplayStatics::LoadGameFromMemory(Bytes));
  if (!TestNotNull(TEXT("Deserialize equipment record from memory"), Restored)) return false;
  TestEqual(TEXT("Schema version preserved"), Restored->Version, Version);
  TestTrue(TEXT("Exact slot record preserved without normalization"),
    Restored->EquippedSlotCodes == Codes);
  TestTrue(TEXT("Inventory ownership record preserved"),
    Restored->InventoryItemCodes == Original->InventoryItemCodes);
  TestEqual(TEXT("Weapon mirror preserved"), Restored->EquippedWeaponCode, Original->EquippedWeaponCode);
  TestEqual(TEXT("Chest mirror preserved"), Restored->EquippedArmorCode, Original->EquippedArmorCode);
  TestEqual(TEXT("Trinket mirror preserved"), Restored->EquippedAccessoryCode, Original->EquippedAccessoryCode);
  if(bValidateMigration){
   const int32 Legacy[]={Restored->EquippedWeaponCode,Restored->EquippedArmorCode,Restored->EquippedAccessoryCode};
   MoonfallEquipmentLayout::FFullLoadout Migrated{};
   if(!TestTrue(TEXT("Deserialized equipment passes migration validation"),
       MoonfallEquipmentLayout::TryRestoreFlexibleFullLoadout(Legacy,3,
       Restored->EquippedSlotCodes.GetData(),Restored->EquippedSlotCodes.Num(),
       Restored->Version,Restored->InventoryItemCodes,Migrated)))return false;
   for(int32 Slot=0;Slot<MoonfallEquipmentLayout::FullSlotCount;++Slot){
    const int32 Expected=Version<17?(Slot<3?Legacy[Slot]:0):
        (Codes.IsValidIndex(Slot)?Codes[Slot]:0);
    TestEqual(TEXT("Deserialized migration preserves exact slot or zero"),Migrated[Slot],Expected);
   }
  }
  return true;
 };
 Original->InventoryItemCodes = AllCodes;
 Original->EquippedWeaponCode = AllCodes[0];
 Original->EquippedArmorCode = AllCodes[1];
 Original->EquippedAccessoryCode = AllCodes[2];
 if (!RoundTrip(AllCodes, 17,true)) return false;
 // One-hot records catch a swapped slot or an omitted serialized element.
 for (int32 Slot = 0; Slot < AllCodes.Num(); ++Slot) {
  TArray<int32> Single;
  Single.Init(0, 9);
  Single[Slot] = AllCodes[Slot];
  Original->EquippedWeaponCode = Single[0];
  Original->EquippedArmorCode = Single[1];
  Original->EquippedAccessoryCode = Single[2];
  if (!RoundTrip(Single, 17,true)) return false;
 }
 // Malformed records must remain detectable by the separate gameplay loader.
 // Serialization itself must not silently pad, truncate or repair these.
 TArray<int32> Short = AllCodes;
 Short.Pop();
 if (!RoundTrip(Short, 17)) return false;
 TArray<int32> Long = AllCodes;
 Long.Add(0);
 if (!RoundTrip(Long, 17)) return false;
 // Synthetic V16 fixture: verifies serialization of the legacy mirrors only,
 // not loading a real historical save or upgrading a live character.
 Original->InventoryItemCodes = {AllCodes[0],AllCodes[1],AllCodes[2]};
 Original->EquippedWeaponCode = AllCodes[0];
 Original->EquippedArmorCode = AllCodes[1];
 Original->EquippedAccessoryCode = AllCodes[2];
 if (!RoundTrip(TArray<int32>{}, 16,true)) return false;
 // Serialize the intermediate V18 layout before exercising future V19.
 TArray<int32> Middle;
 for(int32 Slot=0;Slot<MoonfallEquipmentLayout::SixteenSlotCount;++Slot)
  Middle.Add(MoonfallEquipmentLayout::EncodeFullItem(Slot,1,2,5));
 Original->InventoryItemCodes=Middle;
 Original->EquippedWeaponCode=Middle[0];
 Original->EquippedArmorCode=Middle[1];
 Original->EquippedAccessoryCode=Middle[2];
 if(!RoundTrip(Middle,18,true))return false;
 // Synthetic future V19 fixture: prove 24 entries and repeated ring codes
 // survive native memory serialization. The live V17 loader stays gated.
 TArray<int32> Future;
 Future.Init(0,MoonfallEquipmentLayout::FullSlotCount);
 for(int32 Slot=0;Slot<Future.Num();++Slot)
  Future[Slot]=MoonfallEquipmentLayout::EncodeFullItem(Slot,1,2,5);
 Future[23]=Future[8];
 Original->InventoryItemCodes=Future;
 Original->EquippedWeaponCode=Future[0];
 Original->EquippedArmorCode=Future[1];
 Original->EquippedAccessoryCode=Future[2];
 if (!RoundTrip(Future,19,true)) return false;
 const int32 Mirrors[] = {Future[0],Future[1],Future[2]};
 MoonfallEquipmentLayout::FFullLoadout Migrated{};
 TestTrue(TEXT("V19 accepts all 24 owned slots including duplicate rings"),
  MoonfallEquipmentLayout::TryRestoreFlexibleFullLoadout(Mirrors,3,
   Future.GetData(),Future.Num(),19,Original->InventoryItemCodes,Migrated));
 for(int32 Slot=0;Slot<Future.Num();++Slot)
  TestEqual(TEXT("V19 migration retains exact equipment position"),Migrated[Slot],Future[Slot]);
 TArray<int32> MissingCopy=Original->InventoryItemCodes;
 MissingCopy.RemoveAt(23);
 // A strict V18 checkpoint has sixteen slots and no cross-finger aliases.
 MoonfallEquipmentLayout::FFullLoadout MiddleRestored{};
 TestTrue(TEXT("V18 migration retains sixteen strict equipment positions"),
  MoonfallEquipmentLayout::TryRestoreFlexibleFullLoadout(Mirrors,3,
   Middle.GetData(),Middle.Num(),18,Middle,MiddleRestored));
 for(int32 Slot=0;Slot<MoonfallEquipmentLayout::FullSlotCount;++Slot)
  TestEqual(TEXT("V18 migration zero-fills new positions"),MiddleRestored[Slot],
   Middle.IsValidIndex(Slot)?Middle[Slot]:0);
 const auto BeforeRejected=Migrated;
 const int32 AlternateSlotZero=MoonfallEquipmentLayout::EncodeFullItem(0,2,2,5);
 TArray<int32> V19Inventory=Original->InventoryItemCodes;
 V19Inventory.Add(AlternateSlotZero);
 TArray<int32> V19Record=Future;
 V19Record[0]=AlternateSlotZero;
 TestFalse(TEXT("V19 rejects a valid owned slot-zero record that differs from its legacy mirror"),
  MoonfallEquipmentLayout::TryRestoreFlexibleFullLoadout(Mirrors,3,
   V19Record.GetData(),V19Record.Num(),19,V19Inventory,Migrated));
 TestTrue(TEXT("Rejected V19 mirror mismatch leaves output unchanged"),Migrated==BeforeRejected);
 TArray<int32> V17Inventory=Original->InventoryItemCodes;
 V17Inventory.Add(AlternateSlotZero);
 TArray<int32> V17Record;
 for(int32 Slot=0;Slot<MoonfallEquipmentLayout::SlotCount;++Slot)V17Record.Add(Future[Slot]);
 V17Record[0]=AlternateSlotZero;
 TestFalse(TEXT("V17 rejects valid owned slot-zero record that differs from legacy mirror"),
  MoonfallEquipmentLayout::TryRestoreFlexibleFullLoadout(Mirrors,3,
   V17Record.GetData(),V17Record.Num(),17,V17Inventory,Migrated));
 TestTrue(TEXT("Rejected V17 mirror mismatch leaves output unchanged"),Migrated==BeforeRejected);
 TArray<int32> V18Inventory=Middle;
 V18Inventory.Add(AlternateSlotZero);
 TArray<int32> V18Record=Middle;
 V18Record[0]=AlternateSlotZero;
 TestFalse(TEXT("V18 rejects valid owned slot-zero record that differs from legacy mirror"),
  MoonfallEquipmentLayout::TryRestoreFlexibleFullLoadout(Mirrors,3,
   V18Record.GetData(),V18Record.Num(),18,V18Inventory,Migrated));
 TestTrue(TEXT("Rejected V18 mirror mismatch leaves output unchanged"),Migrated==BeforeRejected);
 TestFalse(TEXT("V17 rejects truncated nine-slot records"),
  MoonfallEquipmentLayout::TryRestoreFlexibleFullLoadout(Mirrors,3,
   Future.GetData(),MoonfallEquipmentLayout::SlotCount-1,17,Original->InventoryItemCodes,Migrated));
 TestTrue(TEXT("Rejected V17 truncation leaves output unchanged"),Migrated==BeforeRejected);
 TestFalse(TEXT("V18 rejects truncated sixteen-slot records"),
  MoonfallEquipmentLayout::TryRestoreFlexibleFullLoadout(Mirrors,3,
   Middle.GetData(),Middle.Num()-1,18,Original->InventoryItemCodes,Migrated));
 TestTrue(TEXT("Rejected V18 truncation leaves output unchanged"),Migrated==BeforeRejected);
 TestFalse(TEXT("Unsupported V20 equipment schema is rejected"),
  MoonfallEquipmentLayout::TryRestoreFlexibleFullLoadout(Mirrors,3,
   Future.GetData(),Future.Num(),20,Original->InventoryItemCodes,Migrated));
 TestTrue(TEXT("Rejected V20 schema leaves output unchanged"),Migrated==BeforeRejected);
 TArray<int32> WrongV18Slot=Middle;
 // Swap two non-ring positions, preserving all physical ownership counts.
 Swap(WrongV18Slot[9],WrongV18Slot[10]);
 TestFalse(TEXT("V18 rejects an item encoded for a different slot"),
  MoonfallEquipmentLayout::TryRestoreFlexibleFullLoadout(Mirrors,3,
   WrongV18Slot.GetData(),WrongV18Slot.Num(),18,Middle,Migrated));
 TestTrue(TEXT("Rejected V18 slot mismatch leaves output unchanged"),Migrated==BeforeRejected);
 TArray<int32> WrongV19Slot=Future;
 Swap(WrongV19Slot[9],WrongV19Slot[10]);
 TestFalse(TEXT("V19 rejects an item encoded for a different slot"),
  MoonfallEquipmentLayout::TryRestoreFlexibleFullLoadout(Mirrors,3,
   WrongV19Slot.GetData(),WrongV19Slot.Num(),19,Original->InventoryItemCodes,Migrated));
 TestTrue(TEXT("Rejected V19 slot mismatch leaves output unchanged"),Migrated==BeforeRejected);
 TestFalse(TEXT("V19 rejects a missing physical ring copy"),
  MoonfallEquipmentLayout::TryRestoreFlexibleFullLoadout(Mirrors,3,
   Future.GetData(),Future.Num(),19,MissingCopy,Migrated));
 TestTrue(TEXT("Rejected migration leaves output unchanged"),Migrated==BeforeRejected);
 TestFalse(TEXT("V19 rejects truncated slot records"),
  MoonfallEquipmentLayout::TryRestoreFlexibleFullLoadout(Mirrors,3,
   Future.GetData(),Future.Num()-1,19,Original->InventoryItemCodes,Migrated));
 TestTrue(TEXT("Rejected V19 truncation leaves output unchanged"),Migrated==BeforeRejected);
 // Also exercise rejection AFTER the real Unreal serializer has reconstructed
 // the malformed record. A memory-only fixture cannot overwrite player saves.
 const auto RejectSerialized=[this,Original,&BeforeRejected](const TCHAR* Label,
     int32 Version,const TArray<int32>& Record,const TArray<int32>& Bag)->bool {
  Original->Version=Version;
  Original->EquippedSlotCodes=Record;
  Original->InventoryItemCodes=Bag;
  TArray<uint8> Bytes;
  if(!TestTrue(TEXT("Serialize malformed equipment fixture"),
       UGameplayStatics::SaveGameToMemory(Original,Bytes)))return false;
  UMoonfallPersistentSave* Restored=Cast<UMoonfallPersistentSave>(
      UGameplayStatics::LoadGameFromMemory(Bytes));
  if(!TestNotNull(TEXT("Deserialize malformed equipment fixture"),Restored))return false;
  TestEqual(TEXT("Malformed fixture version preserved"),Restored->Version,Version);
  TestTrue(TEXT("Malformed slot record preserved"),Restored->EquippedSlotCodes==Record);
  TestTrue(TEXT("Malformed inventory record preserved"),Restored->InventoryItemCodes==Bag);
  const int32 Legacy[]={Restored->EquippedWeaponCode,Restored->EquippedArmorCode,Restored->EquippedAccessoryCode};
  auto Output=BeforeRejected;
  TestFalse(Label,MoonfallEquipmentLayout::TryRestoreFlexibleFullLoadout(
      Legacy,3,Restored->EquippedSlotCodes.GetData(),Restored->EquippedSlotCodes.Num(),
      Restored->Version,Restored->InventoryItemCodes,Output));
  TestTrue(TEXT("Serialized rejection leaves previous output unchanged"),Output==BeforeRejected);
  return true;
 };
 TArray<int32> ShortV19=Future;
 ShortV19.Pop();
 TArray<int32> LongV19=Future;
 LongV19.Add(0);
 if(!RejectSerialized(TEXT("Serialized V19 missing ring copy rejected"),19,Future,MissingCopy))return false;
 if(!RejectSerialized(TEXT("Serialized V19 wrong slot rejected"),19,WrongV19Slot,Future))return false;
 if(!RejectSerialized(TEXT("Serialized V19 mirror mismatch rejected"),19,V19Record,V19Inventory))return false;
 if(!RejectSerialized(TEXT("Serialized V19 truncated record rejected"),19,ShortV19,Future))return false;
 if(!RejectSerialized(TEXT("Serialized V19 oversized record rejected"),19,LongV19,Future))return false;
 if(!RejectSerialized(TEXT("Serialized V20 future schema rejected"),20,Future,Future))return false;
 // All ten fingers may reference identical codes only when ten physical
 // copies exist. Validate the complete serialized loadout before activation.
 TArray<int32> TenRings=Future;
 const int32 SharedRing=Future[8];
 for(const int32 Slot:MoonfallEquipmentLayout::RingSlots)TenRings[Slot]=SharedRing;
 Original->InventoryItemCodes=TenRings;
 if(!RoundTrip(TenRings,19,true))return false;
 TArray<int32> NineCopies=TenRings;
 NineCopies.RemoveAt(MoonfallEquipmentLayout::RingSlots.back());
 if(!RejectSerialized(TEXT("Serialized ten-ring loadout with nine copies rejected"),19,TenRings,NineCopies))return false;

 return !HasAnyErrors();
}

#endif

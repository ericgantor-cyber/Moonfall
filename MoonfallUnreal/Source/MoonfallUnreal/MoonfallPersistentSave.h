#pragma once
#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "MoonfallPersistentSave.generated.h"

USTRUCT()
struct FMoonfallWorldLootRecord {
 GENERATED_BODY()
 UPROPERTY() FGuid DropId;
 UPROPERTY() int32 ItemCode = 0;
 UPROPERTY() FVector Location = FVector::ZeroVector;
 UPROPERTY() FName LevelId = NAME_None;
};
// One persistent character, not a disposable match/run. Version before changing layout.
USTRUCT()
struct FMoonfallCompanionGearRecord {
 GENERATED_BODY()
 UPROPERTY() int32 Weapon=0;
 UPROPERTY() int32 Armor=0;
 UPROPERTY() int32 Accessory=0;
 int32 Get(int32 Slot) const {return Slot==0?Weapon:Slot==1?Armor:Slot==2?Accessory:0;}
 void Set(int32 Slot,int32 Code){if(Slot==0)Weapon=Code;else if(Slot==1)Armor=Code;else if(Slot==2)Accessory=Code;}
};
USTRUCT()
struct FMoonfallCompanionPartyRecord {
 GENERATED_BODY()
 UPROPERTY() FName ResidentId=NAME_None;
 UPROPERTY() int32 Stance=2;
 UPROPERTY() float Health=100.f;
 // Global cooldown remains even when the follower is dismissed.
 UPROPERTY() int64 AbilityReadyUtc=0;
};
UCLASS()
class MOONFALLUNREAL_API UMoonfallPersistentSave : public USaveGame {
 GENERATED_BODY()
public:
 // V13: persistent classless talent ranks. Older V1-V12 saves load with zero ranks.
 // V14: companion profiles keyed by stable resident ID; old saves default to none.
 // V15 adds companion equipment owned separately from the hero's bag.
 // V16 restores the recruited follower by stable resident ID, never by actor pointer.
 UPROPERTY() int32 Version = 16;
 UPROPERTY() FMoonfallCompanionPartyRecord CompanionParty;
 UPROPERTY() TMap<FName,FMoonfallCompanionGearRecord> CompanionGear;
 UPROPERTY() TMap<FName,int32> CompanionProfiles;
 // Exactly 141 ranks; validated against level, budgets and prerequisites before load.
 UPROPERTY() TArray<int32> TalentRanks;
 // 6 zones x 8 columns x 7 rows, one safe 56-bit mask per zone.
 UPROPERTY() TArray<int64> ExploredZoneFogMasks;
 // V11 stores uncollected enemy loot in the same world-save transaction.
 UPROPERTY() TArray<FMoonfallWorldLootRecord> PendingWorldLoot;
 // One permanent defeat bit per dungeon; older saves default to no victories.
 UPROPERTY() int32 DefeatedDungeonBossMask = 0;
 // 30 dungeons * 14 rooms. 0: ungenerated, positive: pending, -1: collected.
 UPROPERTY() TArray<int32> DungeonTreasureStates;
 UPROPERTY() int32 DiscoveredDungeonMask = 0;
 UPROPERTY() TMap<FName, int32> ResidentQuestStates;
 UPROPERTY() TMap<FName, int32> ResidentBonds;
 UPROPERTY() TMap<FName, int32> FactionReputation;
 // UTC expiration retains cooldown across quit/relaunch; no wall-clock-dependent buff duration.
 UPROPERTY() int64 RacePowerReadyUtc = 0;
 UPROPERTY() int32 RaceId = 0;
 UPROPERTY() bool bRaceChosen = false;
 // For V7 this location changes on shrine discovery, not during autosave.
 // V1-V6 retain their last saved location as a legacy checkpoint.
 UPROPERTY() FVector CheckpointLocation = FVector::ZeroVector;
 UPROPERTY() FName ActiveShrineId = NAME_None;
 UPROPERTY() TArray<FName> DiscoveredShrineIds;
 UPROPERTY() int32 Level = 1;
 UPROPERTY() int32 Experience = 0;
 UPROPERTY() int32 Salvage = 0;
 UPROPERTY() float MaxHealth = 100.f;
 UPROPERTY() TArray<int32> InventoryItemCodes;
 UPROPERTY() int32 EquippedWeaponCode = 0;
 UPROPERTY() int32 EquippedArmorCode = 0;
 UPROPERTY() int32 EquippedAccessoryCode = 0;
 // Reserved for V17; V16 writers leave empty and loaders reject populated data.
 UPROPERTY() TArray<int32> ExtraEquippedCodes;
 UPROPERTY() int32 LeftSpell = 0;
 UPROPERTY() int32 RightSpell = 1;
};

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
UCLASS()
class MOONFALLUNREAL_API UMoonfallPersistentSave : public USaveGame {
 GENERATED_BODY()
public:
 // V14 adds Good/Evil morality and a bounded one-time resident choice ledger.
 // Older V1-V13 saves remain readable and migrate without dropping progress.
 UPROPERTY() int32 Version = 14;
 UPROPERTY() int32 MoralAlignment = 0;
 UPROPERTY() TMap<FName, int32> ResidentMoralChoices;
 // -1 = no decision, 0..2 = one confirmed Council policy.
 UPROPERTY() int32 CouncilDecisionId = -1;
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
 UPROPERTY() int32 LeftSpell = 0;
 UPROPERTY() int32 RightSpell = 1;
};

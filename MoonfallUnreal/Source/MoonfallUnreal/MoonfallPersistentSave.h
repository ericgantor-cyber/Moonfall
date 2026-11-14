#pragma once
#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "MoonfallPersistentSave.generated.h"

// One persistent character, not a disposable match/run. Version before changing layout.
UCLASS()
class MOONFALLUNREAL_API UMoonfallPersistentSave : public USaveGame {
 GENERATED_BODY()
public:
 // Version 7: separate shrine checkpoints from two-second progress autosaves.
 UPROPERTY() int32 Version = 7;
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

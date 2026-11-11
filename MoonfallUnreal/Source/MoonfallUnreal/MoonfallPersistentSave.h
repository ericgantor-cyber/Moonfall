#pragma once
#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "MoonfallPersistentSave.generated.h"

// One persistent character, not a disposable match/run. Version before changing layout.
UCLASS()
class MOONFALLUNREAL_API UMoonfallPersistentSave : public USaveGame {
 GENERATED_BODY()
public:
 UPROPERTY() int32 Version = 5;
 UPROPERTY() TMap<FName, int32> ResidentBonds;
 UPROPERTY() TMap<FName, int32> FactionReputation;
 // UTC expiration retains cooldown across quit/relaunch; no wall-clock-dependent buff duration.
 UPROPERTY() int64 RacePowerReadyUtc = 0;
 UPROPERTY() int32 RaceId = 0;
 UPROPERTY() bool bRaceChosen = false;
 UPROPERTY() FVector CheckpointLocation = FVector::ZeroVector;
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

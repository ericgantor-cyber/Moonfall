#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "MoonfallRewardSaveGame.generated.h"

// Separate, versioned reward progress. Does not replace the equipment save.
UCLASS()
class MOONFALLUNREAL_API UMoonfallRewardSaveGame : public USaveGame
{
 GENERATED_BODY()
public:
 // Keep the legacy slot filename for v1/v2 backward compatibility.
 // The hero and level gates must never silently read different slots.
 static const FString& GetRewardSlotName()
 {
  static const FString Slot(TEXT("Moonfall_RewardProgress_v1"));
  return Slot;
 }
 UPROPERTY(SaveGame) int32 SchemaVersion = 2;
 UPROPERTY(SaveGame) int64 Gold = 0;
 UPROPERTY(SaveGame) int64 ConfirmedKills = 0;
 UPROPERTY(SaveGame) TArray<FName> CollectedKeys;
 UPROPERTY(SaveGame) TArray<FName> CompletedFactionQuests;
 // Stable authored gate IDs. v1 saves load with this list empty.
 UPROPERTY(SaveGame) TArray<FName> UnlockedGateIds;
};

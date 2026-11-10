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
 UPROPERTY(SaveGame) int32 SchemaVersion = 1;
 UPROPERTY(SaveGame) int64 Gold = 0;
 UPROPERTY(SaveGame) int64 ConfirmedKills = 0;
 UPROPERTY(SaveGame) TArray<FName> CollectedKeys;
};

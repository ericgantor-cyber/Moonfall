#pragma once
#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "MoonfallWorldProgressSave.generated.h"

// Isolated Unreal migration data. Never reads or rewrites Godot saves.
UCLASS()
class MOONFALLUNREAL_API UMoonfallWorldProgressSave : public USaveGame {
 GENERATED_BODY()
public:
 UPROPERTY() int32 SchemaVersion = 1;
 UPROPERTY() TArray<int32> DefeatedEncounterIds;
};

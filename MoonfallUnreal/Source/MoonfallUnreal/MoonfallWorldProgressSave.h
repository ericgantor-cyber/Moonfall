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
 // Backward-compatible optional data: absent in older schema-v1 saves.
 UPROPERTY() TArray<int32> DiscoveredChamberIds;
 // Optional on older saves, using existing schema version to avoid loss of defeats.
 UPROPERTY() TArray<int32> DiscoveredRegionIds;
 UPROPERTY() TArray<int32> DiscoveredDungeonEntranceIds;
};

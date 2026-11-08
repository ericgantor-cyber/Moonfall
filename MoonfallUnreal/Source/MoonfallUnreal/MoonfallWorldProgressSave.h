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
 // Optional on old saves: -1 means no user-tracked dungeon objective.
 UPROPERTY() int32 TrackedDungeonQuestId = -1;
 // Optional arrays on legacy SaveGame v1, validated before use.
 UPROPERTY() TArray<int32> GuideTrustMemory;
 UPROPERTY() TArray<int32> GuideFearMemory;
 // Optional per-person relationship history, six regions * three identities.
 UPROPERTY() TArray<int32> NpcTrustMemory;
 UPROPERTY() TArray<int32> NpcFearMemory;
 // Sparse-history-in-spirit: compact fixed 1093-int arrays; no actor serialization.
 UPROPERTY() TArray<int32> ResidentTrustMemory;
 UPROPERTY() TArray<int32> ResidentFearMemory;
 // Optional six-region workshop stock, absent in older Unreal SaveGame slots.
 UPROPERTY() TArray<int32> WorkshopStock;
};

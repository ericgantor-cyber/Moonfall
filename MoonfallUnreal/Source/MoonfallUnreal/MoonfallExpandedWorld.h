#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "MoonfallExpandedWorld.generated.h"

class AMoonfallOverworldTile;
class UInstancedStaticMeshComponent;

// Physically connected outdoor blockout. 6 regions + 1 offshore island.
// Scenery and land spawn only near the player: no always-running actor Tick.
UCLASS()
class MOONFALLUNREAL_API AMoonfallExpandedWorld : public AActor
{
 GENERATED_BODY()
public:
 AMoonfallExpandedWorld();
 virtual void BeginPlay() override;
 virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

 static constexpr int32 MainRegionCount = 6;
 static constexpr int32 RegionCount = 7; // last region is the offshore island
 static constexpr int32 TilesPerRegion = 25;
 static constexpr int32 MaxResidentTiles = 25;
 static constexpr int32 MaxSpawnsPerRefresh = 3;
 static constexpr float LoadRadius = 46000.f;
 static constexpr float UnloadRadius = 60000.f;

 static FVector RegionCenter(int32 RegionIndex);
 static const TCHAR* RegionName(int32 RegionIndex);
 static FVector TileCenter(int32 RegionIndex, int32 Column, int32 Row);

 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Overworld")
 TObjectPtr<UInstancedStaticMeshComponent> ConnectingRoads;

 UFUNCTION(BlueprintPure, Category="Moonfall|Overworld")
 int32 GetResidentTileCount() const { return ResidentTiles.Num(); }

private:
 UPROPERTY(Transient) TMap<int32, TObjectPtr<AMoonfallOverworldTile>> ResidentTiles;
 FTimerHandle StreamingTimer;
 // Lightweight sea-gap movement state; water does not automatically rescue
 // or reposition characters who leave the Nightferry.
 TWeakObjectPtr<class AMoonfallHeroPawn> SeaSwimmer;
 void UpdateSeaSwimming();

 void BuildConnectingRoads();
 void AddRoad(const FVector& Start, const FVector& End, float Width);
 void RefreshNearbyTiles();
};

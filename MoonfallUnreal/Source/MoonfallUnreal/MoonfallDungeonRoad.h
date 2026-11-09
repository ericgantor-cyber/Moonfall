#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MoonfallDungeonRoad.generated.h"

class UInstancedStaticMeshComponent;

// Lightweight connected walkable route from the arena to all 30 dungeon
// entrances. Geometry is a temporary world blockout, not finished biome art.
UCLASS()
class MOONFALLUNREAL_API AMoonfallDungeonRoad : public AActor
{
 GENERATED_BODY()
public:
 AMoonfallDungeonRoad();
 virtual void OnConstruction(const FTransform& Transform) override;
 static constexpr float ZoneSpacing = 9000.f;
 static constexpr float DungeonSpacing = 1600.f;
 static constexpr float DungeonStartY = -2700.f;
 static FVector GetDungeonOrigin(int32 ZoneIndex, int32 DungeonIndex);

private:
 UPROPERTY(VisibleAnywhere, Category="Moonfall|World")
 UInstancedStaticMeshComponent* PathFloors;
};

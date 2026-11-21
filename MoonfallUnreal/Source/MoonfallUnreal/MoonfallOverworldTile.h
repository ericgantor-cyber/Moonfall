#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MoonfallOverworldTile.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
class UInstancedStaticMeshComponent;
class UTextRenderComponent;

// One 200m x 200m, zero-Tick, deterministic overworld blockout tile.
// Scenic props do not collide; the shared ground DOES support walking/nav.
UCLASS()
class MOONFALLUNREAL_API AMoonfallOverworldTile : public AActor
{
 GENERATED_BODY()
public:
 AMoonfallOverworldTile();
 virtual void BeginPlay() override;

 static constexpr float TileSize = 20000.f;
 static constexpr int32 TilesPerSide = 5;

 // Set by SpawnActorDeferred before FinishSpawning.
 int32 RegionIndex = 0;
 int32 Column = 0;
 int32 Row = 0;

 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Overworld")
 TObjectPtr<UStaticMeshComponent> Terrain;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Overworld")
 TObjectPtr<UInstancedStaticMeshComponent> Scenery;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Overworld")
 TObjectPtr<UInstancedStaticMeshComponent> Buildings;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Overworld")
 TObjectPtr<UTextRenderComponent> RegionSign;

private:
 UPROPERTY(Transient) TObjectPtr<UStaticMesh> CubeAsset;
 UPROPERTY(Transient) TObjectPtr<UStaticMesh> ConeAsset;
 UPROPERTY(Transient) TObjectPtr<UStaticMesh> CylinderAsset;
 UPROPERTY(Transient) TObjectPtr<UStaticMesh> SphereAsset;
 void BuildBlockout();
};

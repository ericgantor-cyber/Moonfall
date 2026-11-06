#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UStaticMesh;
class UDirectionalLightComponent;
class UExponentialHeightFogComponent;
#include "MoonfallArena.generated.h"
UCLASS()
class MOONFALLUNREAL_API AMoonfallArena : public AActor {
 GENERATED_BODY()
public:
 AMoonfallArena();
 virtual void BeginPlay() override;
 UPROPERTY(EditAnywhere, Category="Dungeon") int32 EnemyCount = 5;
 UPROPERTY(EditDefaultsOnly, Category="Moonfall|Art") TSubclassOf<class AMoonfallEnemy> CrowEnemyClass;
 UPROPERTY(EditDefaultsOnly, Category="Moonfall|Art") UStaticMesh* RuinMeshArt = nullptr;
 UPROPERTY(EditDefaultsOnly, Category="Moonfall|Art") UStaticMesh* GroundMeshArt = nullptr;
 UPROPERTY(EditDefaultsOnly, Category="Moonfall|Art") UMaterialInterface* GroundMaterial = nullptr;
 UPROPERTY(EditDefaultsOnly, Category="Moonfall|Art") UStaticMesh* BoundaryMeshArt = nullptr;
 UPROPERTY(EditDefaultsOnly, Category="Moonfall|Art") UMaterialInterface* BoundaryMaterial = nullptr;
 UPROPERTY(EditDefaultsOnly, Category="Moonfall|Art") UMaterialInterface* RuinStoneMaterial = nullptr;
 UPROPERTY(EditAnywhere, Category="Moonfall|Art", meta=(ClampMin="0",ClampMax="48")) int32 RubbleCount = 24;
 UPROPERTY(EditDefaultsOnly, Category="Moonfall|Art") UStaticMesh* DeadTreeMeshArt = nullptr;
 UPROPERTY(EditDefaultsOnly, Category="Moonfall|Art") UMaterialInterface* DeadTreeMaterial = nullptr;
 UPROPERTY(EditAnywhere, Category="Moonfall|Art", meta=(ClampMin="0",ClampMax="16")) int32 DeadTreeCount = 10;
 UPROPERTY(EditAnywhere, Category="Moonfall|Atmosphere") bool bEnableMoonlight = true;
 UPROPERTY(EditAnywhere, Category="Moonfall|Atmosphere") bool bEnableLowFog = true;
 UPROPERTY(EditAnywhere, Category="Moonfall|Atmosphere", meta=(ClampMin="0.0",ClampMax="0.025")) float GroundFogDensity = 0.012f;
 UPROPERTY(EditDefaultsOnly, Category="Moonfall|Art") UStaticMesh* RitualStoneMeshArt = nullptr;
 UPROPERTY(EditDefaultsOnly, Category="Moonfall|Art") UMaterialInterface* RitualStoneMaterial = nullptr;
 UPROPERTY(EditDefaultsOnly, Category="Moonfall|Ritual Art") UStaticMesh* SigilTileMeshArt = nullptr;
 UPROPERTY(EditDefaultsOnly, Category="Moonfall|Ritual Art") UMaterialInterface* SigilTileMaterial = nullptr;
 UPROPERTY(EditAnywhere, Category="Moonfall|Ritual Art") bool bShowRitualCircle = true;
private:
 UPROPERTY(VisibleAnywhere) UInstancedStaticMeshComponent* RuinPieces;
 UPROPERTY(VisibleAnywhere) UInstancedStaticMeshComponent* DeadTrees;
 UPROPERTY(VisibleAnywhere) UInstancedStaticMeshComponent* RitualStones;
 UPROPERTY(VisibleAnywhere) UInstancedStaticMeshComponent* RitualSigil;
 UPROPERTY(VisibleAnywhere) UDirectionalLightComponent* Moonlight;
 UPROPERTY(VisibleAnywhere) UExponentialHeightFogComponent* GroundFog;
 void CreateBlock(FVector Position, FVector Scale, bool bGround);
 void CreateDecoration(FVector Position, FVector Scale);
 void AddFoliage(FVector Position, FVector Scale, FRotator Rotation);
 void AddRitualStone(FVector Position, FVector Scale, FRotator Rotation);
 void AddSigilTile(FVector Position, FVector Scale, FRotator Rotation);
 void CreateAtmosphereLight(FVector Position, FLinearColor Color, float Intensity);
};

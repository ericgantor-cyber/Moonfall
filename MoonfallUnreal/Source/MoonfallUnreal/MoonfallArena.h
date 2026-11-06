#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UStaticMesh;
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
 UPROPERTY(EditDefaultsOnly, Category="Moonfall|Art") UStaticMesh* RitualStoneMeshArt = nullptr;
 UPROPERTY(EditDefaultsOnly, Category="Moonfall|Art") UMaterialInterface* RitualStoneMaterial = nullptr;
private:
 UPROPERTY(VisibleAnywhere) UInstancedStaticMeshComponent* RuinPieces;
 UPROPERTY(VisibleAnywhere) UInstancedStaticMeshComponent* DeadTrees;
 UPROPERTY(VisibleAnywhere) UInstancedStaticMeshComponent* RitualStones;
 void CreateBlock(FVector Position, FVector Scale, bool bGround);
 void CreateDecoration(FVector Position, FVector Scale);
 void AddFoliage(FVector Position, FVector Scale, FRotator Rotation);
 void AddRitualStone(FVector Position, FVector Scale, FRotator Rotation);
 void CreateAtmosphereLight(FVector Position, FLinearColor Color, float Intensity);
};

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
class UInstancedStaticMeshComponent;
class UMaterialInterface;
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
 UPROPERTY(EditDefaultsOnly, Category="Moonfall|Art") UMaterialInterface* RuinStoneMaterial = nullptr;
 UPROPERTY(EditAnywhere, Category="Moonfall|Art", meta=(ClampMin="0",ClampMax="48")) int32 RubbleCount = 24;
private:
 UPROPERTY(VisibleAnywhere) UInstancedStaticMeshComponent* RuinPieces;
 void CreateBlock(FVector Position, FVector Scale);
 void CreateDecoration(FVector Position, FVector Scale);
 void CreateAtmosphereLight(FVector Position, FLinearColor Color, float Intensity);
};

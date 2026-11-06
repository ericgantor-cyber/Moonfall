#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MoonfallArena.generated.h"
UCLASS()
class MOONFALLUNREAL_API AMoonfallArena : public AActor {
 GENERATED_BODY()
public:
 AMoonfallArena();
 virtual void BeginPlay() override;
 UPROPERTY(EditAnywhere, Category="Dungeon") int32 EnemyCount = 5;
 UPROPERTY(EditDefaultsOnly, Category="Moonfall|Art") TSubclassOf<class AMoonfallEnemy> CrowEnemyClass;
private:
 void CreateBlock(FVector Position, FVector Scale);
 void CreateDecoration(FVector Position, FVector Scale);
 void CreateAtmosphereLight(FVector Position, FLinearColor Color, float Intensity);
};

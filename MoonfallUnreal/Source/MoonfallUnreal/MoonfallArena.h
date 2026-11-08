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
 virtual void Tick(float DeltaSeconds) override;
 UPROPERTY(EditAnywhere, Category="World") bool bContinuousWorld = true;
 UPROPERTY(EditAnywhere, Category="Dungeon") int32 EnemyCount = 5;
private:
 TSet<int32> BuiltRooms;
 TSet<int32> SpawnedEncounters;
 void BuildWorld();
 void StreamWorld();
 bool SpawnEncounter(int32 Id, FVector Position, int32 Region, bool Elite, bool Boss);
 void CreateBlock(FVector Position, FVector Scale);
};

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
 // Read-only C++ access for the compact explored-only minimap.
 const TSet<int32>& GetDiscoveredChambers() const { return DiscoveredChambers; }
 const TSet<int32>& GetDiscoveredRegions() const { return DiscoveredRegions; }
 const TSet<int32>& GetDiscoveredDungeonEntrances() const { return DiscoveredDungeonEntrances; }
private:
 TSet<int32> BuiltRooms;
 TSet<int32> SpawnedEncounters;
 TSet<int32> DefeatedEncounters;
 TSet<int32> DiscoveredChambers;
 TSet<int32> DiscoveredRegions;
 TSet<int32> DiscoveredDungeonEntrances;
 TMap<int32, TWeakObjectPtr<AMoonfallEnemy>> ActiveEncounters;
 bool bWorldSavePending = false;
 void LoadWorldProgress();
 void ObserveDefeatedEncounters();
 void SaveWorldProgress();
 static bool IsValidEncounterId(int32 Id);
 void BuildWorld();
 void StreamWorld();
 bool SpawnEncounter(int32 Id, FVector Position, int32 Region, bool Elite, bool Boss);
 void CreateBlock(FVector Position, FVector Scale);
};

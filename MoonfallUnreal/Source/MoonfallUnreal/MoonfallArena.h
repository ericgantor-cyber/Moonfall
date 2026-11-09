#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "MoonfallArena.generated.h"
UCLASS()
class MOONFALLUNREAL_API AMoonfallArena : public AActor {
 GENERATED_BODY()
public:
 AMoonfallArena();
 virtual void BeginPlay() override;
 UPROPERTY(EditAnywhere, Category="Dungeon") int32 EnemyCount = 5;
 // Explicit dungeon designation. Open-world zone arenas never run the 5-11 bonus-pack rule.
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon|Spawning") bool bIsDungeonEncounter = false;
 // Read-only counters useful for HUDs, debugging, and dungeon regression tests.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Dungeon|Spawning") int32 BonusGroupsSpawned = 0;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Dungeon|Spawning") int32 BonusMobsSpawned = 0;
 // Arena preset only; this does not create a streamed open-world biome system.
 // Profiles: Forest, Cave, Foundry, Swamp, Desert, Tundra, Volcanic,
 // Ruins, Crypt, Coast, Highlands, Eclipse. Unknown names fall back to Forest.
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon|Spawning") FName SpawnBiome = FName(TEXT("Forest"));
 // NAME_None keeps the previous combined showcase. Select one god per arena
 // by setting this to z_void_reaper or nyra_eclipse_queen in the editor.
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon|Divine") FName FeaturedDivineBossId = NAME_None;
private:
 void CreateBlock(FVector Position, FVector Scale);
 void CheckDungeonEntry();
 void SpawnNextBonusGroup();
 TArray<int32> PendingBonusGroupSizes;
 int32 NextBonusGroup = 0;
 int32 TerrainPlacementAttempts = 0;
 FTimerHandle BonusSpawnTimer;
 FTimerHandle DungeonEntryTimer;
};

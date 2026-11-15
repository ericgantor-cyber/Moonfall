#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TimerManager.h"
#include "MoonfallUnrealGameMode.generated.h"

// Thirty physical dungeons in six world regions. Geometry is spawned in small
// time-sliced batches; the GameMode never polls each dungeon every frame.
UCLASS()
class MOONFALLUNREAL_API AMoonfallUnrealGameMode : public AGameModeBase
{
 GENERATED_BODY()
public:
 AMoonfallUnrealGameMode();
 virtual void BeginPlay() override;

 static constexpr int32 DungeonZoneCount = 6;
 static constexpr int32 DungeonsPerZone = 5;
 static constexpr int32 TotalDungeons = DungeonZoneCount * DungeonsPerZone;
 static constexpr float DungeonLaneSpacing = 1350.f;

 UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Moonfall|Dungeon|World")
 bool bGenerateDungeonWorld = true;
 // Allows the atlas/quest system to locate a specific dungeon without
 // scanning every spawned actor or changing the existing save format.
 UFUNCTION(BlueprintPure, Category="Moonfall|Dungeon|World")
 bool GetDungeonEntranceTransform(int32 ZoneIndex, int32 DungeonIndex,
    FTransform& OutTransform) const;

private:
 FTimerHandle DungeonSpawnTimer;
 int32 NextDungeonSlot = 0;
 TArray<uint8> AuthoredDungeonSlots;
 void SpawnDungeonBatch();
};

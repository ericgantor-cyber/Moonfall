#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TimerManager.h"
#include "MoonfallUnrealGameMode.generated.h"

class AMoonfallDungeonLayout;

// Thirty physical dungeons in six world regions. Geometry is spawned in small
// time-sliced batches; the GameMode never polls each dungeon every frame.
UCLASS()
class MOONFALLUNREAL_API AMoonfallUnrealGameMode : public AGameModeBase
{
 GENERATED_BODY()
public:
 AMoonfallUnrealGameMode();
 virtual void BeginPlay() override;
 virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

 static constexpr int32 DungeonZoneCount = 6;
 static constexpr int32 DungeonsPerZone = 5;
 static constexpr int32 TotalDungeons = DungeonZoneCount * DungeonsPerZone;
 static constexpr float DungeonLaneSpacing = 1350.f;

 // Per-map world bootstrap flags. Existing gameplay retains both defaults.
 // Authored testing worlds can disable the arena without touching saves.
 UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Moonfall|World|Startup")
 bool bGenerateLegacyArena = true;
 UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Moonfall|Dungeon|World")
 bool bGenerateDungeonWorld = true;
 // Per-timer-pass upper bound, clamped again in C++ for Blueprint safety.
 UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Moonfall|Dungeon|Performance",
   meta=(ClampMin="1", ClampMax="8"))
 int32 MaxDungeonLayoutsPerBatch = 8;
 // Limit work after the first layout in each batch to reduce frame spikes.
 UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Moonfall|Dungeon|Performance",
   meta=(ClampMin="0.5", ClampMax="8.0"))
 float DungeonSpawnBudgetMs = 2.5f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Dungeon|World")
 int32 ReadyDungeonCount = 0;
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Dungeon|World")
 void OnDungeonWorldReady(int32 ReadyCount);
 // Allows the atlas/quest system to locate a specific dungeon without
 // scanning every spawned actor or changing the existing save format.
 UFUNCTION(BlueprintPure, Category="Moonfall|Dungeon|World")
 bool GetDungeonEntranceTransform(int32 ZoneIndex, int32 DungeonIndex,
    FTransform& OutTransform) const;

private:
 FTimerHandle DungeonSpawnTimer;
 int32 NextDungeonSlot = 0;
 int32 NextFailedDungeonRetry = 0;
 bool bDungeonWorldReadyNotified = false;
 bool bFinalDungeonAuditComplete = false;
 TArray<uint8> AuthoredDungeonSlots;
 TArray<TWeakObjectPtr<AMoonfallDungeonLayout>> AuthoredDungeonActors;
 TArray<uint8> SpawnedDungeonSlots;
 TArray<TWeakObjectPtr<AMoonfallDungeonLayout>> SpawnedDungeonActors;
 TArray<uint8> RetriedDungeonSlots;
 TArray<int32> FailedDungeonSlots;
 void SpawnDungeonBatch();
 bool ReconcileAuthoredDungeonSlot(int32 Slot);
 bool ReconcileSpawnedDungeonSlot(int32 Slot);
};

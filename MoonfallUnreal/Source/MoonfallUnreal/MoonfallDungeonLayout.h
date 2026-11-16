#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MoonfallDungeonLayout.generated.h"

class AMoonfallHeroPawn;
class AMoonfallGroundLoot;
class UBoxComponent;
class UInstancedStaticMeshComponent;
class UPrimitiveComponent;

// Authored, connected 14-room dungeon inside the continuous world.
// No ticking actors, scene transitions, automated enemies, or loot claims.
UCLASS(Blueprintable)
class MOONFALLUNREAL_API AMoonfallDungeonLayout : public AActor
{
 GENERATED_BODY()
public:
 AMoonfallDungeonLayout();
 virtual void OnConstruction(const FTransform& Transform) override;
 virtual void BeginPlay() override;

 static constexpr int32 ChamberCount = 14;
 static constexpr float ChamberLength = 1200.f;
 static constexpr float ChamberWidth = 900.f;

 UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Moonfall|Dungeon")
 int32 ZoneIndex = 0;
 UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Moonfall|Dungeon")
 int32 DungeonIndex = 0;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Dungeon")
 UInstancedStaticMeshComponent* Floors;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Dungeon")
 UInstancedStaticMeshComponent* Walls;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Dungeon")
 UBoxComponent* EntranceTrigger;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Dungeon")
 UBoxComponent* FinalChamberTrigger;

 // Index 0 entrance, 1 combat, 2 treasure, 3 shrine, 4 puzzle,
 // 5 hazard, 6 final boss. Interior triggers announce only; they do not
 // spawn enemies or grant loot automatically.
 UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Moonfall|Dungeon|Rooms")
 int32 LayoutSeed = 0;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Dungeon|Rooms")
 bool bRandomizeChambers = true;
 UFUNCTION(BlueprintPure, Category="Moonfall|Dungeon|Rooms")
 int32 GetEffectiveLayoutSeed() const;
 UFUNCTION(BlueprintPure, Category="Moonfall|Dungeon|Rooms")
 int32 GetChamberType(int32 ChamberIndex) const;
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Dungeon|Rooms")
 void OnChamberEntered(AMoonfallHeroPawn* Hero, int32 ChamberIndex, int32 ChamberType);
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Dungeon|Rooms")
 TArray<TObjectPtr<UBoxComponent>> ChamberTriggers;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Dungeon|Rooms")
 UInstancedStaticMeshComponent* HazardPlates;

 // Spawn only upon entering combat chambers, never in startup waves.
 // Spawn five to six regular enemies per combat room; elite rooms stay solo.
 // Lifetime spawn budget scales from 30 to 66, with the global 69-111 cap.
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Dungeon|Combat",
   meta=(ClampMin="5", ClampMax="6"))
 int32 EnemiesPerCombatChamber = 6;
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Dungeon|Combat")
 void OnChamberEncounterSpawned(AMoonfallHeroPawn* Hero, int32 ChamberIndex,
   int32 SpawnedCount, bool bEliteEncounter);
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Dungeon|Treasure")
 void OnDungeonTreasureSpawned(AMoonfallHeroPawn* Hero, int32 ChamberIndex, int32 ItemCode);
 // Three pawn-only side-alcove traps. The 360-unit center route stays safe.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Dungeon|Hazards")
 UBoxComponent* HazardTriggerA;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Dungeon|Hazards")
 UBoxComponent* HazardTriggerB;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Dungeon|Hazards")
 UBoxComponent* HazardTriggerC;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Dungeon|Hazards",
   meta=(ClampMin="0.0", ClampMax="100.0"))
 float HazardDamage = 12.f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Dungeon|Hazards",
   meta=(ClampMin="0.5", ClampMax="20.0"))
 float HazardCooldownSeconds = 2.f;

 // Entering the last chamber starts one native regional boss encounter.
 // Victory is handled by the existing enemy death/XP/ground-loot pipeline.
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Dungeon|Boss")
 void OnDungeonBossSpawned(AMoonfallHeroPawn* Hero, int32 Zone, int32 Dungeon);
 // Final-room entry does not count as a kill and grants no reward.
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Dungeon")
 void OnFinalChamberEntered(AMoonfallHeroPawn* Hero);

private:
 float NextHazardDamageAt = -1.f;
 // Last valid overlap clock tracks backward time jumps.
 float LastHazardObservedTime = -1.f;
 UFUNCTION()
 void HandleHazardOverlap(UPrimitiveComponent* Overlapped, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
    const FHitResult& SweepResult);
 uint16 EnteredChambersMask = 0;
 int32 SpawnedEncounterCount = 0;
 TWeakObjectPtr<AMoonfallGroundLoot> SpawnedTreasure[ChamberCount];
 bool SpawnTreasureChamber(AMoonfallHeroPawn* Hero, int32 ChamberIndex);
 bool StartCombatChamber(AMoonfallHeroPawn* Hero, int32 ChamberIndex);
 bool StartFinalBossChamber(AMoonfallHeroPawn* Hero);
 bool bFinalChamberNotified = false;
 void BuildHazardChambers(int32 OutChambers[3]) const;
 UFUNCTION()
 void HandleChamberOverlap(UPrimitiveComponent* Overlapped, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
    const FHitResult& SweepResult);
 UFUNCTION()
 void HandleEntrance(UPrimitiveComponent* Overlapped, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
    const FHitResult& SweepResult);
 UFUNCTION()
 void HandleFinalChamber(UPrimitiveComponent* Overlapped, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
    const FHitResult& SweepResult);
};

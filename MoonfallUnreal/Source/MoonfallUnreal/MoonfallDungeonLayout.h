#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MoonfallDungeonLayout.generated.h"

class UInstancedStaticMeshComponent;
class USceneComponent;
class UBoxComponent;
class UPrimitiveComponent;
class AMoonfallHeroPawn;

// One world-placeable, continuous 14-chamber dungeon. No separate game mode,
// enemy waves, teleports, or individual actors for each wall/floor piece.
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

 // Zone IDs 0..5 and dungeon IDs 0..4, matching persistent entrance data.
 UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Moonfall|Dungeon")
 int32 ZoneIndex = 0;
 UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Moonfall|Dungeon")
 int32 DungeonIndex = 0;

 // Level authors can bind this to an authored boss encounter. It is not a
 // kill/clear event, and does not spawn a boss on its own.
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Dungeon")
 void OnFinalChamberEntered(AMoonfallHeroPawn* Hero);
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Dungeon")
 UBoxComponent* EntranceTrigger;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Dungeon")
 UBoxComponent* FinalChamberTrigger;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Dungeon")
 UInstancedStaticMeshComponent* Floors;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Dungeon")
 UInstancedStaticMeshComponent* Walls;
 // Three optional physical trap volumes in side alcoves of chambers 4, 8, 12.
 // A safe central path remains open. No per-frame polling or enemy waves.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Dungeon")
 UInstancedStaticMeshComponent* HazardPlates;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Dungeon")
 UBoxComponent* HazardTriggerA;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Dungeon")
 UBoxComponent* HazardTriggerB;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Dungeon")
 UBoxComponent* HazardTriggerC;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Dungeon|Hazards",
   meta=(ClampMin="0.0", ClampMax="100.0"))
 float HazardDamage = 12.f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Dungeon|Hazards",
   meta=(ClampMin="0.5", ClampMax="20.0"))
 float HazardCooldownSeconds = 2.f;

private:
 float NextHazardDamageAt = -1.f;
 UFUNCTION()
 void HandleHazardOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
    const FHitResult& SweepResult);
 bool bFinalChamberNotified = false;
 UFUNCTION()
 void HandleEntrance(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
    const FHitResult& SweepResult);
 UFUNCTION()
 void HandleFinalChamber(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
    const FHitResult& SweepResult);
};

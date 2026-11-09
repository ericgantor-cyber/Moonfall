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

private:
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

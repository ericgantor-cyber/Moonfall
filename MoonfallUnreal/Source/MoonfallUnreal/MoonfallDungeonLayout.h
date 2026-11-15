#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MoonfallDungeonLayout.generated.h"

class AMoonfallHeroPawn;
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

 // Final-room entry is not a boss kill and grants no reward automatically.
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Dungeon")
 void OnFinalChamberEntered(AMoonfallHeroPawn* Hero);

private:
 bool bFinalChamberNotified = false;
 UFUNCTION()
 void HandleEntrance(UPrimitiveComponent* Overlapped, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
    const FHitResult& SweepResult);
 UFUNCTION()
 void HandleFinalChamber(UPrimitiveComponent* Overlapped, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
    const FHitResult& SweepResult);
};

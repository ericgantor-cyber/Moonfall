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

 // Final-room entry is not a boss kill and grants no reward automatically.
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Dungeon")
 void OnFinalChamberEntered(AMoonfallHeroPawn* Hero);

private:
 uint16 EnteredChambersMask = 0;
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

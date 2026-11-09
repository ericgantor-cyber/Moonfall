#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MoonfallDungeonEntrance.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UPrimitiveComponent;

// Place one of these in an authored zone; no world transition or enemy wave.
// The trigger records the discovery through the hero's persistent quest ledger.
UCLASS(Blueprintable)
class MOONFALLUNREAL_API AMoonfallDungeonEntrance : public AActor
{
 GENERATED_BODY()
public:
 AMoonfallDungeonEntrance();
 virtual void BeginPlay() override;

 // Zero-based zone [0,5] and dungeon [0,4]; set a unique pair per entrance.
 UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Moonfall|Dungeon")
 int32 ZoneIndex = 0;
 UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Moonfall|Dungeon")
 int32 DungeonIndex = 0;

 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Dungeon")
 UBoxComponent* DiscoveryTrigger;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Dungeon")
 UStaticMeshComponent* EntranceMarker;

private:
 UFUNCTION()
 void OnEntranceOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
    const FHitResult& SweepResult);
};

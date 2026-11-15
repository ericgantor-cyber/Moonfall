#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MoonfallDungeonEntrance.generated.h"

class AMoonfallHeroPawn;
class UBoxComponent;
class UStaticMeshComponent;
class UPrimitiveComponent;

// Placeable non-persistent dungeon entrance foundation. Discovery ledger and
// XP rewards are intentionally deferred until their save API reaches main.
UCLASS(Blueprintable)
class MOONFALLUNREAL_API AMoonfallDungeonEntrance : public AActor
{
 GENERATED_BODY()
public:
 AMoonfallDungeonEntrance();
 virtual void BeginPlay() override;

 // Zero-based zone 0..5 and dungeon 0..4. Invalid IDs make this trigger inert.
 UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Moonfall|Dungeon")
 int32 ZoneIndex = 0;
 UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Moonfall|Dungeon")
 int32 DungeonIndex = 0;

 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Dungeon")
 UBoxComponent* DiscoveryTrigger;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Dungeon")
 UStaticMeshComponent* EntranceMarker;

 // Fires once per actor lifetime only; not a persisted discovery or XP reward.
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Dungeon")
 void OnEntranceVisited(AMoonfallHeroPawn* Hero, int32 Zone, int32 Dungeon);

private:
 bool bVisitedThisSession = false;
 UFUNCTION()
 void OnEntranceOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
   UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
   const FHitResult& SweepResult);
};

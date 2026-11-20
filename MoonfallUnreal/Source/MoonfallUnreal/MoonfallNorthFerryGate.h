#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MoonfallNorthFerryGate.generated.h"

class USceneComponent;
class USphereComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class AMoonfallHeroPawn;

// Place two mutually linked actors on authored WALKABLE harbor docks in one
// world. Walking onto a marked deck boards automatically (no new input bind).
// This is an optional greybox teleport, not an ocean simulation or a saved
// travel unlock. Never spawn it in the legacy six-zone map by default.
UCLASS(Blueprintable)
class MOONFALLUNREAL_API AMoonfallNorthFerryGate : public AActor {
 GENERATED_BODY()
public:
 AMoonfallNorthFerryGate();
 virtual void OnConstruction(const FTransform& Transform) override;

 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Sea Travel")
 TObjectPtr<USphereComponent> BoardingVolume;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Sea Travel")
 TObjectPtr<USceneComponent> ArrivalMarker;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Sea Travel")
 TObjectPtr<UStaticMeshComponent> DeckMarker;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Sea Travel")
 TObjectPtr<UTextRenderComponent> FerrySign;

 UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Moonfall|Sea Travel")
 TObjectPtr<AMoonfallNorthFerryGate> PairedGate;
 // Set false for mainland harbor, true for Shiverwake Coast harbor.
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Sea Travel")
 bool bIslandHarbor = false;
 // Explicit opt-in prevents accidental teleporting in unfinished levels.
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Sea Travel")
 bool bEnableSeaPassage = false;
 // Transient only. A permanent first-voyage unlock needs a versioned save.
 UPROPERTY(VisibleAnywhere, Transient, BlueprintReadOnly, Category="Moonfall|Sea Travel")
 bool bReachedIslandThisSession = false;

 // Can also be invoked from a future mouse/controller boarding prompt.
 UFUNCTION(BlueprintCallable, Category="Moonfall|Sea Travel")
 bool BoardFerry(AMoonfallHeroPawn* Hero);
 // Fired AFTER successful teleport; hook authored audio/fade/quest cues here.
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Sea Travel")
 void OnFerryArrival(AMoonfallHeroPawn* Hero, bool bFirstIslandLanding);

private:
 UFUNCTION()
 void OnBoardingOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
                        bool bFromSweep, const FHitResult& SweepResult);
};

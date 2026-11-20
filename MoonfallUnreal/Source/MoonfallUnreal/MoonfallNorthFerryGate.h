#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "MoonfallNorthFerryGate.generated.h"

class USceneComponent;
class USphereComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class AMoonfallHeroPawn;
class AMoonfallNorthGothicFerryActor;

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
 virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

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
 // Automatic, non-steerable crossing requires a decorative vessel at each
 // dock. Assign the visual ships only in an isolated authored preview map.
 UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Moonfall|Sea Travel")
 TObjectPtr<AMoonfallNorthGothicFerryActor> SailingVessel;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Sea Travel",
           meta=(ClampMin="4.0", ClampMax="30.0", UIMin="4.0", UIMax="30.0"))
 float VoyageDurationSeconds = 11.f;
 UPROPERTY(VisibleAnywhere, Transient, BlueprintReadOnly, Category="Moonfall|Sea Travel")
 bool bAutoVoyageInProgress = false;
 // Readable voyage progress for future HUD, controller and accessibility UI.
 // Phase: 0 idle, 1 departing, 2 open sea, 3 approaching dock.
 UPROPERTY(VisibleAnywhere, Transient, BlueprintReadOnly, Category="Moonfall|Sea Travel")
 int32 VoyageSecondsRemaining = 0;
 UPROPERTY(VisibleAnywhere, Transient, BlueprintReadOnly, Category="Moonfall|Sea Travel")
 int32 VoyagePhaseIndex = 0;
 // Transient only. A permanent first-voyage unlock needs a versioned save.
 UPROPERTY(VisibleAnywhere, Transient, BlueprintReadOnly, Category="Moonfall|Sea Travel")
 bool bReachedIslandThisSession = false;

 // Can also be invoked from a future mouse/controller boarding prompt.
 UFUNCTION(BlueprintCallable, Category="Moonfall|Sea Travel")
 bool BoardFerry(AMoonfallHeroPawn* Hero);
 // Fired AFTER successful teleport; hook authored audio/fade/quest cues here.
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Sea Travel")
 void OnFerryArrival(AMoonfallHeroPawn* Hero, bool bFirstIslandLanding);
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Sea Travel")
 void OnAutomaticVoyageStarted(AMoonfallHeroPawn* Hero);
 // At most once per displayed second or phase change, never on every 0.1s
 // movement update. Add voiceover, fog and UI cues in an authored Blueprint.
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Sea Travel")
 void OnAutomaticVoyageProgress(AMoonfallHeroPawn* Hero,
                                int32 SecondsRemaining, int32 PhaseIndex);

private:
 void UpdateAutomaticVoyage();
 void StopAutomaticVoyage();
 void UpdateVoyagePresentation(AMoonfallHeroPawn* Hero, float ElapsedSeconds);
 FTimerHandle AutomaticVoyageTimer;
 TWeakObjectPtr<AMoonfallHeroPawn> PendingVoyager;
 TWeakObjectPtr<AMoonfallNorthFerryGate> PendingArrivalGate;
 TWeakObjectPtr<AMoonfallNorthGothicFerryActor> PendingShip;
 FTransform OriginalShipTransform = FTransform::Identity;
 FVector SailingFrom = FVector::ZeroVector;
 FVector SailingTo = FVector::ZeroVector;
 float VoyageStartWorldSeconds = 0.f;
 float ActiveVoyageDurationSeconds = 11.f;
 UFUNCTION()
 void OnBoardingOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
                        bool bFromSweep, const FHitResult& SweepResult);
};

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "MoonfallGothicFerry.generated.h"

class AMoonfallHeroPawn;
class UStaticMeshComponent;
class UTextRenderComponent;

// Real-time reversible crossing: the boat carries the pawn across open water.
// No level loads, no instantaneous coast-to-coast teleport, no extra actor Tick.
UCLASS()
class MOONFALLUNREAL_API AMoonfallGothicFerry : public AActor
{
 GENERATED_BODY()
public:
 AMoonfallGothicFerry();
 virtual void BeginPlay() override;
 virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

 static constexpr float DockReach = 950.f; // Covers the 800cm safe disembark point.
 static constexpr float CrossingDuration = 14.f;

 static FVector DockLocation(bool bAtIsland);
 // Exposed for mouse/controller input, but every call validates proximity,
 // life state, boat state and whether the character is already a passenger.
 UFUNCTION(BlueprintCallable, Category="Moonfall|World|Ferry")
 bool TryBoard(AMoonfallHeroPawn* Hero);
 UFUNCTION(BlueprintPure, Category="Moonfall|World|Ferry")
 bool IsUnderway() const { return bUnderway; }
 UFUNCTION(BlueprintPure, Category="Moonfall|World|Ferry")
 bool IsAtIsland() const { return bAtIsland; }
 UFUNCTION(BlueprintPure, Category="Moonfall|World|Ferry")
 float GetCrossingProgress() const;

 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|World|Ferry")
 TObjectPtr<UStaticMeshComponent> Hull;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|World|Ferry")
 TObjectPtr<UStaticMeshComponent> Mast;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|World|Ferry")
 TObjectPtr<UStaticMeshComponent> Sail;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|World|Ferry")
 TObjectPtr<UTextRenderComponent> VesselName;

private:
 FTimerHandle CrossingTimer;
 TWeakObjectPtr<AMoonfallHeroPawn> Passenger;
 float CrossingStartedAt = 0.f;
 bool bUnderway = false;
 bool bAtIsland = false;
 void AdvanceCrossing();
 void FinishCrossing(bool bArrived);
};

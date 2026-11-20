#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MoonfallNorthGothicFerryActor.generated.h"

class USceneComponent;
class UInstancedStaticMeshComponent;
class UPointLightComponent;
class UCameraComponent;
class UBoxComponent;

// Original dark-gothic ferry *visual prototype* for the isolated Northern
// Expanse test map. The boarding/landing logic stays in MoonfallNorthFerryGate.
// No Tick, no physics, no blocking collision, no legacy saves or divine assets.
UCLASS(Blueprintable)
class MOONFALLUNREAL_API AMoonfallNorthGothicFerryActor : public AActor {
 GENERATED_BODY()
public:
 AMoonfallNorthGothicFerryActor();
 virtual void OnConstruction(const FTransform& Transform) override;
 // Cosmetic cloth-like flutter using only existing sail instances, never
 // colliding or affecting the automatic course or travel timer.
 void AnimateCrimsonSails(float ElapsedSeconds, bool bReduceMotion);
 void ResetCrimsonSails();

 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|North Ferry")
 bool bShowGothicFerry = true;

 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|North Ferry")
 TObjectPtr<UInstancedStaticMeshComponent> HullPieces;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|North Ferry")
 TObjectPtr<UInstancedStaticMeshComponent> GothicIronwork;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|North Ferry")
 TObjectPtr<UInstancedStaticMeshComponent> CrimsonSails;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|North Ferry")
 TObjectPtr<UInstancedStaticMeshComponent> RavenFigurehead;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|North Ferry")
 TObjectPtr<UInstancedStaticMeshComponent> CrimsonLanterns;

 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|North Ferry")
 TObjectPtr<UPointLightComponent> PortLanternLight;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|North Ferry")
 TObjectPtr<UPointLightComponent> StarboardLanternLight;

 // Optional auto-voyage viewpoint follows the vessel without a helm or
 // player-driven steering. The harbor gate owns restoration on cancel/arrival.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|North Ferry")
 TObjectPtr<UCameraComponent> VoyageCamera;
 // Gameplay-only collision. Unlike the original nonblocking render instances,
 // this movable deck supports CharacterMovement-based passengers.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|North Ferry")
 TObjectPtr<UBoxComponent> PassengerDeck;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|North Ferry")
 TObjectPtr<USceneComponent> PassengerBoardingMarker;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|North Ferry")
 TObjectPtr<UBoxComponent> PortPassengerRail;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|North Ferry")
 TObjectPtr<UBoxComponent> StarboardPassengerRail;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|North Ferry")
 TObjectPtr<UBoxComponent> BowPassengerRail;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|North Ferry")
 TObjectPtr<UBoxComponent> SternPassengerRail;

private:
 TArray<FTransform> SailRestTransforms;
 bool bSailsWereAnimated = false;
};

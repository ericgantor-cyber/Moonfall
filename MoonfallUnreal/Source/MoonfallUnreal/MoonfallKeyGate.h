#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MoonfallKeyGate.generated.h"

class USceneComponent;
class UBoxComponent;
class UStaticMeshComponent;
class UPrimitiveComponent;

// Place in a UE5 dungeon level or Blueprint. Walking up to a gate with its
// already-collected quest key opens it; keys are never consumed.
UCLASS()
class MOONFALLUNREAL_API AMoonfallKeyGate : public AActor
{
 GENERATED_BODY()
public:
 AMoonfallKeyGate();

 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Moonfall|Quest Gate")
 FName RequiredKeyId = FName(TEXT("fallen_gate_key"));

 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Quest Gate")
 bool bUnlocked = false;

 UFUNCTION(BlueprintCallable, Category="Moonfall|Quest Gate")
 bool TryUnlock(AActor* Candidate);

 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Quest Gate")
 void OnGateUnlocked();

 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Quest Gate")
 void OnGateLocked(FName MissingKeyId);

protected:
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Quest Gate")
 TObjectPtr<USceneComponent> GateRoot;

 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Quest Gate")
 TObjectPtr<UStaticMeshComponent> GateMesh;

 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Quest Gate")
 TObjectPtr<UBoxComponent> KeyProximity;

 UFUNCTION()
 void OnKeyProximityBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
     UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
     const FHitResult& SweepResult);
};

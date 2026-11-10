#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MoonfallKeyGate.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

// Place in a UE5 dungeon level or Blueprint. Gates stay locked on approach.
// An explicit interaction calls TryUnlock; valid quest keys are not consumed.
UCLASS()
class MOONFALLUNREAL_API AMoonfallKeyGate : public AActor
{
 GENERATED_BODY()
public:
 AMoonfallKeyGate();

 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Moonfall|Quest Gate")
 FName RequiredKeyId = FName(TEXT("fallen_gate_key"));

 // Give every authored gate a unique ID to preserve an opened door between
 // sessions. NAME_None remains a compatible, non-persistent gate.
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Moonfall|Quest Gate")
 FName GateId = NAME_None;

 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Quest Gate")
 bool bUnlocked = false;

 // Only an explicit UI/Blueprint interaction attempts unlocking.
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
 TObjectPtr<UTextRenderComponent> GateLabel;

};

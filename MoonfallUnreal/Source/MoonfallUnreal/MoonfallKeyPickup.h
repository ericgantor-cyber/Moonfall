#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MoonfallKeyPickup.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class USphereComponent;
class UTextRenderComponent;

// AC-style discoverable key: world pickup requires an explicit interaction.
// It is not added to the equipment bag and does not consume inventory slots.
UCLASS()
class MOONFALLUNREAL_API AMoonfallKeyPickup : public AActor
{
 GENERATED_BODY()
public:
 AMoonfallKeyPickup();
 virtual void BeginPlay() override;

 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Moonfall|Keys")
 FName KeyId = FName(TEXT("fallen_gate_key"));

 UFUNCTION(BlueprintCallable, Category="Moonfall|Keys")
 bool TryCollect(AActor* Candidate);

protected:
 void RefreshKeyLabel();
 UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> KeyRoot;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> KeyMesh;
 // Larger query-only target for mouse-look click interactions.
 UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> AimTarget;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> KeyLabel;
};

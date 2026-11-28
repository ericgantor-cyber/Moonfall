#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MoonfallAirPocket.generated.h"

class USphereComponent;
class UStaticMeshComponent;

// Designer-placeable breathing zone. Does not alter swimming physics.
UCLASS()
class MOONFALLUNREAL_API AMoonfallAirPocket : public AActor
{
    GENERATED_BODY()
public:
    AMoonfallAirPocket();
    virtual void Tick(float DeltaSeconds) override;
    UPROPERTY(EditAnywhere, Category="Moonfall|Water")
    bool bAnimateBubbles = true;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Water")
    USphereComponent* BreathingZone;
    bool ContainsEye(const FVector& EyeLocation) const;
private:
    UPROPERTY(Transient)
    TArray<TObjectPtr<UStaticMeshComponent>> BubbleMarkers;
    bool bBubblesMoving = false;
};

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MoonfallAirPocket.generated.h"

class USphereComponent;

// Designer-placeable breathing zone. Does not alter swimming physics.
UCLASS()
class MOONFALLUNREAL_API AMoonfallAirPocket : public AActor
{
    GENERATED_BODY()
public:
    AMoonfallAirPocket();
    virtual void BeginPlay() override;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Water")
    USphereComponent* BreathingZone;
    bool ContainsEye(const FVector& EyeLocation) const;
};

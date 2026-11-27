#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MoonfallGrottoTreasure.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class AMoonfallHeroPawn;

// Session-only discovery reward; never writes campaign equipment or currencies.
UCLASS()
class MOONFALLUNREAL_API AMoonfallGrottoTreasure : public AActor
{
    GENERATED_BODY()
public:
    AMoonfallGrottoTreasure();
    UPROPERTY(EditInstanceOnly, Category="Moonfall|Water", meta=(ClampMin="0", ClampMax="5"))
    int32 GrottoIndex = 0;
    UPROPERTY(VisibleAnywhere, Category="Moonfall|Water")
    USphereComponent* PickupZone;
    UPROPERTY(VisibleAnywhere, Category="Moonfall|Water")
    UStaticMeshComponent* ChestMesh;
    bool TryClaim(AMoonfallHeroPawn* Hero);
};

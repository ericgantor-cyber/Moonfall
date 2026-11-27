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
    // -1 is an ordinary grotto chest; 0-3 gather, 4-6 fish.
    UPROPERTY(EditInstanceOnly, Category="Moonfall|Water", meta=(ClampMin="-1", ClampMax="6"))
    int32 ResourceIndex = -1;
    UPROPERTY(VisibleAnywhere, Category="Moonfall|Water")
    USphereComponent* PickupZone;
    UPROPERTY(VisibleAnywhere, Category="Moonfall|Water")
    UStaticMeshComponent* ChestMesh;
    bool TryClaim(AMoonfallHeroPawn* Hero, bool bManual = true);
};

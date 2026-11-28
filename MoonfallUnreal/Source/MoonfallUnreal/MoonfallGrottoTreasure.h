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
    UPROPERTY(EditInstanceOnly, Category="Moonfall|Water", meta=(ClampMin="0", ClampMax="14"))
    // IDs 0-5 are legacy loan gear; 6-14 are regional tonic caches.
    int32 GrottoIndex = 0;
    // -1 is an ordinary grotto chest; 0-47 are gathering nodes (12 base and 36 regional).
    UPROPERTY(EditInstanceOnly, Category="Moonfall|Water", meta=(ClampMin="-1", ClampMax="47"))
    int32 ResourceIndex = -1;
    UPROPERTY(VisibleAnywhere, Category="Moonfall|Water")
    USphereComponent* PickupZone;
    UPROPERTY(VisibleAnywhere, Category="Moonfall|Water")
    UStaticMeshComponent* ChestMesh;
    bool TryClaim(AMoonfallHeroPawn* Hero, bool bManual = true);
private:
    void RestoreResource();
    bool bResourceDepleted=false;
};

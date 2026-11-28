#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MoonfallMasteryOrb.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UPrimitiveComponent;
class AMoonfallHeroPawn;

// Authored exploration collectible; no tick, physics or production reward.
UCLASS()
class MOONFALLUNREAL_API AMoonfallMasteryOrb : public AActor {
 GENERATED_BODY()
public:
 AMoonfallMasteryOrb();
 virtual void BeginPlay() override;
 void CheckHeroAtSpawn(AMoonfallHeroPawn* Hero);
 // Native deferred spawn config; reject changes after BeginPlay.
 bool InitializeRouteIdentity(int32 InRegion,int32 InIndex,bool bInHidden);
private:
 UPROPERTY(EditInstanceOnly,Category="Moonfall|Mastery",meta=(ClampMin="0",ClampMax="7")) int32 Region=INDEX_NONE;
 UPROPERTY(EditInstanceOnly,Category="Moonfall|Mastery",meta=(ClampMin="0",ClampMax="63")) int32 Index=INDEX_NONE;
 UPROPERTY(EditInstanceOnly,Category="Moonfall|Mastery") bool bHiddenOrb=false;
 UPROPERTY(VisibleAnywhere) USphereComponent* PickupRange=nullptr;
 UPROPERTY(VisibleAnywhere) UStaticMeshComponent* OrbMesh=nullptr;
 UPROPERTY(VisibleAnywhere) UTextRenderComponent* OrbLabel=nullptr;
 UFUNCTION() void OnPickup(UPrimitiveComponent* Component,AActor* Other,UPrimitiveComponent* OtherComponent,int32 BodyIndex,bool bSweep,const FHitResult& Hit);
 bool bClaimed=false;
};

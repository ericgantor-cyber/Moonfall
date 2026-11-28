#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MoonfallEnemyProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class UProjectileMovementComponent;
class AMoonfallEnemy;
class AMoonfallHeroPawn;

UCLASS()
class MOONFALLUNREAL_API AMoonfallEnemyProjectile : public AActor
{
 GENERATED_BODY()
public:
 AMoonfallEnemyProjectile();
 void Initialize(AMoonfallEnemy* SourceEnemy,float InDamage,const FVector& Direction,
                 float Speed,bool bSpellcaster);
 UFUNCTION(BlueprintImplementableEvent,Category="Combat|Ranged")
 void OnProjectileImpact(FVector ImpactPoint,bool bParried,bool bPerfectDodge);
private:
 UPROPERTY(VisibleAnywhere,Category="Combat|Ranged")
 USphereComponent* Collision = nullptr;
 UPROPERTY(VisibleAnywhere,Category="Combat|Ranged")
 UStaticMeshComponent* Visual = nullptr;
 UPROPERTY(VisibleAnywhere,Category="Combat|Ranged")
 UPointLightComponent* ArcaneGlow = nullptr;
 UPROPERTY(VisibleAnywhere,Category="Combat|Ranged")
 UProjectileMovementComponent* ProjectileMovement = nullptr;
 TWeakObjectPtr<AMoonfallEnemy> SourceEnemy;
 float Damage = 1.f;
 bool bResolved = false;
 UFUNCTION()
 void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent,AActor* OtherActor,
   UPrimitiveComponent* OtherComponent,int32 OtherBodyIndex,bool bFromSweep,
   const FHitResult& SweepResult);
 UFUNCTION()
 void HandleComponentHit(UPrimitiveComponent* HitComponent,AActor* OtherActor,
   UPrimitiveComponent* OtherComponent,FVector NormalImpulse,const FHitResult& Hit);
 void ResolveHeroImpact(AActor* OtherActor);
};

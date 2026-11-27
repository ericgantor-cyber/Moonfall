#include "MoonfallEnemyProjectile.h"
#include "MoonfallEnemy.h"
#include "MoonfallHeroPawn.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Components/PointLightComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/World.h"

AMoonfallEnemyProjectile::AMoonfallEnemyProjectile()
{
 Collision=CreateDefaultSubobject<USphereComponent>(TEXT("ProjectileCollision"));
 SetRootComponent(Collision);
 Collision->InitSphereRadius(16.f);
 Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
 Collision->SetCollisionObjectType(ECC_WorldDynamic);
 Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
 Collision->SetCollisionResponseToChannel(ECC_WorldStatic,ECR_Block);
 Collision->SetCollisionResponseToChannel(ECC_WorldDynamic,ECR_Block);
 // Pawns overlap the projectile so enemy bodies do not intercept shots.
 // The player is resolved explicitly in the overlap callback.
 Collision->SetCollisionResponseToChannel(ECC_Pawn,ECR_Overlap);
 Collision->SetGenerateOverlapEvents(true);
 Collision->SetCanEverAffectNavigation(false);
 Collision->OnComponentBeginOverlap.AddDynamic(this,&AMoonfallEnemyProjectile::HandleBeginOverlap);
 Collision->OnComponentHit.AddDynamic(this,&AMoonfallEnemyProjectile::HandleComponentHit);

 Visual=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileVisual"));
 Visual->SetupAttachment(Collision);
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(
   TEXT("/Engine/BasicShapes/Sphere.Sphere"));
 if(Sphere.Succeeded()) Visual->SetStaticMesh(Sphere.Object);
 Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 Visual->SetCollisionResponseToAllChannels(ECR_Ignore);
 Visual->SetCastShadow(false);
 Visual->SetCanEverAffectNavigation(false);
 Visual->SetRelativeScale3D(FVector(0.3f));

 ArcaneGlow=CreateDefaultSubobject<UPointLightComponent>(TEXT("ArcaneGlow"));
 ArcaneGlow->SetupAttachment(Collision);
 ArcaneGlow->SetMobility(EComponentMobility::Movable);
 ArcaneGlow->SetIntensity(950.f);
 ArcaneGlow->SetAttenuationRadius(220.f);
 ArcaneGlow->SetLightColor(FLinearColor(0.12f,0.48f,1.f));
 ArcaneGlow->SetCastShadows(false);
 ArcaneGlow->SetVisibility(false);

 ProjectileMovement=CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
 ProjectileMovement->SetUpdatedComponent(Collision);
 ProjectileMovement->InitialSpeed=850.f;
 ProjectileMovement->MaxSpeed=850.f;
 ProjectileMovement->ProjectileGravityScale=0.f;
 ProjectileMovement->bRotationFollowsVelocity=true;
 ProjectileMovement->bShouldBounce=false;
 ProjectileMovement->bSweepCollision=true;
 ProjectileMovement->bForceSubStepping=true;
 ProjectileMovement->MaxSimulationTimeStep=0.05f;
 ProjectileMovement->MaxSimulationIterations=4;

 PrimaryActorTick.bCanEverTick=false;
 InitialLifeSpan=3.5f;
}

void AMoonfallEnemyProjectile::Initialize(AMoonfallEnemy* InSourceEnemy,float InDamage,
 const FVector& Direction,float Speed,bool bSpellcaster)
{
 if(!IsValid(InSourceEnemy)||InSourceEnemy->IsActorBeingDestroyed()
    ||!FMath::IsFinite(InDamage)||InDamage<=0.f||Direction.ContainsNaN()
    ||Direction.IsNearlyZero()||!FMath::IsFinite(Speed)||Speed<300.f||Speed>1400.f){
  Destroy();
  return;
 }
 SourceEnemy=InSourceEnemy;
 SetOwner(InSourceEnemy);
 Collision->IgnoreActorWhenMoving(InSourceEnemy,true);
 Damage=FMath::Clamp(InDamage,1.f,1000.f);
 const FVector SafeDirection=Direction.GetSafeNormal();
 SetActorRotation(SafeDirection.Rotation());
 ProjectileMovement->InitialSpeed=Speed;
 ProjectileMovement->MaxSpeed=Speed;
 ProjectileMovement->Velocity=SafeDirection*Speed;
 Visual->SetRelativeScale3D(bSpellcaster?FVector(0.38f):FVector(0.48f,0.12f,0.12f));
 ArcaneGlow->SetVisibility(bSpellcaster);
}

void AMoonfallEnemyProjectile::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent,
 AActor* OtherActor,UPrimitiveComponent* OtherComponent,int32 OtherBodyIndex,bool bFromSweep,
 const FHitResult& SweepResult)
{
 ResolveHeroImpact(OtherActor);
}

void AMoonfallEnemyProjectile::HandleComponentHit(UPrimitiveComponent* HitComponent,
 AActor* OtherActor,UPrimitiveComponent* OtherComponent,FVector NormalImpulse,
 const FHitResult& Hit)
{
 if(Cast<AMoonfallHeroPawn>(OtherActor)){
  ResolveHeroImpact(OtherActor);
  return;
 }
 // Solid world geometry stops the shot. Pawns are overlap-only, so other
 // enemies and companions never become accidental damage targets.
 if(!bResolved){
  bResolved=true;
  Destroy();
 }
}

void AMoonfallEnemyProjectile::ResolveHeroImpact(AActor* OtherActor)
{
 if(bResolved)return;
 AMoonfallHeroPawn* Hero=Cast<AMoonfallHeroPawn>(OtherActor);
 if(!IsValid(Hero)||!Hero->IsPlayerControlled()||Hero->IsActorBeingDestroyed()
    ||!FMath::IsFinite(Hero->Health)||Hero->Health<=0.f)return;
 bResolved=true;
 AMoonfallEnemy* Enemy=SourceEnemy.Get();
 if(!IsValid(Enemy)||Enemy->IsActorBeingDestroyed()
    ||!FMath::IsFinite(Enemy->Health)||Enemy->Health<=0.f){
  Destroy();
  return;
 }
 const FVector ImpactPoint=GetActorLocation();
 if(Hero->TryRewardPerfectDodge()){
  Enemy->NotifyRangedProjectileCountered(true);
  if(IsValid(Enemy)&&!Enemy->IsActorBeingDestroyed())
   Enemy->OnRangedProjectileResolved(false,true);
  OnProjectileImpact(ImpactPoint,false,true);
  if(IsValid(this)&&!IsActorBeingDestroyed())Destroy();
  return;
 }
 const bool bParried=Hero->ApplyMoonfallDamageFrom(Damage,ImpactPoint);
 if(bParried)Enemy->NotifyRangedProjectileCountered(false);
 else Enemy->OnEnemyAttack();
 if(IsValid(Enemy)&&!Enemy->IsActorBeingDestroyed())
  Enemy->OnRangedProjectileResolved(bParried,false);
 if(!IsValid(this)||IsActorBeingDestroyed())return;
 OnProjectileImpact(ImpactPoint,bParried,false);
 if(IsValid(this)&&!IsActorBeingDestroyed())Destroy();
}

#include "MoonfallEnemy.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "MoonfallHeroPawn.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "DrawDebugHelpers.h"
#include "CollisionQueryParams.h"
AMoonfallEnemy::AMoonfallEnemy() {
  Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
  SetRootComponent(Body);
  static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
  if (Sphere.Succeeded()) Body->SetStaticMesh(Sphere.Object);
  Body->SetCollisionProfileName(TEXT("BlockAll"));
  Body->SetRelativeScale3D(FVector(0.85f,0.85f,1.4f));
  PrimaryActorTick.bCanEverTick = true;
  PrimaryActorTick.TickInterval = 0.1f;
}
void AMoonfallEnemy::TakeSpellDamage(float Amount) {
  if (Health <= 0.f) return;
  Health = FMath::Max(0.0f, Health - FMath::Max(0.0f, Amount));
  OnHitFeedback(Amount, Health);
  // Hit feedback is now tracked by the HUD rather than persistent debug strings.
  LastHitAmount = Amount;
  LastHitTime = GetWorld() ? GetWorld()->GetTimeSeconds() : -10.f;
  HitReactionEndsAt = LastHitTime + 0.22f;
  if (Health <= 0.0f) {
   if (AMoonfallHeroPawn* Hero = Cast<AMoonfallHeroPawn>(UGameplayStatics::GetPlayerPawn(this, 0))) Hero->AwardExperience(25);
   SetActorHiddenInGame(true); SetActorEnableCollision(false); SetActorTickEnabled(false); SetLifeSpan(1.25f); }
}

void AMoonfallEnemy::Tick(float DeltaSeconds) {
 Super::Tick(DeltaSeconds);
 if (Health <= 0.f || !GetWorld()) return;
 const float Now = GetWorld()->GetTimeSeconds();
 const bool Staggered = Now < HitReactionEndsAt;
 if (IsValid(Body)) {
  const float Windup = WindupEndsAt > Now ? FMath::Clamp((WindupEndsAt - Now) / FMath::Max(0.1f, CurrentWindup), 0.f, 1.f) : 0.f;
  const float Scale = Staggered ? 0.92f : 1.f + (Windup > 0.f ? (1.f - Windup) * 0.18f : 0.f);
  Body->SetRelativeScale3D(FVector(0.85f * Scale, 0.85f * Scale, 1.4f / Scale));
 }
 if (Staggered) {
  // A successful hit interrupts an enemy's windup, rewarding reactive combat.
  WindupEndsAt = -1.f;
  NextAttackAt = FMath::Max(NextAttackAt, Now + 0.35f);
  return;
 }
 AMoonfallHeroPawn* Hero = Cast<AMoonfallHeroPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
 if (!IsValid(Hero) || Hero->Health <= 0.f || Hero->bOptionsOpen) return;
 FVector Difference = Hero->GetActorLocation() - GetActorLocation();
 Difference.Z = 0.f;
 const float Distance = Difference.Size();
 // Far-away enemies use a slower think rate. Return to responsive combat
 // polling when the hero approaches, without per-frame work on idle mobs.
 const bool bInCombatArea = Distance < 2200.f;
 const float DesiredTickInterval = bInCombatArea ? 0.1f : 0.4f;
 if (!FMath::IsNearlyEqual(PrimaryActorTick.TickInterval, DesiredTickInterval)) {
  SetActorTickInterval(DesiredTickInterval);
 }
 if (!bInCombatArea) {
  WindupEndsAt = -1.f;
  CachedWaypoint = FVector::ZeroVector;
  NextPathRefreshAt = 0.f;
  return;
 }
 if (WindupEndsAt > 0.f && Now >= WindupEndsAt) {
  WindupEndsAt = -1.f;
  // Heavy elite crushes demand recovery, leaving a counterattack window.
  NextAttackAt = Now + AttackCooldown + (bElite && CurrentAttackMultiplier > 2.f ? 0.8f : 0.f);
  // Attack resolves against the hero's CURRENT position, so dodging works.
  if (Distance <= AttackRange * CurrentAttackRangeMultiplier) {
   // Do not damage the hero through solid arena walls or cover.
   FCollisionQueryParams SightParams(SCENE_QUERY_STAT(MoonfallMeleeSight), false);
   SightParams.AddIgnoredActor(this);
   FHitResult SightHit;
   const bool bBlocked = GetWorld()->LineTraceSingleByChannel(
       SightHit, GetActorLocation(), Hero->GetActorLocation(), ECC_Visibility, SightParams)
       && SightHit.GetActor() != Hero;
   if (!bBlocked) {
    Hero->ApplyMoonfallDamage(AttackDamage * CurrentAttackMultiplier);
    OnEnemyAttack();
   }
  }
  return;
 }
 // Keep the enemy committed to a telegraphed strike until it resolves. A player
 // stepping outside the basic range should not silently cancel a wider sweep.
 if (WindupEndsAt > 0.f) {
  DrawDebugSphere(GetWorld(), GetActorLocation(), AttackRange * CurrentAttackRangeMultiplier, 16, CurrentAttackMultiplier > 2.f ? FColor::Purple : (CurrentAttackMultiplier > 1.5f ? FColor::Orange : FColor::Red), false, 0.12f);
  DrawDebugString(GetWorld(), GetActorLocation() + FVector(0.f,0.f,160.f), CurrentAttackMultiplier > 2.f ? TEXT("ELITE CRUSH") : (CurrentAttackMultiplier > 1.5f ? TEXT("HEAVY") : (CurrentAttackRangeMultiplier > 1.f ? TEXT("SWEEP") : TEXT("STRIKE"))), nullptr, FColor::Red, 0.12f, false);
  return;
 }
 if (Distance > AttackRange && Distance < 2200.f) {
  FHitResult Hit;
  FVector Target = Hero->GetActorLocation();
  // Solve paths only when the cache expires. Use the live hero position when
  // navigation is unavailable, and never chase an uninitialized waypoint.
  if (UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld())) {
   if (Now >= NextPathRefreshAt) {
    NextPathRefreshAt = Now + 0.5f + FMath::FRandRange(0.f, 0.2f);
    FNavLocation Point;
    if (Nav->ProjectPointToNavigation(Target, Point)) {
     UNavigationPath* Path = Nav->FindPathToLocationSynchronously(GetWorld(), GetActorLocation(), Point.Location, this);
     CachedWaypoint = (IsValid(Path) && !Path->IsPartial() && Path->PathPoints.Num() > 1)
       ? Path->PathPoints[1] : Point.Location;
    } else {
     CachedWaypoint = Target;
    }
   }
   if (!CachedWaypoint.IsNearlyZero()) {
    FVector ToWaypoint = CachedWaypoint - GetActorLocation();
    ToWaypoint.Z = 0.f;
    if (ToWaypoint.SizeSquared() < FMath::Square(65.f)) {
     // Reaching an intermediate waypoint must advance the path, not idle
     // until a stale waypoint is recomputed on the normal timer.
     NextPathRefreshAt = 0.f;
    } else {
     Target = CachedWaypoint;
    }
   }
  }
  FVector Direction = Target - GetActorLocation();
  Direction.Z = 0.f;
  AddActorWorldOffset(Direction.GetSafeNormal() * ChaseSpeed * DeltaSeconds, true, &Hit);
 } else if (Distance <= AttackRange) {
  const float Now = GetWorld()->GetTimeSeconds();
  if (WindupEndsAt < 0.f && Now >= NextAttackAt) {
   // Alternate quick strikes, heavy attacks and wider sweeps with distinct warnings.
   // An elite cannot chain another attack during its recovery window.
   const int32 Pattern = AttackSequence++ % (bElite ? 4 : 3);
   CurrentWindup = Pattern == 0 ? 0.38f : (Pattern == 1 ? 0.85f : (Pattern == 2 ? 0.65f : 1.05f));
   CurrentAttackMultiplier = Pattern == 1 ? 1.8f : (Pattern == 2 ? 0.75f : (Pattern == 3 ? 2.25f : 1.f));
   CurrentAttackRangeMultiplier = Pattern == 2 ? 1.25f : (Pattern == 3 ? 1.4f : 1.f);
   WindupEndsAt = Now + CurrentWindup;
  }
 }
}

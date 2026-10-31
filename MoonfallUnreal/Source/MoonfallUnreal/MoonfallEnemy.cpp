#include "MoonfallEnemy.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "MoonfallHeroPawn.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "DrawDebugHelpers.h"
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
  if (Health <= 0.0f) { SetActorHiddenInGame(true); SetActorEnableCollision(false); SetActorTickEnabled(false); SetLifeSpan(1.25f); }
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
 if (WindupEndsAt > 0.f && Now >= WindupEndsAt) {
  WindupEndsAt = -1.f;
  NextAttackAt = Now + AttackCooldown;
  // Attack resolves against the hero's CURRENT position, so dodging works.
  if (Distance <= AttackRange * CurrentAttackRangeMultiplier) {
   Hero->ApplyMoonfallDamage(AttackDamage * CurrentAttackMultiplier);
   OnEnemyAttack();
  }
  return;
 }
 if (Distance > AttackRange && Distance < 2200.f) {
  WindupEndsAt = -1.f;
  FHitResult Hit;
  FVector Target = Hero->GetActorLocation();
  if (UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld())) {
   FNavLocation Point;
   // Cache both nav projection and path solves to avoid repeated work each tick.
   if (Now >= NextPathRefreshAt) {
    NextPathRefreshAt = Now + 0.5f + FMath::FRandRange(0.f, 0.2f);
    if (Nav->ProjectPointToNavigation(Target, Point)) {
     UNavigationPath* Path = Nav->FindPathToLocationSynchronously(GetWorld(), GetActorLocation(), Point.Location, this);
     CachedWaypoint = (IsValid(Path) && !Path->IsPartial() && Path->PathPoints.Num() > 1) ? Path->PathPoints[1] : Point.Location;
    } else CachedWaypoint = Target;
   }
   Target = CachedWaypoint;
  }
  FVector Direction = Target - GetActorLocation();
  Direction.Z = 0.f;
  AddActorWorldOffset(Direction.GetSafeNormal() * ChaseSpeed * DeltaSeconds, true, &Hit);
 } else if (Distance <= AttackRange) {
  const float Now = GetWorld()->GetTimeSeconds();
  if (WindupEndsAt < 0.f && Now >= NextAttackAt) {
   // Alternate quick strikes, heavy attacks and wider sweeps with distinct warnings.
   const int32 Pattern = AttackSequence++ % 3;
   CurrentWindup = Pattern == 0 ? 0.38f : (Pattern == 1 ? 0.85f : 0.65f);
   CurrentAttackMultiplier = Pattern == 1 ? 1.8f : (Pattern == 2 ? 0.75f : 1.f);
   CurrentAttackRangeMultiplier = Pattern == 2 ? 1.25f : 1.f;
   WindupEndsAt = Now + CurrentWindup;
  }
  if (WindupEndsAt > 0.f) {
   DrawDebugSphere(GetWorld(), GetActorLocation(), AttackRange * CurrentAttackRangeMultiplier, 16, CurrentAttackMultiplier > 1.5f ? FColor::Orange : FColor::Red, false, 0.12f);
   DrawDebugString(GetWorld(), GetActorLocation() + FVector(0.f,0.f,160.f), CurrentAttackMultiplier > 1.5f ? TEXT("HEAVY") : (CurrentAttackRangeMultiplier > 1.f ? TEXT("SWEEP") : TEXT("STRIKE")), nullptr, FColor::Red, 0.12f, false);
  }
 }
}

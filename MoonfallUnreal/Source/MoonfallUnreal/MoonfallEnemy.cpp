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
void AMoonfallEnemy::BeginPlay() {
 Super::BeginPlay();
 MaxHealthAtSpawn = FMath::Max(1.f, Health);
 BossMaxHealth = bBoss ? MaxHealthAtSpawn : 0.f;
}

void AMoonfallEnemy::TakeSpellDamage(float Amount) {
  if (MaxHealthAtSpawn <= 0.f) MaxHealthAtSpawn = FMath::Max(1.f, Health);
  if (bBoss) BossMaxHealth = MaxHealthAtSpawn;
  if (Health <= 0.f) return;
  // Elites briefly brace against repeated hits, but the first strike retains
  // full impact and every hit still interrupts their attack windup.
  const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
  const float EffectiveDamage = bElite && Now < HitReactionEndsAt ? Amount * 0.75f : Amount;
  Health = FMath::Max(0.0f, Health - FMath::Max(0.0f, EffectiveDamage));
  if (bBoss && Health > 0.f) {
   const float HealthRatio = Health / FMath::Max(1.f, MaxHealthAtSpawn);
   const int32 NewPhase = HealthRatio <= 0.33f ? 3 : (HealthRatio <= 0.66f ? 2 : 1);
   if (NewPhase > BossPhase) {
    BossPhase = NewPhase;
    WindupEndsAt = -1.f;
    BossWarningEndTime = -1.f;
    NextAttackAt = Now + 1.5f;
    BossPhaseMessageUntil = Now + 1.5f;
    AttackSequence = 0;
    BossSpecialMove = 0;
    bComboSecondHitPending = false;
    bComboSecondHitActive = false;
    CachedWaypoint = FVector::ZeroVector;
    OnBossPhaseChanged(BossPhase);
   }
  }
  OnHitFeedback(EffectiveDamage, Health);
  // Hit feedback is now tracked by the HUD rather than persistent debug strings.
  LastHitAmount = EffectiveDamage;
  LastHitTime = GetWorld() ? GetWorld()->GetTimeSeconds() : -10.f;
  HitReactionEndsAt = LastHitTime + 0.22f;
  if (Health <= 0.0f) {
   if (AMoonfallHeroPawn* Hero = Cast<AMoonfallHeroPawn>(UGameplayStatics::GetPlayerPawn(this, 0))) { Hero->AwardExperience(bBoss ? 125 : (bElite ? 50 : 25)); Hero->AwardEquipmentDrop(bElite, bBoss); }
   SetActorHiddenInGame(true); SetActorEnableCollision(false); SetActorTickEnabled(false); SetLifeSpan(1.25f); }
}

void AMoonfallEnemy::Tick(float DeltaSeconds) {
 Super::Tick(DeltaSeconds);
 if (Health <= 0.f || !GetWorld()) return;
 const float Now = GetWorld()->GetTimeSeconds();
 const bool Staggered = Now < HitReactionEndsAt;
 if (bBoss && Now < BossPhaseMessageUntil) {
  DrawDebugString(GetWorld(), GetActorLocation() + FVector(0.f, 0.f, 230.f),
    BossPhase == 3 ? TEXT("BOSS PHASE 3") : TEXT("BOSS PHASE 2"),
    nullptr, FColor::Purple, 0.12f, false);
 }
 if (IsValid(Body)) {
  const float Windup = WindupEndsAt > Now ? FMath::Clamp((WindupEndsAt - Now) / FMath::Max(0.1f, CurrentWindup), 0.f, 1.f) : 0.f;
  const bool bBossTelegraph = bBoss && WindupEndsAt > Now;
  const float Anticipation = bBossTelegraph ? (1.f - Windup) : 0.f;
  const float Recovery = bBoss && WindupEndsAt < 0.f && NextAttackAt > Now
    ? FMath::Clamp((NextAttackAt - Now) / FMath::Max(0.2f, AttackCooldown + 0.8f), 0.f, 1.f) : 0.f;
  const float Scale = Staggered ? 0.92f : 1.f + (Windup > 0.f ? (1.f - Windup) * 0.18f : 0.f);
  const float Squash = bBossTelegraph ? 1.f - 0.12f * Anticipation : 1.f - 0.09f * Recovery;
  Body->SetRelativeScale3D(FVector(0.85f * Scale / FMath::Max(0.75f,Squash), 0.85f * Scale / FMath::Max(0.75f,Squash), 1.4f * Squash / Scale));
 }
 if (bBoss && Now < BossPhaseMessageUntil) {
  // A readable phase-change recovery: no movement or attacks during the transition.
  WindupEndsAt = -1.f;
  BossWarningEndTime = -1.f;
  bComboSecondHitPending = false;
  bComboSecondHitActive = false;
  return;
 }
 if (Staggered) {
  // A successful hit interrupts an enemy's windup, rewarding reactive combat.
  WindupEndsAt = -1.f;
  BossWarningEndTime = -1.f;
  bComboSecondHitPending = false;
  bComboSecondHitActive = false;
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
 const bool bDashing = bBoss && BossSpecialMove == 3 && WindupEndsAt > Now
   && WindupEndsAt - Now <= 0.25f;
 const float DesiredTickInterval = bDashing ? 0.03f : (bInCombatArea ? 0.1f : 0.4f);
 if (!FMath::IsNearlyEqual(PrimaryActorTick.TickInterval, DesiredTickInterval)) {
  SetActorTickInterval(DesiredTickInterval);
 }
 if (!bInCombatArea) {
  WindupEndsAt = -1.f;
  bComboSecondHitPending = false;
  bComboSecondHitActive = false;
  BossWarningEndTime = -1.f;
  CachedWaypoint = FVector::ZeroVector;
  NextPathRefreshAt = 0.f;
  return;
 }
 if (WindupEndsAt > 0.f && Now >= WindupEndsAt) {
  WindupEndsAt = -1.f;
  BossWarningEndTime = -1.f;
  // Heavy elite crushes demand recovery, leaving a counterattack window.
  NextAttackAt = Now + FMath::Max(0.65f, AttackCooldown) + ((bElite || bBoss) && CurrentAttackMultiplier > 2.f ? 1.0f : 0.f);
  if (bBoss) DrawDebugSphere(GetWorld(), GetActorLocation(), AttackRange * CurrentAttackRangeMultiplier, 16, BossPhase >= 3 ? FColor::Magenta : FColor::Orange, false, 0.18f, 0, 2.f);
  // Dash displacement is handled gradually during its final windup.
  // Attack still resolves after the warning against the hero's live position.
  // Attack resolves against the hero's CURRENT position, so dodging works.
  if (Difference.Size() <= AttackRange * CurrentAttackRangeMultiplier) {
   // Do not damage the hero through solid arena walls or cover.
   FCollisionQueryParams SightParams(SCENE_QUERY_STAT(MoonfallMeleeSight), false);
   SightParams.AddIgnoredActor(this);
   FHitResult SightHit;
   const bool bBlocked = GetWorld()->LineTraceSingleByChannel(
       SightHit, GetActorLocation(), Hero->GetActorLocation(), ECC_Visibility, SightParams)
       && SightHit.GetActor() != Hero;
   if (!bBlocked) {
    if (bBoss) Hero->RewardPerfectDodge();
    Hero->ApplyMoonfallDamage(AttackDamage * FMath::Min(2.3f, CurrentAttackMultiplier));
    OnEnemyAttack();
   }
  }
  if (bComboSecondHitPending) {
   // The follow-up has its own tell and can be dodged or interrupted.
   bComboSecondHitPending = false;
   bComboSecondHitActive = true;
   CurrentWindup = BossPhase >= 3 ? 0.5f : 0.7f;
   CurrentAttackMultiplier = 0.9f;
   CurrentAttackRangeMultiplier = 1.15f;
   WindupEndsAt = Now + CurrentWindup;
   BossWarningEndTime = WindupEndsAt;
  } else {
   bComboSecondHitActive = false;
  }
  return;
 }
 // Keep the enemy committed to a telegraphed strike until it resolves. A player
 // stepping outside the basic range should not silently cancel a wider sweep.
 if (WindupEndsAt > 0.f) {
  if (bBoss && BossSpecialMove == 3 && WindupEndsAt - Now <= 0.25f) {
   // Sweep a bounded step each AI tick. Stop short of the hero, and stop at walls.
   const FVector DashVector = (Hero->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
   const float Remaining = FMath::Max(0.f, Distance - 85.f);
   const float Step = FMath::Min(FMath::Min(960.f * FMath::Max(0.f, DeltaSeconds), Remaining), 240.f);
   if (Step > 0.f) {
    FHitResult DashHit;
    AddActorWorldOffset(DashVector * Step, true, &DashHit);
   }
  }
  // Boss warnings are intentionally cheap: one short-lived debug ring per AI tick,
  // no spawned particles, persistent actors, dynamic materials or extra tick loops.
  const AMoonfallHeroPawn* Viewer = bBoss ? Cast<AMoonfallHeroPawn>(UGameplayStatics::GetPlayerPawn(this, 0)) : nullptr;
  const float WarningScale = IsValid(Viewer) ? Viewer->BossWarningScale : 1.f;
  const bool bBossCrush = bBoss && CurrentAttackMultiplier > 2.f;
  const FColor TellColor = bBossCrush ? (BossPhase >= 3 ? FColor::Magenta : FColor::Purple)
    : (CurrentAttackMultiplier > 2.f ? FColor::Purple
    : (CurrentAttackMultiplier > 1.5f ? FColor::Orange : FColor::Red));
  DrawDebugSphere(GetWorld(), GetActorLocation(), AttackRange * CurrentAttackRangeMultiplier,
    bBoss ? 24 : 16, TellColor, false, 0.12f, 0, bBoss ? 1.f + WarningScale : 1.f);
  // Horizontal radius marker on the ground helps distinguish the dodge zone.
  if (bBoss) DrawDebugCircle(GetWorld(), GetActorLocation() - FVector(0.f,0.f,55.f),
    AttackRange * CurrentAttackRangeMultiplier, 24, TellColor, false, 0.12f, 0, 3.f * WarningScale,
    FVector(1.f,0.f,0.f), FVector(0.f,1.f,0.f), false);
  DrawDebugString(GetWorld(), GetActorLocation() + FVector(0.f,0.f,160.f), BossSpecialMove == 5 ? TEXT("SKYFALL SLAM") : (BossSpecialMove == 4 ? TEXT("SPINNING SWEEP") : (BossSpecialMove == 3 ? TEXT("DASH STRIKE") : (bComboSecondHitActive ? TEXT("COMBO FOLLOW-UP") : (BossSpecialMove == 2 ? TEXT("ECLIPSE SLAM") : (BossSpecialMove == 1 ? TEXT("MOON CLEAVE") : (CurrentAttackMultiplier > 2.f ? (bBoss ? TEXT("BOSS CRUSH") : TEXT("ELITE CRUSH")) : (CurrentAttackMultiplier > 1.5f ? TEXT("HEAVY") : (CurrentAttackRangeMultiplier > 1.f ? TEXT("SWEEP") : TEXT("STRIKE"))))))))), nullptr, FColor::Red, 0.12f, false);
  return;
 }
 if (Distance > AttackRange && Distance < 2200.f) {
  FHitResult Hit;
  FVector Target = Hero->GetActorLocation();
  // Elites approach at an offset, creating a flank instead of simply
  // piling into the same straight-line pursuit as regular enemies.
  if ((bElite || (bBoss && BossPhase >= 2)) && Distance > AttackRange * 2.f && Distance < 900.f) {
   const FVector Side(-Difference.Y, Difference.X, 0.f);
   // Bosses change flank direction between attacks; normal elites keep the original flank.
   const float FlankSign = bBoss && AttackSequence % 2 == 0 ? -1.f : 1.f;
   Target += Side.GetSafeNormal() * (bBoss ? 175.f : 125.f) * FlankSign;
  }
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
  // If collision blocks the cached route, request a fresh navigation path.
  // Rate-limit retries to avoid a synchronous path solve every AI tick.
  if (Hit.bBlockingHit && Now >= NextBlockedRepathAt) {
   NextPathRefreshAt = 0.f;
   NextBlockedRepathAt = Now + 0.5f;
   CachedWaypoint = FVector::ZeroVector;
  }
 } else if (Distance <= AttackRange) {
  const float Now = GetWorld()->GetTimeSeconds();
  if (WindupEndsAt < 0.f && Now >= NextAttackAt) {
   // Alternate quick strikes, heavy attacks and wider sweeps with distinct warnings.
   // An elite cannot chain another attack during its recovery window.
   // Phase-two bosses unlock a fourth, highly telegraphed crush.
   const int32 Pattern = AttackSequence++ % ((bElite || (bBoss && BossPhase >= 2)) ? 4 : 3);
   BossSpecialMove = bBoss && BossPhase >= 2 && AttackSequence % 5 == 0 ? (BossPhase >= 3 ? 2 : 1) : 0;
   if (bBoss && BossPhase >= 2 && AttackSequence % 7 == 0) BossSpecialMove = 3;
   if (bBoss && BossPhase >= 3 && AttackSequence % 9 == 0) BossSpecialMove = 4;
   if (bBoss && BossPhase >= 3 && AttackSequence % 11 == 0) BossSpecialMove = 5;
   // Later-phase bosses alternate fast opener and a readable delayed follow-up.
   const bool bComboFollowUp = bBoss && BossPhase >= 2 && Pattern == 1;
   CurrentWindup = Pattern == 0 ? 0.38f : (Pattern == 1 ? 0.85f : (Pattern == 2 ? 0.65f : 1.05f));
   CurrentAttackMultiplier = Pattern == 1 ? 1.8f : (Pattern == 2 ? 0.75f : (Pattern == 3 ? 2.25f : 1.f));
   CurrentAttackRangeMultiplier = Pattern == 2 ? 1.25f : (Pattern == 3 ? 1.4f : 1.f);
   if (bBoss && BossPhase >= 2) {
    // Phase three alternates swift strikes with pronounced heavy tells.
    if (BossPhase == 3 && Pattern == 0) CurrentWindup *= 0.8f;
    else CurrentWindup *= 0.95f;
    CurrentAttackMultiplier *= BossPhase == 3 ? 1.2f : 1.1f;
   }
   if (BossSpecialMove == 1) {
    // Moon cleave: a broad deliberate sweeping strike.
    CurrentWindup = 1.15f;
    CurrentAttackMultiplier = 1.35f;
    CurrentAttackRangeMultiplier = 1.65f;
   } else if (BossSpecialMove == 2) {
    // Eclipse slam: a slower high-impact strike with pronounced recovery.
    CurrentWindup = 1.4f;
    CurrentAttackMultiplier = 2.5f;
    CurrentAttackRangeMultiplier = 1.3f;
   }
   if (BossSpecialMove == 3) { // Fast dash strike: narrower and readable.
    CurrentWindup = 0.85f; CurrentAttackMultiplier = 1.2f; CurrentAttackRangeMultiplier = 1.45f;
   } else if (BossSpecialMove == 4) { // Spinning sweep.
    CurrentWindup = 1.1f; CurrentAttackMultiplier = 1.0f; CurrentAttackRangeMultiplier = 1.8f;
   } else if (BossSpecialMove == 5) { // Aerial-style slam, grounded hit resolution.
    CurrentWindup = 1.5f; CurrentAttackMultiplier = 2.2f; CurrentAttackRangeMultiplier = 1.35f;
   }
   if (bComboFollowUp && BossSpecialMove == 0) {
    CurrentWindup += 0.25f;
    bComboSecondHitPending = true;
   }
   WindupEndsAt = Now + CurrentWindup;
   if (bBoss) BossWarningEndTime = WindupEndsAt;
  }
 }
}

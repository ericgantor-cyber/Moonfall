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
  CrowLeftWing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CrowLeftWing"));
  CrowRightWing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CrowRightWing"));
  CrowBeak = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CrowBeak"));
  static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
  for (UStaticMeshComponent* Part : {CrowLeftWing,CrowRightWing,CrowBeak}) {
   Part->SetupAttachment(Body);
   Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
   Part->SetCastShadow(false);
   Part->SetHiddenInGame(true);
   if (Cube.Succeeded()) Part->SetStaticMesh(Cube.Object);
  }
  CrowLeftWing->SetRelativeLocation(FVector(0.f,-95.f,0.f));
  CrowRightWing->SetRelativeLocation(FVector(0.f,95.f,0.f));
  CrowLeftWing->SetRelativeScale3D(FVector(0.5f,1.75f,0.065f));
  CrowRightWing->SetRelativeScale3D(FVector(0.5f,1.75f,0.065f));
  CrowBeak->SetRelativeLocation(FVector(100.f,0.f,-20.f));
  CrowBeak->SetRelativeScale3D(FVector(0.55f,0.14f,0.13f));
  PrimaryActorTick.bCanEverTick = true;
  PrimaryActorTick.TickInterval = 0.1f;
}
void AMoonfallEnemy::BeginPlay() {
 Super::BeginPlay();
 // Scale only at spawn to avoid mid-fight HP spikes as the player levels.
 const AMoonfallHeroPawn* Player = Cast<AMoonfallHeroPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
 SpawnBalanceLevel = IsValid(Player) ? FMath::Clamp(Player->Level, 1, 100) : 1;
 const float Levels = static_cast<float>(SpawnBalanceLevel - 1);
 const float HPScale = 1.f + 0.035f * FMath::Min(Levels, 30.f) + 0.018f * FMath::Max(0.f, Levels - 30.f);
 const float DamageScale = 1.f + 0.012f * FMath::Min(Levels, 30.f) + 0.006f * FMath::Max(0.f, Levels - 30.f);
 Health = FMath::Max(1.f, Health * HPScale);
 AttackDamage = FMath::Max(1.f, AttackDamage * DamageScale);
 MaxHealthAtSpawn = FMath::Max(1.f, Health);
 BossMaxHealth = bBoss ? MaxHealthAtSpawn : 0.f;
 if (bCorruptedCrow) {
  CrowHome = GetActorLocation();
  AttackCooldown = FMath::Max(1.7f,AttackCooldown);
  if (Body) Body->SetRelativeScale3D(FVector(0.65f,0.65f,0.5f));
  CrowLeftWing->SetHiddenInGame(false);
  CrowRightWing->SetHiddenInGame(false);
  CrowBeak->SetHiddenInGame(false);
  CrowRecoverUntil = GetWorld() ? GetWorld()->GetTimeSeconds() + 1.0f : 1.f;
  SetActorTickInterval(0.05f);
 }
}

void AMoonfallEnemy::TakeSpellDamage(float Amount) {
  // Ignore invalid/no-op hits so they cannot trigger phase changes, feedback, or loot.
  if (!FMath::IsFinite(Amount) || Amount <= 0.f || Health <= 0.f) return;
  if (MaxHealthAtSpawn <= 0.f) MaxHealthAtSpawn = FMath::Max(1.f, Health);
  if (bBoss) BossMaxHealth = MaxHealthAtSpawn;
  if (Health <= 0.f) return;
  // Elites briefly brace against repeated hits, but the first strike retains
  // full impact and every hit still interrupts their attack windup.
  const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
  const float EffectiveDamage = bElite && Now < HitReactionEndsAt ? Amount * 0.75f : Amount;
  const float HealthBeforeHit = Health;
  Health = FMath::Max(0.0f, Health - FMath::Max(0.0f, EffectiveDamage));
  const float ActualDamage = HealthBeforeHit - Health;
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
  OnHitFeedback(ActualDamage, Health);
  // Hit feedback is now tracked by the HUD rather than persistent debug strings.
  LastHitAmount = ActualDamage;
  LastHitTime = GetWorld() ? GetWorld()->GetTimeSeconds() : -10.f;
  // Bosses absorb ordinary hits without repeated attack interruptions; phase changes still recover safely.
  // Normal enemies retain brief hit reactions to reward offensive pressure.
  if (!bBoss) HitReactionEndsAt = LastHitTime + 0.22f;
  if (Health <= 0.0f) {
   if (AMoonfallHeroPawn* Hero = Cast<AMoonfallHeroPawn>(UGameplayStatics::GetPlayerPawn(this, 0))) { Hero->AwardExperience(FMath::RoundToInt((bBoss ? 125.f : (bElite ? 50.f : 25.f)) * (1.f + 0.018f * FMath::Min(SpawnBalanceLevel - 1, 40)))); Hero->AwardEquipmentDrop(bElite, bBoss); if (bBoss) Hero->CompleteArenaBossQuest(); }
   SetActorHiddenInGame(true); SetActorEnableCollision(false); SetActorTickEnabled(false); SetLifeSpan(1.25f); }
}

void AMoonfallEnemy::Tick(float DeltaSeconds) {
 Super::Tick(DeltaSeconds);
 if (Health <= 0.f || !GetWorld()) return;
 if (bCorruptedCrow) { TickCorruptedCrow(DeltaSeconds); return; }
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
 if (!IsValid(Hero) || Hero->Health <= 0.f || Hero->bOptionsOpen || Hero->bWorldMapOpen) return;
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

void AMoonfallEnemy::UpdateCrowAppearance(float TimeSeconds)
{
 if (!bCorruptedCrow || !Body) return;
 const float WingBeat = FMath::Sin(TimeSeconds*(bCrowDiving ? 11.f : 6.8f) + GetUniqueID()*0.13f);
 if (CrowLeftWing) CrowLeftWing->SetRelativeRotation(FRotator(0.f,0.f,25.f+WingBeat*26.f));
 if (CrowRightWing) CrowRightWing->SetRelativeRotation(FRotator(0.f,0.f,-25.f-WingBeat*26.f));
}
void AMoonfallEnemy::TickCorruptedCrow(float DeltaSeconds)
{
 if (!GetWorld() || Health <= 0.f) return;
 AMoonfallHeroPawn* Hero = Cast<AMoonfallHeroPawn>(UGameplayStatics::GetPlayerPawn(this,0));
 if (!IsValid(Hero) || Hero->Health <= 0.f || Hero->bOptionsOpen || Hero->bWorldMapOpen) return;
 const float Now = GetWorld()->GetTimeSeconds();
 const FVector HeroPosition = Hero->GetActorLocation();
 const FVector Current = GetActorLocation();
 const float PlanarDistance = FVector::Dist2D(Current,HeroPosition);
 // Do not run high-frequency bird animation when outside combat visibility.
 if (PlanarDistance > 2300.f && !bCrowDiving) {
  SetActorTickInterval(0.4f);
  CrowAttackEndsAt = -1.f;
  CrowWarnStartedAt = -1.f;
  return;
 }
 SetActorTickInterval(0.05f);
 UpdateCrowAppearance(Now);
 if (bCrowDiving) {
  const float AttackSpeed = CrowAttackKind == 1 ? 1450.f : 1250.f;
  const FVector DiveTarget = HeroPosition + FVector(0.f,0.f,35.f);
  FHitResult Hit;
  AddActorWorldOffset((DiveTarget-Current).GetClampedToMaxSize(AttackSpeed*DeltaSeconds),true,&Hit);
  const float Reach = FVector::Dist(GetActorLocation(),DiveTarget);
  // A blocking collision with the hero counts as contact, but walls must never deal damage.
  const bool bContact = Hit.bBlockingHit && Hit.GetActor() == Hero;
  if (!bCrowHasHit && (bContact || Reach < 115.f)) {
   FHitResult Sight;
   FCollisionQueryParams Params(SCENE_QUERY_STAT(MoonfallCrowSight),false);
   Params.AddIgnoredActor(this);
   const bool bBlocked = GetWorld()->LineTraceSingleByChannel(
    Sight,GetActorLocation(),HeroPosition,ECC_Visibility,Params) && Sight.GetActor()!=Hero;
   if (!bBlocked) {
    const float DamageFactor = CrowAttackKind == 1 ? 1.15f : 0.8f;
    Hero->ApplyMoonfallDamage(AttackDamage*DamageFactor);
    OnEnemyAttack();
    bCrowHasHit = true;
   }
  }
  if (bContact || Reach < 115.f || Now >= CrowAttackEndsAt || Hit.bBlockingHit) {
   bCrowDiving = false;
   CrowAttackEndsAt = -1.f;
   CrowWarnStartedAt = -1.f;
   CrowRecoverStartAt = Now;
   CrowRecoverUntil = Now+FMath::Max(1.7f,AttackCooldown);
  }
  return;
 }
 if (CrowAttackEndsAt >= 0.f) {
  const float TellDuration = CrowAttackKind == 2 ? 1.0f : CrowAttackKind == 1 ? 0.9f : 0.7f;
  const FVector WarningPosition(HeroPosition.X,HeroPosition.Y,HeroPosition.Z+45.f);
  DrawDebugCircle(GetWorld(),WarningPosition,CrowAttackKind == 1 ? 155.f : 115.f,
                  16,CrowAttackKind == 1 ? FColor::Red : FColor::Orange,
                  false,0.09f,0,2.f,FVector(1.f,0.f,0.f),FVector(0.f,1.f,0.f),false);
  if (Now < CrowAttackEndsAt) {
   DrawDebugString(GetWorld(),Current+FVector(0.f,0.f,100.f),
    CrowAttackKind == 1 ? TEXT("CROW TALON SWEEP - DODGE") :
    CrowAttackKind == 2 ? TEXT("CROW FEINT - WAIT FOR DIVE") : TEXT("CROW DIVE - DODGE"),
    nullptr,FColor::Orange,0.07f,false);
   return;
  }
  // A feint holds position longer, rather than dealing phantom damage during its warning.
  bCrowDiving = true;
  bCrowHasHit = false;
  CrowAttackEndsAt = Now + (CrowAttackKind == 1 ? 0.65f : 0.55f);
  return;
 }
 const FVector HighTarget(HeroPosition.X,HeroPosition.Y,HeroPosition.Z+360.f);
 const float Angle = Now*(CrowRecoverUntil > Now ? 1.25f : 0.8f) + GetUniqueID()*0.17f;
 const float OrbitRadius = CrowRecoverUntil > Now ? 470.f : 360.f;
 const FVector OrbitTarget = HighTarget + FVector(FMath::Cos(Angle)*OrbitRadius,
                                                   FMath::Sin(Angle)*OrbitRadius,85.f);
 const FVector Delta = OrbitTarget - Current;
 AddActorWorldOffset(Delta.GetClampedToMaxSize(420.f*DeltaSeconds),true);
 if (Delta.SizeSquared2D() > 1.f)
  SetActorRotation(FRotator(0.f,Delta.Rotation().Yaw,0.f));
 if (Now >= CrowRecoverUntil && PlanarDistance < 1250.f) {
  // Deterministic variation avoids random repeated attack patterns.
  CrowAttackKind = CrowAttackSequence++ % 3;
  CrowWarnStartedAt = Now;
  CrowAttackEndsAt = Now + (CrowAttackKind == 2 ? 1.0f : CrowAttackKind == 1 ? 0.9f : 0.7f);
 }
}

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
#include "EngineUtils.h"
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
 // Scale only at spawn to avoid mid-fight HP spikes as the player levels.
 const AMoonfallHeroPawn* Player = Cast<AMoonfallHeroPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
 SpawnBalanceLevel = EncounterLevelOverride > 0
   ? FMath::Clamp(EncounterLevelOverride, 1, 100)
   : (IsValid(Player) ? FMath::Clamp(Player->Level, 1, 100) : 1);
 const float Levels = static_cast<float>(SpawnBalanceLevel - 1);
 const float HPScale = 1.f + 0.035f * FMath::Min(Levels, 30.f) + 0.018f * FMath::Max(0.f, Levels - 30.f);
 const float DamageScale = 1.f + 0.012f * FMath::Min(Levels, 30.f) + 0.006f * FMath::Max(0.f, Levels - 30.f);
 // Blueprint tuning must not overflow enemy vitality or damage at spawn.
 const float BaseHealth = FMath::IsFinite(Health) && Health > 0.f ? Health : 1.f;
 const float BaseDamage = FMath::IsFinite(AttackDamage) && AttackDamage > 0.f ? AttackDamage : 1.f;
 Health = FMath::Clamp(BaseHealth, 1.f, 100000.f) * HPScale;
 AttackDamage = FMath::Clamp(BaseDamage, 1.f, 10000.f) * DamageScale;
 MaxHealthAtSpawn = FMath::Max(1.f, Health);
 BossMaxHealth = bBoss ? MaxHealthAtSpawn : 0.f;
 // Bosses and elites require sustained pressure rather than a fixed three-parry break.
 const float PostureMultiplier = bBoss ? 1.8f : (bElite ? 1.25f : 1.f);
 MaxPosture = FMath::IsFinite(MaxPosture) && MaxPosture > 0.f
   ? FMath::Clamp(MaxPosture * PostureMultiplier, 1.f, 10000.f) : 100.f * PostureMultiplier;
 Posture = 0.f;
 BasePostureAtSpawn = MaxPosture;
}

void AMoonfallEnemy::CancelDivineSignature() {
 if (!bDivineSignatureActive) return;
 // Blueprint events are synchronous and may inspect this actor. Publish
 // inactive state first, then notify exactly once with the captured move.
 const FName InterruptedMove = ActiveDivineMoveId;
 bDivineSignatureActive = false;
 ActiveDivineMoveId = NAME_None;
 OnDivineAttackInterrupted(InterruptedMove);
}

void AMoonfallEnemy::ApplyPostureDamage(float Amount) {
 if (Health <= 0.f || !GetWorld() || !FMath::IsFinite(Amount) || Amount <= 0.f
     || !FMath::IsFinite(MaxPosture) || MaxPosture <= 0.f) return;
 const float Now = GetWorld()->GetTimeSeconds();
 // Reject invalid posture timers rather than accidentally breaking a boss for free.
 if (!FMath::IsFinite(Now) || !FMath::IsFinite(PostureBrokenUntil)
     || !FMath::IsFinite(NextPostureBreakAllowedAt)) return;
 if (Now < PostureBrokenUntil || Now < NextPostureBreakAllowedAt) return;
 if (!FMath::IsFinite(Posture)) Posture = 0.f;
 Posture = FMath::Clamp(Posture + Amount, 0.f, MaxPosture);
 PostureRecoveryStartsAt = Now + (FMath::IsFinite(PosturePressureRecoveryDelaySeconds)
   ? FMath::Clamp(PosturePressureRecoveryDelaySeconds, 0.f, 5.f) : 3.f);
 OnPostureChanged(Posture, MaxPosture);
 if (Posture < MaxPosture) return;
 // Break interrupts current attacks and exposes the enemy for a finite window.
 Posture = 0.f;
 const float VulnerabilitySeconds = bBoss ? 1.5f : (bElite ? 2.f : 2.5f);
 PostureBrokenUntil = Now + VulnerabilitySeconds;
 CancelDivineSignature();
 bPostureFinisherAvailable = true;
 PostureBreakCount = FMath::Min(PostureBreakCount + 1, 100000);
 if (bBoss && FMath::IsFinite(BasePostureAtSpawn) && BasePostureAtSpawn > 0.f) {
  const float Growth = FMath::IsFinite(BossPostureGrowthPerBreak)
    ? FMath::Clamp(BossPostureGrowthPerBreak, 0.f, 0.5f) : 0.f;
  const float Cap = FMath::IsFinite(BossPostureGrowthCap)
    ? FMath::Clamp(BossPostureGrowthCap, 1.f, 3.f) : 1.f;
  MaxPosture = FMath::Min(10000.f, BasePostureAtSpawn
    * FMath::Min(Cap, 1.f + Growth * FMath::Min(PostureBreakCount, 100)));
 }
 // Bosses cannot be immediately chained into another posture break after recovery.
 if (bBoss && FMath::IsFinite(BossPostureBreakRecoverySeconds))
  NextPostureBreakAllowedAt = PostureBrokenUntil
    + FMath::Clamp(BossPostureBreakRecoverySeconds, 0.f, 6.f);
 HitReactionEndsAt = FMath::Max(HitReactionEndsAt, PostureBrokenUntil);
 WindupEndsAt = -1.f;
 BossWarningEndTime = -1.f;
 bComboSecondHitPending = false;
 bComboSecondHitActive = false;
 NextAttackAt = FMath::Max(NextAttackAt, PostureBrokenUntil + 0.35f);
 CachedWaypoint = FVector::ZeroVector;
 OnPostureChanged(Posture, MaxPosture);
 OnPostureBroken(VulnerabilitySeconds);
}

void AMoonfallEnemy::RegisterComboMilestone(int32 Stage) {
 if (Stage != 4 && Stage != 8 && Stage != 12) return;
 if (!GetWorld() || !FMath::IsFinite(Health) || Health <= 0.f) return;
 const float Now = GetWorld()->GetTimeSeconds();
 if (!FMath::IsFinite(Now) || !FMath::IsFinite(PostureBrokenUntil)) return;
 // Ordinary enemies flinch briefly. Bosses keep their existing attack
 // and posture-break rules instead of being repeatedly stun-locked.
 if (!bBoss && Now >= PostureBrokenUntil) {
  // Higher chain milestones are more readable, but never prolong an
  // ordinary hit reaction past a quarter-second from the current hit.
  const float FlinchSeconds = Stage == 12 ? 0.24f : (Stage == 8 ? 0.18f : 0.12f);
  const float PreviousReactionEnd = FMath::IsFinite(HitReactionEndsAt) ? HitReactionEndsAt : Now;
  HitReactionEndsAt = FMath::Max(PreviousReactionEnd, Now + FlinchSeconds);
 }
 OnComboMilestone(Stage);
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
  const bool bPostureExposed = Now < PostureBrokenUntil;
  const float ExposureMultiplier = FMath::IsFinite(BrokenPostureDamageMultiplier)
    ? FMath::Clamp(BrokenPostureDamageMultiplier, 1.f, 3.f) : 1.f;
  const bool bShatterFinisher = bPostureExposed && bPostureFinisherAvailable;
  const float FinisherMultiplier = bShatterFinisher && FMath::IsFinite(PostureFinisherDamageMultiplier)
    ? FMath::Clamp(PostureFinisherDamageMultiplier, 1.f, 4.f) : 1.f;
  // A earned shatter finisher bypasses elite repeat-hit bracing, but normal
  // spells still respect it. Avoids undercutting the one-shot payoff.
  const float BracedDamage = bElite && Now < HitReactionEndsAt && !bShatterFinisher
    ? Amount * 0.75f : Amount;
  const float EffectiveDamage = BracedDamage
    * (bShatterFinisher ? FinisherMultiplier : (bPostureExposed ? ExposureMultiplier : 1.f));
  // Extreme finite Blueprint values can overflow once affixes and shatter multiply.
  // Do not consume the once-only shatter opening for invalid damage.
  if (!FMath::IsFinite(EffectiveDamage) || EffectiveDamage <= 0.f) return;
  if (bShatterFinisher) bPostureFinisherAvailable = false;
  const float HealthBeforeHit = Health;
  Health = FMath::Max(0.0f, Health - FMath::Max(0.0f, EffectiveDamage));
  const float ActualDamage = HealthBeforeHit - Health;
  // Wounded elites get a brief breathing-space maneuver, not an escape loop.
  // Bosses, dead enemies and enemies with broken posture never enter retreat.
  if (bElite && !bBoss && ActualDamage > 0.f && Health > 0.f
      && MaxHealthAtSpawn > 0.f && Health / MaxHealthAtSpawn <= 0.3f
      && FMath::IsFinite(Now) && Now >= NextTacticalRetreatAt
      && Now >= PostureBrokenUntil) {
   TacticalRetreatUntil = Now + 2.f;
   TacticalReengageUntil = TacticalRetreatUntil + 1.5f;
   NextTacticalRetreatAt = Now + 8.f;
   bRetreatFlankLeft = !bRetreatFlankLeft;
   NextAttackAt = FMath::Max(NextAttackAt, TacticalRetreatUntil + 0.25f);
   WindupEndsAt = -1.f;
   bComboSecondHitPending = false;
   bComboSecondHitActive = false;
   CachedWaypoint = FVector::ZeroVector;
  }
  if (bShatterFinisher && ActualDamage > 0.f)
    OnShatterFinisher(ActualDamage);
  // Offensive pressure builds posture, but an already broken enemy cannot
  // immediately be rebroken by damage dealt during its vulnerability window.
  if (ActualDamage > 0.f && Health > 0.f && FMath::IsFinite(SpellPostureScale)
      && SpellPostureScale > 0.f && !bPostureExposed)
    ApplyPostureDamage(ActualDamage * FMath::Clamp(SpellPostureScale, 0.f, 2.f));
  if (bBoss && Health > 0.f) {
   const float HealthRatio = Health / FMath::Max(1.f, MaxHealthAtSpawn);
   const int32 NewPhase = HealthRatio <= 0.33f ? 3 : (HealthRatio <= 0.66f ? 2 : 1);
   if (NewPhase > BossPhase) {
    BossPhase = NewPhase;
    CancelDivineSignature();
    WindupEndsAt = -1.f;
    BossWarningEndTime = -1.f;
    // A phase transition cannot truncate an active posture-break lockout.
    NextAttackAt = FMath::Max(NextAttackAt, Now + 1.5f);
    BossPhaseMessageUntil = Now + 1.5f;
    // A divine boss opens each escalated phase with its own signature
    // after the existing 1.5-second breathing window. The scheduler
    // tests the pre-increment attack sequence when deciding ranged casts.
    // Generic bosses keep their normal fresh-cycle opening.
    const bool bDivinePhase = DivineBossId == FName(TEXT("z_void_reaper"))
      || DivineBossId == FName(TEXT("nyra_eclipse_queen"));
    const int32 NewDivineInterval = BossPhase >= 3 ? 2 : (BossPhase == 2 ? 3 : 4);
    AttackSequence = bDivinePhase ? NewDivineInterval - 1 : 0;
    BossSpecialMove = 0;
    bDivineSignatureActive = false;
    bDivineAlternateGeometry = false;
    DivineSignatureCastCount = 0;
    bComboSecondHitPending = false;
    bComboSecondHitActive = false;
    CachedWaypoint = FVector::ZeroVector;
    OnBossPhaseChanged(BossPhase);
    if (bDivinePhase) OnDivinePhaseEntered(DivineBossId, BossPhase, 1.5f);
   }
  }
  OnHitFeedback(ActualDamage, Health);
  // Hit feedback is now tracked by the HUD rather than persistent debug strings.
  LastHitAmount = ActualDamage;
  bLastHitWasShatter = bShatterFinisher && ActualDamage > 0.f;
  LastHitTime = GetWorld() ? GetWorld()->GetTimeSeconds() : -10.f;
  if (ActualDamage > 0.f && FMath::IsFinite(ActualDamage) && GetWorld()) {
   FRecentDamageHit& Entry = DamageHistory[NextDamageHistorySlot];
   Entry.Amount = ActualDamage;
   Entry.Time = LastHitTime;
   Entry.bShatter = bLastHitWasShatter;
   Entry.bKillingBlow = Health <= 0.f;
   NextDamageHistorySlot = (NextDamageHistorySlot + 1) % DamageHistorySize;
  }
  // Bosses absorb ordinary hits without repeated attack interruptions; phase changes still recover safely.
  // Normal enemies retain brief hit reactions to reward offensive pressure.
  if (!bBoss) HitReactionEndsAt = FMath::Max(HitReactionEndsAt, LastHitTime + 0.22f);
  if (Health <= 0.0f) {
   // Defeat cancels pending divine warnings before notifying VFX and death
   // animations. Never leave a danger marker active on a defeated god.
   CancelDivineSignature();
   WindupEndsAt = -1.f;
   BossWarningEndTime = -1.f;
   bComboSecondHitPending = false;
   bComboSecondHitActive = false;
   // Disable combat immediately but retain the mesh for real native defeat
   // choreography. Tick handles only death visuals after Health reaches zero.
   SetActorEnableCollision(false);
   // Wait for the hero's actual animation slot before playing the death
   // reaction; weak queue refs and a finite lifespan prevent stranded corpses.
   DefeatAnimationStartedAt = -100.f;
   DefeatAnimationOrigin = GetActorLocation();
   bDefeatAnimationShatter = bLastHitWasShatter;
   DefeatAnimationStyle = 1;
   SetActorTickEnabled(true);
   SetActorTickInterval(0.f);
   // Set the fallback BEFORE registering: an immediately played finisher
   // shortens it to 1.25 seconds and must not be overridden afterward.
   SetLifeSpan(9.5f);
   OnEnemyDefeated(bLastHitWasShatter);
   if (AMoonfallHeroPawn* Hero = Cast<AMoonfallHeroPawn>(UGameplayStatics::GetPlayerPawn(this, 0))) {
    DefeatAnimationStyle = FMath::Max(1, Hero->RegisterFinisherKill(bLastHitWasShatter, bBoss, GetActorLocation(), this));
    const float RewardScale = 1.f + 0.018f * FMath::Min(SpawnBalanceLevel - 1, 40);
    Hero->AwardExperience(FMath::RoundToInt((bBoss ? 125.f : (bElite ? 50.f : 25.f)) * RewardScale));
    Hero->SpawnGroundEquipmentDrop(bElite, bBoss, GetActorLocation());
   }
   // Visible until the queued finisher plays or the finite fallback expires.
  }
}

void AMoonfallEnemy::BeginFinisherDefeat(int32 Style)
{
 if (Health > 0.f || !GetWorld() || DefeatAnimationStartedAt >= 0.f) return;
 DefeatAnimationStyle = FMath::Clamp(Style, 1, 5);
 DefeatAnimationOrigin = GetActorLocation();
 DefeatAnimationStartedAt = GetWorld()->GetTimeSeconds();
 // Keep the defeated actor only as long as its choreography needs.
 SetLifeSpan(1.25f);
}

void AMoonfallEnemy::TickDefeatAnimation(float DeltaSeconds)
{
 if (!Body || !GetWorld() || DefeatAnimationStartedAt < 0.f) return;
 const float Age = GetWorld()->GetTimeSeconds() - DefeatAnimationStartedAt;
 const float T = FMath::Clamp(Age / 1.15f, 0.f, 1.f);
 const float Ease = T * T * (3.f - 2.f * T);
 const float Direction = DefeatAnimationStyle % 2 == 0 ? -1.f : 1.f;
 const float Spin = DefeatAnimationStyle == 4 ? 900.f : (DefeatAnimationStyle == 3 ? 540.f : 200.f);
 const float Launch = DefeatAnimationStyle == 2 ? 210.f * FMath::Sin(T * PI)
   : DefeatAnimationStyle == 5 ? 100.f * FMath::Sin(T * PI)
   : 85.f * FMath::Sin(T * PI);
 const float Backward = (bDefeatAnimationShatter ? 160.f : 95.f) * Ease;
 const FVector Offset = -GetActorForwardVector().GetSafeNormal2D() * Backward
   + FVector::UpVector * Launch;
 SetActorLocation(DefeatAnimationOrigin + Offset, false);
 Body->SetRelativeRotation(FRotator(Direction * Spin * Ease, 0.f,
   DefeatAnimationStyle == 3 ? Direction * 180.f * Ease : 0.f));
 const float Shrink = FMath::Clamp(1.f - 0.94f * FMath::Pow(T, 2.f), 0.06f, 1.f);
 Body->SetRelativeScale3D(FVector(0.85f, 0.85f, 1.4f) * Shrink);
 // Actor is removed by the 1.25-second lifespan, with collisions already off.
}

void AMoonfallEnemy::Tick(float DeltaSeconds) {
 Super::Tick(DeltaSeconds);
 if (!GetWorld()) return;
 if (Health <= 0.f) {
  TickDefeatAnimation(DeltaSeconds);
  return;
 }
 const float Now = GetWorld()->GetTimeSeconds();
 // Finite, capped simulation time prevents a hung frame from teleporting enemies.
 if (!FMath::IsFinite(Now)) return;
 const float SafeAIDelta = FMath::IsFinite(DeltaSeconds) ? FMath::Clamp(DeltaSeconds, 0.f, 0.1f) : 0.f;
 // Pause hostile behavior during both gameplay menus and after hero defeat.
 // Cancel winding attacks so closing a menu never delivers an instant hit.
 AMoonfallHeroPawn* Hero = Cast<AMoonfallHeroPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
 if (!IsValid(Hero) || !FMath::IsFinite(Hero->Health) || Hero->Health <= 0.f
     || Hero->bOptionsOpen || Hero->bInventoryOpen) {
  CancelDivineSignature();
  WindupEndsAt = -1.f;
  BossWarningEndTime = -1.f;
  bComboSecondHitPending = false;
  bComboSecondHitActive = false;
  LockedAttackDirection = FVector::ZeroVector;
  NextAttackAt = FMath::Max(FMath::IsFinite(NextAttackAt) ? NextAttackAt : Now, Now + 0.35f);
  return;
 }
 // A shatter follow-up expires with its break window. Never carry it into
 // another posture cycle or let an expired flag affect later spells.
 if (bPostureFinisherAvailable && Now >= PostureBrokenUntil)
  bPostureFinisherAvailable = false;
 if (Now >= PostureBrokenUntil && FMath::IsFinite(Posture) && Posture > 0.f
     && FMath::IsFinite(PostureRecoveryStartsAt) && Now >= PostureRecoveryStartsAt
     && FMath::IsFinite(MaxPosture) && MaxPosture > 0.f
     && FMath::IsFinite(PostureRecoveryPerSecond) && PostureRecoveryPerSecond > 0.f) {
  Posture = FMath::Clamp(Posture - FMath::Clamp(PostureRecoveryPerSecond, 0.f, 100.f)
      * (FMath::IsFinite(DeltaSeconds) ? FMath::Clamp(DeltaSeconds, 0.f, 0.4f) : 0.f), 0.f, MaxPosture);
  OnPostureChanged(Posture, MaxPosture);
 }
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
  CancelDivineSignature();
  WindupEndsAt = -1.f;
  BossWarningEndTime = -1.f;
  bComboSecondHitPending = false;
  bComboSecondHitActive = false;
  NextAttackAt = FMath::Max(NextAttackAt, Now + 0.35f);
  return;
 }
 // Reject invalid actor transforms before targeting, pathfinding or attack hit checks.
 const FVector HeroPosition = Hero->GetActorLocation();
 const FVector EnemyPosition = GetActorLocation();
 if (HeroPosition.ContainsNaN() || EnemyPosition.ContainsNaN()) return;
 FVector Difference = HeroPosition - EnemyPosition;
 Difference.Z = 0.f;
 const float Distance = Difference.Size();
 if (!FMath::IsFinite(Distance)) return;
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
  CancelDivineSignature();
  WindupEndsAt = -1.f;
  bComboSecondHitPending = false;
  bComboSecondHitActive = false;
  BossWarningEndTime = -1.f;
  CachedWaypoint = FVector::ZeroVector;
  NextPathRefreshAt = 0.f;
  return;
 }
 // Complete the full two-second elite retreat even after moving outside
 // melee range. Otherwise the pursuit branch would cancel it early.
 if (bElite && !bBoss && Now < TacticalRetreatUntil
     && Now >= HitReactionEndsAt && Now >= PostureBrokenUntil) {
  if (Distance > 1.f) {
   const FVector RetreatDirection = -Difference.GetSafeNormal2D();
   const float RetreatStep = 200.f * SafeAIDelta;
   FHitResult RetreatHit;
   AddActorWorldOffset(RetreatDirection * RetreatStep, true, &RetreatHit);
  }
  return;
 }
 if (WindupEndsAt > 0.f && Now >= WindupEndsAt) {
  WindupEndsAt = -1.f;
  BossWarningEndTime = -1.f;
  // Heavy elite crushes demand recovery, leaving a counterattack window.
  // Preserve longer stagger recovery and cap invalid cooldown tuning.
  const float RecoveryCooldown = FMath::IsFinite(AttackCooldown) ? FMath::Clamp(AttackCooldown, 0.65f, 10.f) : 1.f;
  const float AttackRecoveryAt = Now + RecoveryCooldown + ((bElite || bBoss) && CurrentAttackMultiplier > 2.f ? 1.0f : 0.f);
  NextAttackAt = FMath::Max(FMath::IsFinite(NextAttackAt) ? NextAttackAt : Now, AttackRecoveryAt);
  if (bBoss) DrawDebugSphere(GetWorld(), GetActorLocation(), AttackRange * CurrentAttackRangeMultiplier, 16, BossPhase >= 3 ? FColor::Magenta : FColor::Orange, false, 0.18f, 0, 2.f);
  // Dash displacement is handled gradually during its final windup.
  // Attack still resolves after the warning against the hero's live position.
  // Attack resolves against the hero's CURRENT position, so dodging works.
  // Commit attack direction when windup starts, rather than tracking the player
  // through a full turn. Wide sweeps cover a larger arc; spins remain 360 degrees.
  const FVector ImpactDirection = Difference.GetSafeNormal2D();
  const float FacingDot = FVector::DotProduct(LockedAttackDirection, ImpactDirection);
  const bool bSpinningSweep = bBoss && BossSpecialMove == 4;
  const float MinFacingDot = (BossSpecialMove == 1 || CurrentAttackRangeMultiplier > 1.2f)
    ? -0.35f : 0.f;
  const bool bWithinAttackArc = bDivineSignatureActive || bSpinningSweep || LockedAttackDirection.IsNearlyZero()
    || Difference.IsNearlyZero() || FacingDot >= MinFacingDot;
  // Z strikes the locked mark; Nyra pulses around her position at cast start.
  // Both signatures are avoidable by leaving the marked area before impact.
  const bool bWithinDivineMarker = bDivineSignatureActive
    && FVector::DistSquared2D(HeroPosition, DivineSignatureCenter) <= FMath::Square(DivineSignatureRadius);
  const bool bWithinBasicAttack = !bDivineSignatureActive
    && bWithinAttackArc && Difference.Size() <= AttackRange * CurrentAttackRangeMultiplier;
  // Resolution event is emitted once for either hit or miss, before reset.
  if (bDivineSignatureActive) {
   const FName ResolvedMove = ActiveDivineMoveId;
   const FVector ResolvedCenter = DivineSignatureCenter;
   // A cast counts once at resolution, even when it misses. Staggers,
   // posture breaks, phase transitions and deaths do not consume an
   // alternate phase-three pattern before the cast actually resolves.
   DivineSignatureCastCount = FMath::Min(DivineSignatureCastCount + 1, 100000);
   bDivineSignatureActive = false;
   ActiveDivineMoveId = NAME_None;
   OnDivineAttackResolved(ResolvedMove, ResolvedCenter, bWithinDivineMarker);
  }
  if (bWithinDivineMarker || bWithinBasicAttack) {
   // Do not damage the hero through solid arena walls or cover.
   FCollisionQueryParams SightParams(SCENE_QUERY_STAT(MoonfallMeleeSight), false);
   SightParams.AddIgnoredActor(this);
   FHitResult SightHit;
   const bool bBlocked = GetWorld()->LineTraceSingleByChannel(
       SightHit, GetActorLocation(), Hero->GetActorLocation(), ECC_Visibility, SightParams)
       && SightHit.GetActor() != Hero;
   if (!bBlocked) {
    // Damage must be finite and positive before dodge or parry callbacks fire.
    const float IncomingAttackDamage = AttackDamage * FMath::Min(2.3f, CurrentAttackMultiplier);
    if (!FMath::IsFinite(IncomingAttackDamage) || IncomingAttackDamage <= 0.f) return;
    // One actual timed dodge rewards the hero and pressures only the
    // attacking enemy. Repeated overlapping hits cannot farm posture.
    const bool bPerfectDodge = Hero->TryRewardPerfectDodge();
    if (bPerfectDodge) {
     const float DodgePressure = FMath::IsFinite(PerfectDodgePostureDamage)
       ? FMath::Clamp(PerfectDodgePostureDamage, 0.f, 100.f) : 0.f;
     if (DodgePressure > 0.f) ApplyPostureDamage(DodgePressure);
     // Evading a committed strike interrupts the attacker's combo, creating
     // a real opening without adding more immunity to the hero.
     const float RecoverySeconds = FMath::IsFinite(PerfectDodgeRecoverySeconds)
       ? FMath::Clamp(PerfectDodgeRecoverySeconds, 0.f, 2.f) : 0.f;
     HitReactionEndsAt = FMath::Max(HitReactionEndsAt, Now + RecoverySeconds);
     NextAttackAt = FMath::Max(NextAttackAt, HitReactionEndsAt + 0.35f);
     bComboSecondHitPending = false;
     bComboSecondHitActive = false;
     BossWarningEndTime = -1.f;
     CachedWaypoint = FVector::ZeroVector;
     OnPerfectDodged(DodgePressure);
    }
    const bool bParried = Hero->ApplyMoonfallDamageFrom(
      IncomingAttackDamage, GetActorLocation());
    if (bParried) {
     // A successful parry ends this attack chain and opens counterplay.
     // Bosses recover sooner than ordinary enemies but cannot hit instantly.
     const float ParryStaggerSeconds = bBoss ? 0.45f : 0.75f;
     HitReactionEndsAt = FMath::Max(HitReactionEndsAt, Now + ParryStaggerSeconds);
     ApplyPostureDamage(FMath::Max(0.f, ParryPostureDamage));
     OnPerfectParried(ParryStaggerSeconds);
     // Temporary visualization for editor/development builds until a Blueprint
     // implementation provides the finished impact animation and VFX.
#if ENABLE_DRAW_DEBUG
     DrawDebugSphere(GetWorld(), GetActorLocation() + FVector(0.f, 0.f, 75.f),
       bBoss ? 125.f : 95.f, 16, FColor::Cyan, false, 0.3f, 0, 3.f);
#endif
     NextAttackAt = FMath::Max(NextAttackAt, HitReactionEndsAt + 0.35f);
     bComboSecondHitPending = false;
     bComboSecondHitActive = false;
     BossWarningEndTime = -1.f;
     CachedWaypoint = FVector::ZeroVector;
    } else if (!bPerfectDodge) {
     OnEnemyAttack();
    }
   }
  }
  if (bComboSecondHitPending) {
   // The follow-up has its own tell and can be dodged or interrupted.
   bComboSecondHitPending = false;
   bComboSecondHitActive = true;
   CurrentWindup = BossPhase >= 3 ? 0.5f : 0.7f;
   CurrentAttackMultiplier = 0.9f;
   CurrentAttackRangeMultiplier = 1.15f;
   LockedAttackDirection = Difference.GetSafeNormal2D();
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
   const float Step = FMath::Min(FMath::Min(960.f * SafeAIDelta, Remaining), 240.f);
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
  if (bDivineSignatureActive) {
   DrawDebugCircle(GetWorld(), DivineSignatureCenter - FVector(0.f,0.f,55.f),
     DivineSignatureRadius, 32, DivineBossId == FName(TEXT("z_void_reaper")) ? FColor::Purple : FColor::Cyan,
     false, 0.12f, 0, 3.f * WarningScale, FVector(1.f,0.f,0.f), FVector(0.f,1.f,0.f), false);
   const bool bVoidSignature = DivineBossId == FName(TEXT("z_void_reaper"));
   const TCHAR* DivineWarningText = bVoidSignature
     ? (bDivineAlternateGeometry ? TEXT("VOID COLLAPSE") : TEXT("VOID JUDGMENT"))
     : (bDivineAlternateGeometry ? TEXT("ECLIPSE MARK") : TEXT("ECLIPSE PULSE"));
   DrawDebugString(GetWorld(), DivineSignatureCenter + FVector(0.f,0.f,120.f),
     DivineWarningText, nullptr, FColor::White, 0.12f, false);
  }
  DrawDebugString(GetWorld(), GetActorLocation() + FVector(0.f,0.f,160.f), BossSpecialMove == 5 ? TEXT("SKYFALL SLAM") : (BossSpecialMove == 4 ? TEXT("SPINNING SWEEP") : (BossSpecialMove == 3 ? TEXT("DASH STRIKE") : (bComboSecondHitActive ? TEXT("COMBO FOLLOW-UP") : (BossSpecialMove == 2 ? TEXT("ECLIPSE SLAM") : (BossSpecialMove == 1 ? TEXT("MOON CLEAVE") : (CurrentAttackMultiplier > 2.f ? (bBoss ? TEXT("BOSS CRUSH") : TEXT("ELITE CRUSH")) : (CurrentAttackMultiplier > 1.5f ? TEXT("HEAVY") : (CurrentAttackRangeMultiplier > 1.f ? TEXT("SWEEP") : TEXT("STRIKE"))))))))), nullptr, FColor::Red, 0.12f, false);
  return;
 }
 // Ordinary enemies investigate a recently seen hero rather than tracking
 // the player through solid walls. Boss encounters retain full arena awareness.
 // Visibility traces run at most once per 0.45 seconds per ordinary enemy.
 if (!bBoss && Now >= NextSightCheckAt) {
  const bool bWasVisible = bCanSeeHero;
  NextSightCheckAt = Now + 0.45f + 0.05f * static_cast<float>(GetUniqueID() % 3u);
  FCollisionQueryParams PerceptionParams(SCENE_QUERY_STAT(MoonfallEnemyPerception), false);
  PerceptionParams.AddIgnoredActor(this);
  FHitResult PerceptionHit;
  const bool bSightBlocked = GetWorld()->LineTraceSingleByChannel(
    PerceptionHit, EnemyPosition, HeroPosition, ECC_Visibility, PerceptionParams)
    && PerceptionHit.GetActor() != Hero;
  bCanSeeHero = !bSightBlocked;
  // The old waypoint may lead toward the hero's now-hidden live position.
  // Invalidate it once when visibility changes, not on every sight tick.
  if (bCanSeeHero != bWasVisible) {
   CachedWaypoint = FVector::ZeroVector;
   NextPathRefreshAt = 0.f;
  }
  if (bCanSeeHero) {
   LastSawHeroAt = Now;
   LastKnownHeroPosition = HeroPosition;
  }
 }
 const bool bHasRecentSight = bBoss || (FMath::IsFinite(LastSawHeroAt)
   && Now - LastSawHeroAt <= 2.5f);
 if (!bHasRecentSight) {
  CachedWaypoint = FVector::ZeroVector;
  NextPathRefreshAt = 0.f;
  return;
 }
 // Search terminates when an ordinary enemy actually reaches the last
 // observed location. Do not orbit a vacant position for 2.5 seconds.
 if (!bBoss && !bCanSeeHero
     && FVector::DistSquared2D(EnemyPosition, LastKnownHeroPosition) <= FMath::Square(85.f)) {
  LastSawHeroAt = -100.f;
  CachedWaypoint = FVector::ZeroVector;
  NextPathRefreshAt = 0.f;
  return;
 }
 // Each divine phase compresses the signature cycle without shortening
 // its existing readable warning. Phase 1: fourth attack, phase 2: third,
 // phase 3: every second attack. Shared by range eligibility and selection.
 const int32 DivineSignatureInterval = BossPhase >= 3 ? 2 : (BossPhase == 2 ? 3 : 4);
 // Divine bosses can begin their distinct marked-area signature from a
 // measured spell range. Only the next scheduled signature is eligible:
 // normal blows, other bosses, elites and ordinary mobs remain melee-only.
 // This checks existing state without adding extra AI ticks or nav queries.
 const bool bDivineSignatureDue = bBoss
   && (DivineBossId == FName(TEXT("z_void_reaper"))
       || DivineBossId == FName(TEXT("nyra_eclipse_queen")))
   && AttackSequence % DivineSignatureInterval == DivineSignatureInterval - 1;
 const bool bDivineRangedCastCandidate = bDivineSignatureDue
   && WindupEndsAt < 0.f && Now >= NextAttackAt
   && Distance > AttackRange && Distance <= 750.f;
 // Ranged spells need a clear sightline when the warning begins. Evaluate
 // only ready signatures and cache a result for half a second to avoid
 // extra line traces on every think tick around solid world geometry.
 if (bDivineRangedCastCandidate && Now >= NextDivineCastSightAt) {
  NextDivineCastSightAt = Now + 0.5f;
  FCollisionQueryParams DivineSightParams(SCENE_QUERY_STAT(MoonfallDivineCastSight), false);
  DivineSightParams.AddIgnoredActor(this);
  FHitResult DivineSightHit;
  bDivineCastLineClear = !GetWorld()->LineTraceSingleByChannel(
    DivineSightHit, EnemyPosition, HeroPosition, ECC_Visibility, DivineSightParams)
    || DivineSightHit.GetActor() == Hero;
 }
 const bool bDivineRangedCastReady = bDivineRangedCastCandidate && bDivineCastLineClear;
 if ((Distance > AttackRange || (!bBoss && !bCanSeeHero))
     && Distance < 2200.f && !bDivineRangedCastReady) {
  FHitResult Hit;
  FVector Target = (!bBoss && !bCanSeeHero) ? LastKnownHeroPosition : Hero->GetActorLocation();
  // Give different ordinary enemies stable approach lanes instead of all
  // pursuing the same point. A fixed four-slot split avoids per-tick random
  // jitter, extra neighbor searches, and new navigation queries.
  if ((bBoss || bCanSeeHero) && Distance > AttackRange * 2.f && Distance < 900.f) {
   const FVector Side(-Difference.Y, Difference.X, 0.f);
   float FlankSign = 1.f;
   float FlankDistance = 0.f;
   if (bBoss) {
    // Preserve phase-one direct pursuit and existing phase-two side swaps.
    if (BossPhase >= 2) {
     FlankSign = AttackSequence % 2 == 0 ? -1.f : 1.f;
     FlankDistance = 175.f;
    }
   } else if (bElite) {
    // Switch flank side after a low-health withdrawal.
    FlankSign = bRetreatFlankLeft ? -1.f : 1.f;
    FlankDistance = 125.f;
   } else {
    // Deterministic actor-local lanes: left/right, near/wide. Collision
    // sweeps still protect against walls and occupied corridors.
    const uint32 Lane = GetUniqueID() % 4u;
    FlankSign = Lane % 2u == 0u ? -1.f : 1.f;
    FlankDistance = Lane >= 2u ? 105.f : 70.f;
   }
   Target += Side.GetSafeNormal() * FlankDistance * FlankSign;
  }
  // Solve paths only when the cache expires. Use the live hero position when
  // navigation is unavailable, and never chase an uninitialized waypoint.
  if (UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld())) {
   if (!FMath::IsFinite(NextPathRefreshAt) || Now >= NextPathRefreshAt) {
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
   if (CachedWaypoint.ContainsNaN()) {
    CachedWaypoint = FVector::ZeroVector;
    NextPathRefreshAt = 0.f;
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
  // A brief re-entry chase bonus rewards the two-second withdrawal
  // without increasing damage, attack frequency, or extra navigation queries.
  const bool bTacticalReengage = bElite && !bBoss
    && Now >= TacticalRetreatUntil && Now < TacticalReengageUntil;
  const float SafeChaseSpeed = FMath::Clamp(FMath::IsFinite(ChaseSpeed) ? ChaseSpeed : 0.f, 0.f, 2000.f);
  const float ChaseMultiplier = bTacticalReengage ? 1.2f : 1.f;
  AddActorWorldOffset(Direction.GetSafeNormal() * SafeChaseSpeed * ChaseMultiplier * SafeAIDelta, true, &Hit);
  // Collision can trap packs at tight doorways. First try one short
  // sideways swept step, alternating sides on successive collisions.
  // Rate-limit attempts so wall contact never triggers an extra movement
  // sweep or synchronous path query every AI tick.
  if (Hit.bBlockingHit && Now >= NextObstacleSidestepAt
      && SafeAIDelta > 0.f && !Direction.IsNearlyZero()) {
   NextObstacleSidestepAt = Now + 0.6f;
   const FVector Side(-Direction.Y, Direction.X, 0.f);
   const float SideSign = ((GetUniqueID() + static_cast<uint32>(ObstacleSidestepCount++)) % 2u == 0u) ? -1.f : 1.f;
   const FVector EscapeStep = Side.GetSafeNormal() * SideSign * (120.f * SafeAIDelta);
   FHitResult EscapeHit;
   AddActorWorldOffset(EscapeStep, true, &EscapeHit);
  }
  // A blocked route is invalidated only on its original repath cooldown.
  // The existing single navigation solve then selects a fresh waypoint.
  if (Hit.bBlockingHit && Now >= NextBlockedRepathAt) {
   NextPathRefreshAt = 0.f;
   NextBlockedRepathAt = Now + 0.5f;
   CachedWaypoint = FVector::ZeroVector;
  }
 } else if (Distance <= AttackRange || bDivineRangedCastReady) {
  // Elites and advanced bosses reposition between strikes, not during
  // locked attack windups or posture breaks. A small sideways sweep keeps
  // combat mobile while preserving walls, hit arcs and dodge windows.
  if (!bDivineRangedCastReady
      && (bElite || (bBoss && BossPhase >= 2))
      && WindupEndsAt < 0.f && Now < NextAttackAt
      && Now >= HitReactionEndsAt && Now >= PostureBrokenUntil
      && Distance > AttackRange * 0.45f && Distance > 1.f) {
   const FVector StrafeSide(-Difference.Y, Difference.X, 0.f);
   const float StrafeSign = (AttackSequence % 2 == 0) ? -1.f : 1.f;
   const float StrafeSpeed = bBoss ? 95.f : 70.f;
   const FVector StrafeStep = StrafeSide.GetSafeNormal() * StrafeSign
     * StrafeSpeed * SafeAIDelta;
   if (!StrafeStep.IsNearlyZero()) {
    FHitResult StrafeHit;
    AddActorWorldOffset(StrafeStep, true, &StrafeHit);
   }
  }
  if ((bBoss || bCanSeeHero) && WindupEndsAt < 0.f && Now >= NextAttackAt) {
   // Small-pack coordination: avoid three ordinary enemies delivering their
   // tells at once. Scan only at attack decisions, not on every AI tick;
   // cap work even in massive continuously explored regions.
   if (!bBoss && !bElite) {
    int32 NearbyWindingAllies = 0;
    int32 ScannedActors = 0;
    int32 ConsideredAllies = 0;
    // Cap the iterator itself, not only eligible allies. Large worlds can
    // contain many bosses, defeated actors, and inactive enemies.
    for (TActorIterator<AMoonfallEnemy> It(GetWorld());
         It && ScannedActors < 64 && ConsideredAllies < 32; ++It) {
     ++ScannedActors;
     const AMoonfallEnemy* Other = *It;
     if (!IsValid(Other) || Other == this || Other->Health <= 0.f
         || Other->bBoss || Other->bElite) continue;
     ++ConsideredAllies;
     if (Other->WindupEndsAt > Now
         && FVector::DistSquared2D(Other->GetActorLocation(), GetActorLocation()) < FMath::Square(550.f)) {
      if (++NearbyWindingAllies >= 2) break;
     }
    }
    if (NearbyWindingAllies >= 2) {
     const float SlotDelay = 0.18f + 0.04f * static_cast<float>(GetUniqueID() % 4u);
     NextAttackAt = Now + SlotDelay;
     return;
    }
   }
   // Alternate quick strikes, heavy attacks and wider sweeps with distinct warnings.
   // An elite cannot chain another attack during its recovery window.
   // Phase-two bosses unlock a fourth, highly telegraphed crush.
   const int32 PatternCount = (bElite || (bBoss && BossPhase >= 2)) ? 4 : 3;
   int32 Pattern = AttackSequence++ % PatternCount;
   if (bBoss) {
    // Select uniformly from all other basic patterns. No extra actor,
    // ticker, path solve or unbounded retry loop is needed.
    if (LastBossAttackPattern >= 0 && LastBossAttackPattern < PatternCount) {
     Pattern = FMath::RandRange(0, PatternCount - 2);
     if (Pattern >= LastBossAttackPattern) ++Pattern;
    } else {
     Pattern = FMath::RandRange(0, PatternCount - 1);
    }
    LastBossAttackPattern = Pattern;
   }
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
   // Each phase uses its shared, progressively tighter divine attack cycle.
   // No new actors, timers, animation allocations or pathfinding queries.
   bDivineSignatureActive = false;
   bDivineAlternateGeometry = false;
   const bool bVoidReaper = DivineBossId == FName(TEXT("z_void_reaper"));
   const bool bEclipseQueen = DivineBossId == FName(TEXT("nyra_eclipse_queen"));
   if (bBoss && (bVoidReaper || bEclipseQueen) && AttackSequence % DivineSignatureInterval == 0) {
    bDivineSignatureActive = true;
    BossSpecialMove = 0;
    // Use the number of *resolved* signatures, not attempted windups.
    // First complete cast is the normal mark, the second is the alternate.
    bDivineAlternateGeometry = BossPhase >= 3 && DivineSignatureCastCount % 2 == 1;
    DivineSignatureCenter = bDivineAlternateGeometry
      ? (bVoidReaper ? GetActorLocation() : HeroPosition)
      : (bVoidReaper ? HeroPosition : GetActorLocation());
    DivineSignatureRadius = bDivineAlternateGeometry
      ? (bVoidReaper ? 440.f : 350.f)
      : (bVoidReaper ? 300.f : 410.f);
    CurrentWindup = BossPhase >= 3 ? 1.2f : 1.7f;
    CurrentAttackMultiplier = bVoidReaper ? 1.8f : 1.5f;
    CurrentAttackRangeMultiplier = 1.f;
    bComboSecondHitPending = false;
    ActiveDivineMoveId = bVoidReaper
      ? FName(bDivineAlternateGeometry ? TEXT("VoidCollapse") : TEXT("VoidJudgment"))
      : FName(bDivineAlternateGeometry ? TEXT("EclipseMark") : TEXT("EclipsePulse"));
   }
   if (!bDivineSignatureActive && bComboFollowUp && BossSpecialMove == 0) {
    CurrentWindup += 0.25f;
    bComboSecondHitPending = true;
   }
   LockedAttackDirection = Difference.GetSafeNormal2D();
   WindupEndsAt = Now + CurrentWindup;
   if (bBoss) BossWarningEndTime = WindupEndsAt;
   // Blueprint hooks run synchronously. Publish a fully initialized warning
   // clock before notifying the animation / accessible combat indicator.
   if (bDivineSignatureActive) {
    OnDivineAttackTelegraphed(ActiveDivineMoveId, DivineSignatureCenter, DivineSignatureRadius, CurrentWindup);
   }
  }
 }
}

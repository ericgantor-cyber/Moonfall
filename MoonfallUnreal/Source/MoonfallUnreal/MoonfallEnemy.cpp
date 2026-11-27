#include "MoonfallEnemy.h"
#include "MoonfallEnemyProjectile.h"
#include "MoonfallUnrealGameMode.h"
#include "MoonfallWorldNPC.h"
#include "MoonfallAttackTellVisual.h"
#include "MoonfallEliteTactics.h"
#include "MoonfallPackTactics.h"
#include "MoonfallOmniattack.h"
#include "MoonfallPracticeAI.h"
#include "MoonfallGuardianFootprint.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "MoonfallHeroPawn.h"
#include "MoonfallDungeonLayout.h"
#include "MoonfallRaceCatalog.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "DrawDebugHelpers.h"
#include "CollisionQueryParams.h"
#include "EngineUtils.h"
#include "Misc/GuardValue.h"
AMoonfallEnemy::AMoonfallEnemy() {
  Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
  SetRootComponent(Body);
  static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
  if (Sphere.Succeeded()) Body->SetStaticMesh(Sphere.Object);
  Body->SetCollisionProfileName(TEXT("BlockAll"));
  Body->SetRelativeScale3D(FVector(0.85f,0.85f,1.4f));
  // One non-colliding, render-only attack cue per enemy; preserve the collision root.
  AttackTellMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AttackTellMarker"));
  AttackTellMarker->SetupAttachment(Body);
  static ConstructorHelpers::FObjectFinder<UStaticMesh> TellCube(TEXT("/Engine/BasicShapes/Cube.Cube"));
  if (TellCube.Succeeded()) AttackTellMarker->SetStaticMesh(TellCube.Object);
  AttackTellMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  AttackTellMarker->SetCollisionResponseToAllChannels(ECR_Ignore);
  AttackTellMarker->SetGenerateOverlapEvents(false);
  AttackTellMarker->SetCastShadow(false);
  AttackTellMarker->SetVisibility(false);
  PrimaryActorTick.bCanEverTick = true;
  PrimaryActorTick.TickInterval = 0.1f;
}
int32 AMoonfallEnemy::GetDirectionalWindup(float Now, const FVector& HeroPosition) const {
 if (!bElite || bBoss || bCompanionAttackCommitted || IsActorBeingDestroyed() || !FMath::IsFinite(Health) || Health <= 0.f
     || !FMath::IsFinite(Now) || !FMath::IsFinite(WindupEndsAt) || WindupEndsAt <= Now
     || !FMath::IsFinite(AttackRange) || AttackRange <= 0.f
     || !FMath::IsFinite(CurrentAttackRangeMultiplier) || CurrentAttackRangeMultiplier <= 0.f
     || LockedAttackDirection.ContainsNaN() || HeroPosition.ContainsNaN() || !MoonfallOmniattack::ValidAngle(CommittedSwordAngle)) return -1;
 const FVector Delta = HeroPosition - GetActorLocation();
 const float Range = FMath::Clamp(AttackRange * CurrentAttackRangeMultiplier + 60.f, 0.f, 650.f);
 if (Delta.ContainsNaN() || Delta.SizeSquared() > FMath::Square(Range)) return -1;
 if (!Delta.IsNearlyZero() && FVector::DotProduct(LockedAttackDirection, Delta.GetSafeNormal2D())
      < (CurrentAttackRangeMultiplier > 1.2f ? -0.35f : 0.f)) return -1;
 return CommittedSwordAngle;
}
void AMoonfallEnemy::RefreshAttackTellMarker(float Now, bool bReduceMotion) {
 if (!IsValid(AttackTellMarker)) return;
 const bool bWindup = bEnableProceduralAttackTell && !bBoss
  && FMath::IsFinite(Now) && FMath::IsFinite(Health) && Health > 0.f
  && FMath::IsFinite(WindupEndsAt) && WindupEndsAt > Now
  && FMath::IsFinite(CurrentWindup) && CurrentWindup > 0.f
  && FMath::IsFinite(CurrentAttackMultiplier) && FMath::IsFinite(CurrentAttackRangeMultiplier);
 if (!bWindup) {
  if (AttackTellMarker->IsVisible()) AttackTellMarker->SetVisibility(false);
  return;
 }
 const auto Type = MoonfallAttackTellVisual::Select(
  false, CurrentAttackMultiplier, CurrentAttackRangeMultiplier);
 const auto Profile = MoonfallAttackTellVisual::Profile(Type);
 const float Progress = MoonfallAttackTellVisual::Progress(
  WindupEndsAt - Now, CurrentWindup, bReduceMotion);
 const float Growth = MoonfallAttackTellVisual::Growth(Progress);
 const FVector Scale(Profile.Width * Growth, Profile.Depth * Growth, Profile.Height * Growth);
 const FVector Location(0.f, 0.f, 107.f + (bReduceMotion ? 0.f : 14.f * Progress));
 const FRotator Rotation(0.f, bReduceMotion ? 0.f : Profile.YawDegrees * Progress, 0.f);
 if (!AttackTellMarker->GetRelativeScale3D().Equals(Scale, KINDA_SMALL_NUMBER))
  AttackTellMarker->SetRelativeScale3D(Scale);
 if (!AttackTellMarker->GetRelativeLocation().Equals(Location, KINDA_SMALL_NUMBER))
  AttackTellMarker->SetRelativeLocation(Location);
 if (!AttackTellMarker->GetRelativeRotation().Equals(Rotation, KINDA_SMALL_NUMBER))
  AttackTellMarker->SetRelativeRotation(Rotation);
 if (!AttackTellMarker->IsVisible()) AttackTellMarker->SetVisibility(true);
}
FString AMoonfallEnemy::GetRaceName() const { return FString(MoonfallRaces::Name(RaceId)); }

void AMoonfallEnemy::NotifyRangedProjectileCountered(bool bPerfectDodge)
{
 if(!GetWorld()||IsActorBeingDestroyed()||!FMath::IsFinite(Health)||Health<=0.f)return;
 const float Now=GetWorld()->GetTimeSeconds();
 if(!FMath::IsFinite(Now))return;
 const float Recovery=bPerfectDodge
   ?(FMath::IsFinite(PerfectDodgeRecoverySeconds)?FMath::Clamp(PerfectDodgeRecoverySeconds,0.f,2.f):0.5f)
   :0.75f;
 HitReactionEndsAt=FMath::Max(FMath::IsFinite(HitReactionEndsAt)?HitReactionEndsAt:Now,Now+Recovery);
 NextAttackAt=FMath::Max(FMath::IsFinite(NextAttackAt)?NextAttackAt:Now,HitReactionEndsAt+0.35f);
 WindupEndsAt=-1.f;
 BossWarningEndTime=-1.f;
 bComboSecondHitPending=false;
 bComboSecondHitActive=false;
 const float Pressure=bPerfectDodge
   ?(FMath::IsFinite(PerfectDodgePostureDamage)?FMath::Clamp(PerfectDodgePostureDamage,0.f,100.f):0.f)
   :(FMath::IsFinite(ParryPostureDamage)?FMath::Clamp(ParryPostureDamage,0.f,100.f):0.f);
 if(Pressure>0.f)ApplyPostureDamage(Pressure);
 if(!IsValid(this)||IsActorBeingDestroyed())return;
 if(bPerfectDodge)OnPerfectDodged(Pressure);
 else OnPerfectParried(Recovery);
}

void AMoonfallEnemy::BeginPlay() {
 Super::BeginPlay();
 DungeonHomePosition = GetActorLocation();
 // Native regional bosses and elites receive one static engraved armor batch.
 // Ordinary native enemies receive two race ornaments in the same batch.
 // Never overlay authored Blueprint meshes, divine bosses, or practice sentries.
 const bool bRegionalArt = bBoss && DungeonBossRank>=1 && DungeonBossRank<=5
     && !DungeonBossFamily.IsNone() && ActorHasTag(FName(TEXT("MoonfallDungeonBoss")));
 const bool bEliteArt = bElite && !bBoss && !ActorHasTag(FName(TEXT("MoonfallSanctuaryPractice")));
 const bool bRaceArt = !bBoss && !bElite && !ActorHasTag(FName(TEXT("MoonfallSanctuaryPractice")))
     && RaceId>=0 && RaceId<MoonfallRaces::Count;
 if (GetClass()==AMoonfallEnemy::StaticClass() && DivineBossId.IsNone()
     && !ActorHasTag(FName(TEXT("MoonfallSanctuaryPractice")))
     && (bRegionalArt || bEliteArt || bRaceArt)
     && IsValid(Body) && Body->GetStaticMesh()
     && Body->GetStaticMesh()->GetPathName()==TEXT("/Engine/BasicShapes/Sphere.Sphere")
     && IsValid(AttackTellMarker) && AttackTellMarker->GetStaticMesh()) {
  UInstancedStaticMeshComponent* Crest=NewObject<UInstancedStaticMeshComponent>(this,TEXT("EnemyHeraldryArt"));
  if (IsValid(Crest)) {
   Crest->SetupAttachment(Body);
   Crest->SetStaticMesh(AttackTellMarker->GetStaticMesh());
   Crest->SetCollisionEnabled(ECollisionEnabled::NoCollision);
   Crest->SetCollisionResponseToAllChannels(ECR_Ignore);
   Crest->SetGenerateOverlapEvents(false);
   Crest->SetCastShadow(false);
   Crest->SetCanEverAffectNavigation(false);
   Crest->SetCullDistances(4500,6500);
   // Race-specific masks / pauldrons for primitive native enemies only.
   // Two small pieces per ordinary enemy, using stable existing race IDs.
   if (bRaceArt) {
    struct FRaceOrnament { float Width,Height,Roll; };
    static constexpr FRaceOrnament RaceOrnaments[MoonfallRaces::Count]={
     {.15f,.10f,0.f},   // Human: squared brow
     {.10f,.19f,28.f},  // High Elf: long tapered brow
     {.10f,.14f,48.f},  // Wood Elf: swept leaf shape
     {.21f,.09f,0.f},   // Dwarf: heavy metal plate
     {.12f,.08f,12.f},  // Halfling: rounded profile
     {.20f,.17f,35.f},  // Orc: angled tusk-like armor
     {.09f,.15f,55.f},  // Goblin: narrow angular profile
     {.13f,.22f,20.f},  // Troll: tall cheek guards
     {.17f,.21f,40.f},  // Entkin: bark-like ridges
     {.11f,.16f,60.f},  // Shadowborn: swept mask
     {.16f,.12f,30.f},  // Moonborn: lunar crest
     {.19f,.20f,45.f}   // Draconid: scaled brow
    };
    const auto& Shape=RaceOrnaments[RaceId];
    for (const float Side : {-1.f,1.f}) {
     Crest->AddInstance(FTransform(FRotator(0,0,Side*Shape.Roll),
       FVector(23.f,Side*25.f,30.f),FVector(.10f,Shape.Width,Shape.Height)));
    }
   } else {
   // Armor follows the scaled body without changing the collision root.
   for (const float Side : {-1.f,1.f}) {
    Crest->AddInstance(FTransform(FRotator(0,0,Side*24.f),
      FVector(0,Side*39.f,16.f),FVector(.34f,.23f,.14f)));
    Crest->AddInstance(FTransform(FRotator(0,0,Side*18.f),
      FVector(-9.f,Side*27.f,30.f),FVector(.23f,.12f,.24f)));
   }
   if (bRegionalArt) {
    // Rank-dependent crown silhouette, never a new attack tell or hitbox.
    const int32 Rank=FMath::Clamp(DungeonBossRank,1,5);
    for (int32 I=0;I<Rank;++I) {
     const float Y=(float(I)-float(Rank-1)*.5f)*10.f;
     Crest->AddInstance(FTransform(FRotator(0,45,0),
       FVector(-4.f,Y,45.f),FVector(.09f,.09f,.17f)));
    }
   }
   }
   AddInstanceComponent(Crest);
   Crest->RegisterComponent();
  }
 }
 // Scale only at spawn to avoid mid-fight HP spikes as the player levels.
 AMoonfallHeroPawn* Player = Cast<AMoonfallHeroPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
 CachedHero = Player;
 // Spread one-second possession refreshes across a crowd instead of making
 // every freshly spawned enemy query the player controller on the same frame.
 NextHeroCacheRefreshAt = GetWorld()->GetTimeSeconds() + 1.f
   + 0.05f * static_cast<float>(GetUniqueID() % 7u);
 // Practice sentries stay predictable for every character level, while
 // all ordinary dungeon mobs and bosses retain the existing level scaling.
 const bool bSanctuaryPracticeAtSpawn = ActorHasTag(FName(TEXT("MoonfallSanctuaryPractice")));
 SpawnBalanceLevel = bSanctuaryPracticeAtSpawn ? 1
   : (EncounterLevelOverride > 0 ? FMath::Clamp(EncounterLevelOverride, 1, 100)
   : (IsValid(Player) ? FMath::Clamp(Player->Level, 1, 100) : 1));
 const float Levels = static_cast<float>(SpawnBalanceLevel - 1);
 const float HPScale = 1.f + 0.035f * FMath::Min(Levels, 30.f) + 0.018f * FMath::Max(0.f, Levels - 30.f);
 const float DamageScale = 1.f + 0.012f * FMath::Min(Levels, 30.f) + 0.006f * FMath::Max(0.f, Levels - 30.f);
 // Blueprint tuning must not overflow enemy vitality or damage at spawn.
 const float BaseHealth = FMath::IsFinite(Health) && Health > 0.f ? Health : 1.f;
 const float BaseDamage = FMath::IsFinite(AttackDamage) && AttackDamage > 0.f ? AttackDamage : 1.f;
 // Apply modest race identity to ordinary and elite foes only.
 // Preserve unique Z/Nyra and other authored boss tuning exactly.
 const float RaceHealth = (bBoss || bSanctuaryPracticeAtSpawn) ? 1.f : MoonfallRaces::HealthScale(RaceId);
 const float RaceDamage = (bBoss || bSanctuaryPracticeAtSpawn) ? 1.f : MoonfallRaces::DamageScale(RaceId);
 Health = FMath::Clamp(BaseHealth, 1.f, 100000.f) * HPScale * RaceHealth;
 AttackDamage = FMath::Clamp(BaseDamage, 1.f, 10000.f) * DamageScale * RaceDamage;
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
 if (bResolvingPosture || !FMath::IsFinite(Health) || IsActorBeingDestroyed()) return;
 TGuardValue<bool> PostureGuard(bResolvingPosture, true);
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
 if (IsActorBeingDestroyed() || !FMath::IsFinite(Health) || Health <= 0.f) return;
 if (!FMath::IsFinite(Posture) || !FMath::IsFinite(MaxPosture) || MaxPosture <= 0.f) return;
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
 if (!FMath::IsFinite(HitReactionEndsAt)) HitReactionEndsAt = Now;
 if (!FMath::IsFinite(NextAttackAt)) NextAttackAt = Now;
 HitReactionEndsAt = FMath::Max(HitReactionEndsAt, PostureBrokenUntil);
 bCompanionAttackCommitted = false;
 LockedCompanionTarget.Reset();
 CommittedSwordAngle = -1;
 LockedAttackDirection = FVector::ZeroVector;
 WindupEndsAt = -1.f;
 BossWarningEndTime = -1.f;
 // A posture break cancels the warning instantly, without waiting for Tick.
 if (IsValid(AttackTellMarker) && AttackTellMarker->IsVisible())
  AttackTellMarker->SetVisibility(false);
 bComboSecondHitPending = false;
 bComboSecondHitActive = false;
 NextAttackAt = FMath::Max(NextAttackAt, PostureBrokenUntil + 0.35f);
 bHasCachedNavigationWaypoint = false;
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

void AMoonfallEnemy::RememberCompanionAttacker(AMoonfallWorldNPC* Scout,float Now){
 if(!HasAuthority()||!IsValid(Scout)||Scout->GetWorld()!=GetWorld()||bBoss||bElite||Health<=0.f
    ||ActorHasTag(FName(TEXT("MoonfallSanctuaryPractice")))||!FMath::IsFinite(Now))return;
 RecentCompanionAttacker=Scout;RecentCompanionAttackerAt=Now;
}
bool AMoonfallEnemy::HasRecentCompanionAggression(const AMoonfallWorldNPC* Scout,float Now) const{
 return IsValid(Scout)&&RecentCompanionAttacker.Get()==Scout&&FMath::IsFinite(Now)
    &&Now>=RecentCompanionAttackerAt&&Now-RecentCompanionAttackerAt<=8.f;
}
bool AMoonfallEnemy::TryCompanionSupportStrike(float Amount){
 // The shared TakeSpellDamage path owns the guard. Acquiring it here
 // would suppress the companion's own legitimate melee damage.
 if (bResolvingDamage) return false;
 if(!HasAuthority()||!GetWorld()||IsActorBeingDestroyed()||bBoss||bElite
     ||!FMath::IsFinite(Amount)||Amount<=0.f||Amount>10.f
     ||!FMath::IsFinite(Health)||Health<=Amount||bPostureFinisherAvailable)return false;
 const float Now=GetWorld()->GetTimeSeconds();
 if(!FMath::IsFinite(Now)||!FMath::IsFinite(PostureBrokenUntil)||Now<PostureBrokenUntil)return false;
 const float Before=Health;
 TakeMeleeDamage(Amount);
 return Health<Before;
}
void AMoonfallEnemy::TakeMeleeDamage(float Amount) {
 // Keep a single authoritative damage/defeat path while UE5 melee gains its
 // own hit detection, stamina cost and directional animation hook.
 TakeSpellDamage(Amount);
}

void AMoonfallEnemy::TakeSpellDamage(float Amount) {
  if (bResolvingDamage) return;
  TGuardValue<bool> DamageGuard(bResolvingDamage, true);
  // Ignore invalid/no-op hits so they cannot trigger phase changes, feedback, or loot.
  if (!FMath::IsFinite(Amount) || Amount <= 0.f || Health <= 0.f) return;
  if (!FMath::IsFinite(Health) || !GetWorld() || IsActorBeingDestroyed()) return;
  if (!FMath::IsFinite(GetWorld()->GetTimeSeconds())) return;
  if (!FMath::IsFinite(MaxHealthAtSpawn)) MaxHealthAtSpawn = FMath::Max(1.f, Health);
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
  const float HealthBeforeHit = Health;
  Health = FMath::Max(0.0f, Health - FMath::Max(0.0f, EffectiveDamage));
  const float ActualDamage = HealthBeforeHit - Health;
  // Sub-precision hits must not consume a finisher or trigger hit reactions.
  if (ActualDamage <= 0.f) return;
  if (bShatterFinisher) bPostureFinisherAvailable = false;
  // A struck ordinary enemy investigates its attacker briefly. This does not
  // reveal the hero forever or share omniscient positions with other enemies.
  if (!bBoss && ActualDamage > 0.f && GetWorld()) {
   const AMoonfallHeroPawn* Attacker = Cast<AMoonfallHeroPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
   if (IsValid(Attacker) && Attacker->Health > 0.f
       && !Attacker->GetActorLocation().ContainsNaN()
       && FVector::DistSquared2D(GetActorLocation(), Attacker->GetActorLocation()) < FMath::Square(1800.f)) {
    LastSawHeroAt = Now;
    LastKnownHeroPosition = Attacker->GetActorLocation();
    bHasCachedNavigationWaypoint = false;
    CachedWaypoint = FVector::ZeroVector;
    NextPathRefreshAt = 0.f;
    // A distant mob may be ticking only once a second. A nearby hit should
    // wake its AI for responsive combat without an extra pathfinding scan.
    if (PrimaryActorTick.TickInterval > 0.1f)
     SetActorTickInterval(0.1f);
   }
  }
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
   // Wounded elite retreats cancel the pending warning immediately.
   if (IsValid(AttackTellMarker) && AttackTellMarker->IsVisible())
    AttackTellMarker->SetVisibility(false);
   bComboSecondHitPending = false;
   bComboSecondHitActive = false;
   bHasCachedNavigationWaypoint = false;
   CachedWaypoint = FVector::ZeroVector;
  }
  if (bShatterFinisher && ActualDamage > 0.f)
    OnShatterFinisher(ActualDamage);
  if (!IsValid(this) || IsActorBeingDestroyed() || !GetWorld() || !FMath::IsFinite(Health)) return;
  // Offensive pressure builds posture, but an already broken enemy cannot
  // immediately be rebroken by damage dealt during its vulnerability window.
  if (ActualDamage > 0.f && Health > 0.f && FMath::IsFinite(SpellPostureScale)
      && SpellPostureScale > 0.f && !bPostureExposed)
    ApplyPostureDamage(ActualDamage * FMath::Clamp(SpellPostureScale, 0.f, 2.f));
  if (!IsValid(this) || IsActorBeingDestroyed() || !GetWorld() || !FMath::IsFinite(Health)) return;
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
    bHasCachedNavigationWaypoint = false;
    CachedWaypoint = FVector::ZeroVector;
    OnBossPhaseChanged(BossPhase);
    if (!IsValid(this) || IsActorBeingDestroyed() || !GetWorld() || !FMath::IsFinite(Health)) return;
    if (bDivinePhase) OnDivinePhaseEntered(DivineBossId, BossPhase, 1.5f);
    if (!IsValid(this) || IsActorBeingDestroyed() || !GetWorld() || !FMath::IsFinite(Health)) return;
   }
  }
  OnHitFeedback(ActualDamage, Health);
  if (!IsValid(this) || IsActorBeingDestroyed() || !GetWorld() || !FMath::IsFinite(Health)) return;
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
  // Immediate ordinary-enemy flinch: cancel live windups before the next
  // throttled AI tick. Bosses retain their normal phase/posture safeguards.
  if (!bBoss && ActualDamage > 0.f && Health > 0.f
      && FMath::IsFinite(Now) && WindupEndsAt > Now) {
   WindupEndsAt = -1.f;
   BossWarningEndTime = -1.f;
   bComboSecondHitPending = false;
   bComboSecondHitActive = false;
   LockedAttackDirection = FVector::ZeroVector;
   NextAttackAt = FMath::Max(
       FMath::IsFinite(NextAttackAt) ? NextAttackAt : Now, Now + 0.35f);
   if (IsValid(AttackTellMarker) && AttackTellMarker->IsVisible())
    AttackTellMarker->SetVisibility(false);
  }
  if (Health <= 0.0f) {
   // Defeat cancels pending divine warnings before notifying VFX and death
   // animations. Never leave a danger marker active on a defeated god.
   CancelDivineSignature();
   WindupEndsAt = -1.f;
   BossWarningEndTime = -1.f;
   bComboSecondHitPending = false;
   bComboSecondHitActive = false;
   // Tick is disabled for queued corpses: hide the canceled warning NOW.
   if (IsValid(AttackTellMarker) && AttackTellMarker->IsVisible())
    AttackTellMarker->SetVisibility(false);
   // Disable combat immediately but retain the mesh for real native defeat
   // choreography. Tick handles only death visuals after Health reaches zero.
   SetActorEnableCollision(false);
   // Wait for the hero's actual animation slot before playing the death
   // reaction; weak queue refs and a finite lifespan prevent stranded corpses.
   DefeatAnimationStartedAt = -100.f;
   DefeatAnimationOrigin = GetActorLocation();
   bDefeatAnimationShatter = bLastHitWasShatter;
   DefeatAnimationStyle = 1;
   // Queued corpses have no animation to update yet. A zero-interval tick
   // would otherwise run every frame for up to 9.5 seconds per corpse.
   // Lifespan timers and finisher callbacks work without actor Tick.
   SetActorTickEnabled(false);
   // Set the fallback BEFORE registering: an immediately played finisher
   // shortens it to 1.25 seconds and must not be overridden afterward.
   SetLifeSpan(9.5f);
   // This is the actual killing blow, not room entry or a corpse expiration.
   // Only enemies tagged by our own dungeon layout may affect its boss ledger.
   if (bBoss && Tags.Contains(FName(TEXT("MoonfallDungeonBoss")))) {
    if (AMoonfallHeroPawn* LivingHero = Cast<AMoonfallHeroPawn>(
        UGameplayStatics::GetPlayerPawn(this, 0))) {
     LivingHero->MarkDungeonBossDefeated(this);
    }
   }
   if (!IsValid(this) || IsActorBeingDestroyed() || !GetWorld()) return;
   OnEnemyDefeated(bLastHitWasShatter);
   if (!IsValid(this) || IsActorBeingDestroyed() || !GetWorld()) return;
   if (AMoonfallHeroPawn* Hero = Cast<AMoonfallHeroPawn>(UGameplayStatics::GetPlayerPawn(this, 0))) {
    if (!IsValid(Hero) || Hero->IsActorBeingDestroyed()) return;
    DefeatAnimationStyle = FMath::Max(1, Hero->RegisterFinisherKill(bLastHitWasShatter, bBoss, GetActorLocation(), this));
    if (!IsValid(this) || IsActorBeingDestroyed() || !GetWorld()
        || !IsValid(Hero) || Hero->IsActorBeingDestroyed()) return;
    const bool bSanctuaryPracticeReward = ActorHasTag(FName(TEXT("MoonfallSanctuaryPractice")));
    if (bSanctuaryPracticeReward) {
     // Practice grants a small one-time-kill reward per actor, not a full
     // regular-monster equipment drop on every level reload.
     Hero->AwardExperience(5);
    } else {
     const float RewardScale = 1.f + 0.018f * FMath::Min(SpawnBalanceLevel - 1, 40);
     Hero->AwardExperience(FMath::RoundToInt((bBoss ? 125.f : (bElite ? 50.f : 25.f)) * RewardScale));
     if (!IsValid(this) || IsActorBeingDestroyed() || !GetWorld()
         || !IsValid(Hero) || Hero->IsActorBeingDestroyed()) return;
     Hero->SpawnGroundEquipmentDrop(bElite, bBoss, GetActorLocation());
    }
   }
   // Visible until the queued finisher plays or the finite fallback expires.
  }
}

void AMoonfallEnemy::BeginFinisherDefeat(int32 Style)
{
 if (Health > 0.f || !GetWorld() || DefeatAnimationStartedAt >= 0.f) return;
 DefeatAnimationStyle = FMath::Clamp(Style, 1, 5);
 const float Now = GetWorld()->GetTimeSeconds();
 if (!FMath::IsFinite(Now)) return;
 DefeatAnimationOrigin = GetActorLocation();
 DefeatAnimationStartedAt = Now;
 // Only active defeat choreography needs every-frame mesh updates.
 SetActorTickInterval(0.f);
 SetActorTickEnabled(true);
 // Keep the defeated actor only as long as its choreography needs.
 SetLifeSpan(1.25f);
}

void AMoonfallEnemy::TickDefeatAnimation(float DeltaSeconds)
{
 if (!Body || !GetWorld() || !FMath::IsFinite(DefeatAnimationStartedAt)
     || DefeatAnimationStartedAt < 0.f) return;
 const float WorldTime = GetWorld()->GetTimeSeconds();
 if (!FMath::IsFinite(WorldTime)) return;
 const float Age = WorldTime - DefeatAnimationStartedAt;
 if (!FMath::IsFinite(Age)) return;
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
 if (!FMath::IsFinite(Health) || IsActorBeingDestroyed()) {
  if (IsValid(AttackTellMarker) && AttackTellMarker->IsVisible()) AttackTellMarker->SetVisibility(false);
  return;
 }
 if (Health <= 0.f) {
  if (IsValid(AttackTellMarker) && AttackTellMarker->IsVisible())
   AttackTellMarker->SetVisibility(false);
  TickDefeatAnimation(DeltaSeconds);
  return;
 }
 if (!bBoss && ActorHasTag(FName(TEXT("MoonfallBossMinion")))) {
  const AMoonfallEnemy* Parent=EncounterBoss.Get();
  if (!IsValid(Parent)||Parent->IsActorBeingDestroyed()||!Parent->bBoss
      ||Parent->GetWorld()!=GetWorld()||Parent->GetOwner()!=GetOwner()
      ||Parent->DungeonChamberIndex!=DungeonChamberIndex
      ||!FMath::IsFinite(Parent->Health)||Parent->Health<=0.f) {
   Destroy(); // No reward, attack or victory callback for encounter cleanup.
   return;
  }
 }
 const float Now = GetWorld()->GetTimeSeconds();
 // Finite, capped simulation time prevents a hung frame from teleporting enemies.
 if (!FMath::IsFinite(Now)) return;
 const float SafeAIDelta = FMath::IsFinite(DeltaSeconds) ? FMath::Clamp(DeltaSeconds, 0.f, 0.1f) : 0.f;
 // The hostile world stays live while any player menu is open: talents,
 // inventory, spellbook, atlas, options and NPC dialogue are NOT safe zones.
 // Only a missing, unpossessed or defeated hero cancels pending enemy attacks.
 // The hit HUD warns during menus, and lethal damage closes the talent web.
 AMoonfallHeroPawn* Hero = CachedHero.Get();
 // An unpossessed old pawn may remain valid after respawn or hero switching.
 // Never allow it to receive another cached-target attack.
 if (!IsValid(Hero) || !Hero->IsPlayerControlled()) {
  // Reacquire immediately when the previous hero is destroyed or unpossessed.
  Hero = Cast<AMoonfallHeroPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
  CachedHero = Hero;
  NextHeroCacheRefreshAt = Now + 1.f
    + 0.05f * static_cast<float>(GetUniqueID() % 7u);
 } else if (Now >= NextHeroCacheRefreshAt) {
  // The previous pawn may still exist after possession changes. Refresh at
  // most once per second instead of targeting a stale but valid hero forever.
  Hero = Cast<AMoonfallHeroPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
  CachedHero = Hero;
  NextHeroCacheRefreshAt = Now + 1.f
    + 0.05f * static_cast<float>(GetUniqueID() % 7u);
 }
 if (!IsValid(Hero) || !Hero->IsPlayerControlled()
     || !FMath::IsFinite(Hero->Health) || Hero->Health <= 0.f) {
  CancelDivineSignature();
  if (IsValid(AttackTellMarker) && AttackTellMarker->IsVisible())
   AttackTellMarker->SetVisibility(false);
  WindupEndsAt = -1.f;
  bCompanionAttackCommitted=false;LockedCompanionTarget.Reset();
  BossWarningEndTime = -1.f;
  bComboSecondHitPending = false;
  bComboSecondHitActive = false;
  LockedAttackDirection = FVector::ZeroVector;
  NextAttackAt = FMath::Max(FMath::IsFinite(NextAttackAt) ? NextAttackAt : Now, Now + 0.35f);
  return;
 }
 // A corrupt nonfinite windup must never trap an enemy in a permanent tell.
 // Cancel the full chain without dealing damage, then apply a brief recovery.
 if (!FMath::IsFinite(WindupEndsAt)) {
  CancelDivineSignature();
  bCompanionAttackCommitted = false;
  LockedCompanionTarget.Reset();
  CommittedSwordAngle = -1;
  WindupEndsAt = -1.f;
  BossWarningEndTime = -1.f;
  bComboSecondHitPending = false;
  bComboSecondHitActive = false;
  LockedAttackDirection = FVector::ZeroVector;
  bHasCachedNavigationWaypoint = false;
  CachedWaypoint = FVector::ZeroVector;
  NextPathRefreshAt = 0.f;
  NextAttackAt = FMath::Max(FMath::IsFinite(NextAttackAt) ? NextAttackAt : Now, Now + 0.35f);
 }
 // Visual-only cue: existing 0.1s AI tick, no new path query or damage.
 RefreshAttackTellMarker(Now, Hero->bReduceMotion);
 // A shatter follow-up expires with its break window. Never carry it into
 // another posture cycle or let an expired flag affect later spells.
 if (bPostureFinisherAvailable && Now >= PostureBrokenUntil)
  bPostureFinisherAvailable = false;
 if (Now >= PostureBrokenUntil && FMath::IsFinite(Posture) && Posture > 0.f
     && FMath::IsFinite(PostureRecoveryStartsAt) && Now >= PostureRecoveryStartsAt
     && FMath::IsFinite(MaxPosture) && MaxPosture > 0.f
     && FMath::IsFinite(PostureRecoveryPerSecond) && PostureRecoveryPerSecond > 0.f) {
  const float PreviousPosture = Posture;
  Posture = FMath::Clamp(Posture - FMath::Clamp(PostureRecoveryPerSecond, 0.f, 100.f)
      * (FMath::IsFinite(DeltaSeconds) ? FMath::Clamp(DeltaSeconds, 0.f, 0.4f) : 0.f), 0.f, MaxPosture);
  if (Posture != PreviousPosture) OnPostureChanged(Posture, MaxPosture);
  if (IsActorBeingDestroyed() || !FMath::IsFinite(Health) || Health <= 0.f) return;
  if (!IsValid(Hero) || !Hero->IsPlayerControlled() || !FMath::IsFinite(Hero->Health) || Hero->Health <= 0.f) return;
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
  const FVector DesiredScale(0.85f * Scale / FMath::Max(0.75f,Squash),
    0.85f * Scale / FMath::Max(0.75f,Squash), 1.4f * Squash / Scale);
  // In large worlds, idle enemies otherwise send the same mesh transform
  // every AI tick. Preserve all attack tells and hit reactions.
  if (!Body->GetRelativeScale3D().Equals(DesiredScale, KINDA_SMALL_NUMBER))
   Body->SetRelativeScale3D(DesiredScale);
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
  bCompanionAttackCommitted=false;LockedCompanionTarget.Reset();
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
 // Sanctuary practice sentries never pursue players through the safe village.
 // Only deliberate close-range approaches activate their forgiving attacks.
 static const FName PracticeSimulationTag(TEXT("MoonfallSanctuaryPractice"));
 static const FName RangedRoleTag(TEXT("MoonfallEnemyRole_Ranged"));
 static const FName CasterRoleTag(TEXT("MoonfallEnemyRole_Caster"));
 static const FName BossMinionTag(TEXT("MoonfallBossMinion"));
 static const FName BossEncounterTag(TEXT("MoonfallBossEncounter"));
 const bool bSanctuaryPractice = ActorHasTag(PracticeSimulationTag);
 const bool bCasterCombatRole = !bBoss && !bSanctuaryPractice && ActorHasTag(CasterRoleTag);
 const bool bRangedCombatRole = !bBoss && !bSanctuaryPractice && (ActorHasTag(RangedRoleTag)||bCasterCombatRole);
 const bool bBossMinion = !bBoss && ActorHasTag(BossMinionTag);
 const float EffectiveAttackRange = bRangedCombatRole
   ? MoonfallPackTactics::RangedAttackRange : AttackRange;
 const int32 RangedPositionMode = bRangedCombatRole
   ? MoonfallPackTactics::RangedRepositionMode(Distance) : 0;
 if (bSanctuaryPractice && Distance > 450.f) {
  // A trainee may freely retreat: no lingering aim, tells or instant strike.
  WindupEndsAt = -1.f;
  BossWarningEndTime = -1.f;
  bComboSecondHitPending = false;
  bComboSecondHitActive = false;
  LockedAttackDirection = FVector::ZeroVector;
  bCanSeeHero = false;
  LastSawHeroAt = -100.f;
  InvestigationEndsAt = -1.f;
  SearchTravelEndsAt = -1.f;
  if (IsValid(AttackTellMarker) && AttackTellMarker->IsVisible())
   AttackTellMarker->SetVisibility(false);
  NextAttackAt = FMath::Max(
      FMath::IsFinite(NextAttackAt) ? NextAttackAt : Now,
      Now + MoonfallPracticeAI::ReapproachDelay);
  if (!FMath::IsNearlyEqual(PrimaryActorTick.TickInterval, MoonfallPracticeAI::IdleTickSeconds))
   SetActorTickInterval(MoonfallPracticeAI::IdleTickSeconds);
  bHasCachedNavigationWaypoint = false;
  CachedWaypoint = FVector::ZeroVector;
  NextPathRefreshAt = 0.f;
  return;
 }
 // Far-away enemies use a slower think rate. Return to responsive combat
 // polling when the hero approaches, without per-frame work on idle mobs.
 const bool bInCombatArea = Distance < 2200.f;
 const bool bDashing = bBoss && BossSpecialMove == 3 && WindupEndsAt > Now
   && WindupEndsAt - Now <= 0.25f;
 // Dawnwalker/Assassin's Creed/Skyrim-style simulation LOD: retain authored enemies,
 // but wake nearby combat immediately and check distant dungeon mobs less often.
 // No enemy is removed, respawned, or granted free attacks by this optimization.
 // Bosses and elite encounters keep their existing responsiveness.
 static const FName DungeonSimulationTag(TEXT("MoonfallDungeonEncounter"));
 const bool bDungeonSimulationMember = ActorHasTag(DungeonSimulationTag);
 const bool bDistantDungeonMob = !bBoss && !bElite && !bInCombatArea
   && bDungeonSimulationMember;
 const float DesiredTickInterval = bDashing ? 0.03f
   : (bInCombatArea ? 0.1f
      : (bDistantDungeonMob ? (Distance >= 10000.f ? 2.0f : 1.0f) : 0.4f));
 if (!FMath::IsNearlyEqual(PrimaryActorTick.TickInterval, DesiredTickInterval)) {
  SetActorTickInterval(DesiredTickInterval);
 }
 // Returning guards retain their world presence instead of disappearing.
 // Use swept, bounded movement and no extra perception traces or path queries.
 // The longer out-of-combat tick interval must not slow return travel to a crawl.
 const auto ReturnToDungeonPost = [&]() {
  if (bBoss || bElite
      || !bDungeonSimulationMember
      || DungeonHomePosition.ContainsNaN()) return;
  FVector ToHome = DungeonHomePosition - GetActorLocation();
  ToHome.Z = 0.f;
  // Most distant guards are already at their post. Skip square roots there.
  const double HomeDistanceSq = ToHome.SizeSquared();
  if (!FMath::IsFinite(HomeDistanceSq) || HomeDistanceSq <= FMath::Square(85.0)) return;
  const float HomeDistance = static_cast<float>(FMath::Sqrt(HomeDistanceSq));
  if (!FMath::IsFinite(HomeDistance) || HomeDistance <= 85.f) return;
  const float SafeReturnSpeed = FMath::Clamp(
    FMath::IsFinite(ChaseSpeed) ? ChaseSpeed : 0.f, 0.f, 2000.f);
  const float ReturnDelta = FMath::IsFinite(DeltaSeconds)
    ? FMath::Clamp(DeltaSeconds, 0.f, 2.f) : 0.f;
  const float Step = FMath::Min(HomeDistance,
    SafeReturnSpeed * 0.6f * ReturnDelta);
  if (Step > 0.f) {
   const FVector ReturnDirection = ToHome.GetSafeNormal();
   FHitResult ReturnHit;
   AddActorWorldOffset(ReturnDirection * Step, true, &ReturnHit);
   // Try one swept sidestep when a doorframe blocks the direct journey home.
   // Throttle attempts across AI ticks; never teleport through world geometry.
   if (ReturnHit.bBlockingHit && Now >= NextObstacleSidestepAt) {
    NextObstacleSidestepAt = Now + 0.6f;
    // Slide along the actual obstacle surface first. Perpendicular-only
    // avoidance can keep pushing guards into angled dungeon doorframes.
    FVector Side = FVector::VectorPlaneProject(ReturnDirection, ReturnHit.Normal);
    Side.Z = 0.f;
    if (!Side.Normalize()) {
     Side = FVector(-ReturnDirection.Y, ReturnDirection.X, 0.f);
     const float Sign = (ObstacleSidestepCount % 2 == 0) ? 1.f : -1.f;
     Side *= Sign;
    }
    ++ObstacleSidestepCount;
    // A single 0.1-second AI step is too short to clear many doorframes.
    // Reserve a modest swept clearance step at most once per 0.6 seconds.
    const float SidestepDistance = FMath::Min(80.f,
      FMath::Max(Step, SafeReturnSpeed * 0.6f * 0.35f));
    FHitResult SidestepHit;
    AddActorWorldOffset(Side * SidestepDistance, true, &SidestepHit);
   }
  }
 };
 if (!bInCombatArea) {
  // Drop stale aggro once out of range; otherwise guards can stay stranded.
  if (!bBoss && (Now - LastSawHeroAt > 4.f)) {
   LastSawHeroAt = -100.f;
   InvestigationEndsAt = -1.f;
   SearchTravelEndsAt = -1.f;
   bCanSeeHero = false;
   ReturnToDungeonPost();
  }
  // Clear the entire attack commitment before sleeping the distant AI.
  // Only an interrupted attack needs recovery; idle enemies remain ready.
  if (WindupEndsAt > 0.f || bComboSecondHitPending || bComboSecondHitActive)
   NextAttackAt = FMath::Max(FMath::IsFinite(NextAttackAt) ? NextAttackAt : Now, Now + 0.35f);
  CancelDivineSignature();
  bCompanionAttackCommitted = false;
  LockedCompanionTarget.Reset();
  CommittedSwordAngle = -1;
  LockedAttackDirection = FVector::ZeroVector;
  WindupEndsAt = -1.f;
  if (IsValid(AttackTellMarker) && AttackTellMarker->IsVisible())
   AttackTellMarker->SetVisibility(false);
  bComboSecondHitPending = false;
  bComboSecondHitActive = false;
  BossWarningEndTime = -1.f;
  bHasCachedNavigationWaypoint = false;
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
  const int32 ResolvedSwordAngle = CommittedSwordAngle;
  CommittedSwordAngle = -1;
  WindupEndsAt = -1.f;
  BossWarningEndTime = -1.f;
  // Heavy elite crushes demand recovery, leaving a counterattack window.
  // Preserve longer stagger recovery and cap invalid cooldown tuning.
  const float RecoveryCooldown = FMath::IsFinite(AttackCooldown) ? FMath::Clamp(AttackCooldown, 0.65f, 10.f) : 1.f;
  const float AttackRecoveryAt = Now + RecoveryCooldown + ((bElite || bBoss) && CurrentAttackMultiplier > 2.f ? 1.0f : 0.f);
  NextAttackAt = FMath::Max(FMath::IsFinite(NextAttackAt) ? NextAttackAt : Now, AttackRecoveryAt);
  // A committed companion strike either hits that follower or misses.
  // Never redirect it to the hero if the follower moves, is dismissed or falls.
  if(bCompanionAttackCommitted){
   bCompanionAttackCommitted=false;
   auto* Target=LockedCompanionTarget.Get();LockedCompanionTarget.Reset();
   if(auto* Mode=GetWorld()->GetAuthGameMode<AMoonfallUnrealGameMode>())
    Mode->ResolveCompanionCounterAttack(Hero,this,Target,LockedAttackDirection,
         AttackDamage*FMath::Min(2.3f,CurrentAttackMultiplier),AttackRange*CurrentAttackRangeMultiplier);
   bComboSecondHitPending=false;bComboSecondHitActive=false;
   return;
  }
  if(bRangedCombatRole){
   AMoonfallEnemyProjectile* SpawnedProjectile=nullptr;
   FVector ProjectileMuzzle=FVector::ZeroVector;
   // The bolt travels from a real muzzle point to the position locked when
   // the tell began. A later dodge can leave the projectile behind.
   if(IsValid(Hero)&&Hero->IsPlayerControlled()&&FMath::IsFinite(Hero->Health)
      &&Hero->Health>0.f&&bCanSeeHero&&!LockedRangedAimPoint.ContainsNaN()){
    ProjectileMuzzle=GetActorLocation()+GetActorForwardVector()*90.f+FVector(0.f,0.f,65.f);
    const FVector Direction=(LockedRangedAimPoint-ProjectileMuzzle).GetSafeNormal();
    const float DamageScale=MoonfallPackTactics::RangedProjectileDamageScale(bCasterCombatRole);
    const float ProjectileDamage=AttackDamage*FMath::Clamp(CurrentAttackMultiplier,0.1f,2.3f)*DamageScale;
    if(!ProjectileMuzzle.ContainsNaN()&&!Direction.ContainsNaN()&&!Direction.IsNearlyZero()
       &&FMath::IsFinite(ProjectileDamage)&&ProjectileDamage>0.f){
     const FTransform ProjectileTransform(Direction.Rotation(),ProjectileMuzzle);
     SpawnedProjectile=GetWorld()->SpawnActorDeferred<AMoonfallEnemyProjectile>(
       AMoonfallEnemyProjectile::StaticClass(),ProjectileTransform,this,nullptr,
       ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
     if(IsValid(SpawnedProjectile)){
      SpawnedProjectile->Initialize(this,ProjectileDamage,Direction,
        MoonfallPackTactics::RangedProjectileSpeed(bCasterCombatRole),bCasterCombatRole);
      if(IsValid(SpawnedProjectile)&&!SpawnedProjectile->IsActorBeingDestroyed())
       SpawnedProjectile->FinishSpawning(ProjectileTransform);
      if(!IsValid(SpawnedProjectile)||SpawnedProjectile->IsActorBeingDestroyed()) SpawnedProjectile=nullptr;
     }
    }
   }
   bComboSecondHitPending=false;
   bComboSecondHitActive=false;
   if(IsValid(SpawnedProjectile)&&!SpawnedProjectile->IsActorBeingDestroyed())
    OnRangedProjectileFired(ProjectileMuzzle,LockedRangedAimPoint,bCasterCombatRole);
   return;
  }
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
  const bool bWithinGuardianFootprint = ActiveGuardianFootprint >= 0 &&
    MoonfallGuardianFootprint::Hit(
      static_cast<MoonfallGuardianFootprint::Shape>(ActiveGuardianFootprint),
      GuardianOrigin.X, GuardianOrigin.Y, GuardianTarget.X, GuardianTarget.Y,
      LockedAttackDirection.X, LockedAttackDirection.Y,
      HeroPosition.X, HeroPosition.Y, AttackRange * CurrentAttackRangeMultiplier);
  // Preserve the existing directional melee contract for untagged enemies.
  const bool bOrdinaryDirectionalMelee =
    bWithinAttackArc && Distance <= AttackRange * CurrentAttackRangeMultiplier;
  const bool bWithinBasicAttack = !bDivineSignatureActive &&
    (ActiveGuardianFootprint >= 0 ? bWithinGuardianFootprint : bOrdinaryDirectionalMelee);
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
    // Practice uses the same damage/parry pipeline, but stops at one HP.
    const float EffectiveAttackDamage = bSanctuaryPractice
      ? MoonfallPracticeAI::NonLethalDamage(IncomingAttackDamage, Hero->Health)
      : IncomingAttackDamage;
    if (!FMath::IsFinite(EffectiveAttackDamage) || EffectiveAttackDamage <= 0.f) return;
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
     bHasCachedNavigationWaypoint = false;
     CachedWaypoint = FVector::ZeroVector;
     OnPerfectDodged(DodgePressure);
    }
    const bool bDodgeOnlyStrike = bBoss
      && Tags.Contains(FName(TEXT("MoonfallDungeonBoss")))
      && bDungeonBossAttackDodgeOnly;
    const bool bParried = bDodgeOnlyStrike
      ? Hero->ApplyMoonfallDodgeOnlyDamageFrom(EffectiveAttackDamage, GetActorLocation())
      : (bElite && !bBoss && !bSanctuaryPractice && MoonfallOmniattack::ValidAngle(ResolvedSwordAngle)
          ? Hero->ApplyDirectionalSwordDamageFrom(EffectiveAttackDamage, GetActorLocation(), ResolvedSwordAngle)
          : Hero->ApplyMoonfallDamageFrom(EffectiveAttackDamage, GetActorLocation()));
    if (bParried) {
     // A successful parry ends this attack chain and opens counterplay.
     // Bosses recover sooner than ordinary enemies but cannot hit instantly.
     const float ParryStaggerSeconds = bBoss ? 0.45f : 0.75f;
     HitReactionEndsAt = FMath::Max(HitReactionEndsAt, Now + ParryStaggerSeconds);
     ApplyPostureDamage(FMath::Max(0.f, ParryPostureDamage));
     OnPerfectParried(ParryStaggerSeconds);
     // A parried non-boss elite can schedule one ordinary sweep, not a bonus hit.
     // The four-second memory and five-second re-arm cooldown bound the response.
     if (MoonfallEliteTactics::CanArm(bElite, bBoss, bSanctuaryPractice,
           Health > 0.f, Now < PostureBrokenUntil,
           Now >= NextAdaptiveSweepAllowedAt)) {
      AdaptiveSweepReadyUntil = Now + MoonfallEliteTactics::ParryMemorySeconds;
      NextAdaptiveSweepAllowedAt =
        Now + MoonfallEliteTactics::ParryResponseCooldownSeconds;
     }
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
     bHasCachedNavigationWaypoint = false;
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
   if (bBoss && !DungeonBossFamily.IsNone()
       && Tags.Contains(FName(TEXT("MoonfallDungeonBoss")))) {
    DungeonBossAttackLabel = FText::FromString(TEXT("COMBO FOLLOW-UP"));
    DungeonBossAttackWarningDuration = FMath::Clamp(CurrentWindup, 0.1f, 5.f);
    bDungeonBossAttackDodgeOnly = false;
   }
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
  const AMoonfallHeroPawn* Viewer = bBoss ? Hero : nullptr;
  const float WarningScale = IsValid(Viewer) ? Viewer->BossWarningScale : 1.f;
  const bool bBossCrush = bBoss && CurrentAttackMultiplier > 2.f;
  const FColor TellColor = bBossCrush ? (BossPhase >= 3 ? FColor::Magenta : FColor::Purple)
    : (CurrentAttackMultiplier > 2.f ? FColor::Purple
    : (CurrentAttackMultiplier > 1.5f ? FColor::Orange : FColor::Red));
  if (ActiveGuardianFootprint >= 0) {
   const auto Shape = static_cast<MoonfallGuardianFootprint::Shape>(ActiveGuardianFootprint);
   const float Reach = AttackRange * CurrentAttackRangeMultiplier;
   const FVector Origin = GuardianOrigin - FVector(0.f,0.f,55.f);
   const FVector Target = GuardianTarget - FVector(0.f,0.f,55.f);
   const FColor ShapeColor = Shape == MoonfallGuardianFootprint::Shape::VenomPool ? FColor::Green
     : (Shape == MoonfallGuardianFootprint::Shape::EclipseCross ? FColor::Purple : FColor::Orange);
   auto Circle = [&](const FVector& Center,float Radius) {
    DrawDebugCircle(GetWorld(), Center, Radius, 32, ShapeColor, false, 0.12f, 0, 3.f,
      FVector(1.f,0.f,0.f), FVector(0.f,1.f,0.f), false);
   };
   if (Shape == MoonfallGuardianFootprint::Shape::MoonboundRing) {
    Circle(Origin, Reach); Circle(Origin, Reach * 0.34f);
   } else if (Shape == MoonfallGuardianFootprint::Shape::VenomPool) {
    Circle(Target, Reach * 0.46f);
   } else if (Shape == MoonfallGuardianFootprint::Shape::EmberCharge) {
    const FVector Dir = LockedAttackDirection.GetSafeNormal2D();
    const FVector Side(-Dir.Y,Dir.X,0.f);
    const FVector End = Origin + Dir * (Reach*1.25f);
    DrawDebugLine(GetWorld(), Origin + Side*Reach*0.22f, End + Side*Reach*0.22f, ShapeColor, false, 0.12f, 0, 3.f);
    DrawDebugLine(GetWorld(), Origin - Side*Reach*0.22f, End - Side*Reach*0.22f, ShapeColor, false, 0.12f, 0, 3.f);
   } else if (Shape == MoonfallGuardianFootprint::Shape::EclipseCross) {
    DrawDebugLine(GetWorld(), Target + FVector(Reach,0,0), Target - FVector(Reach,0,0), ShapeColor, false, 0.12f, 0, 5.f);
    DrawDebugLine(GetWorld(), Target + FVector(0,Reach,0), Target - FVector(0,Reach,0), ShapeColor, false, 0.12f, 0, 5.f);
   }
   DrawDebugString(GetWorld(), GetActorLocation()+FVector(0.f,0.f,160.f),
      UTF8_TO_TCHAR(MoonfallGuardianFootprint::Name(Shape)), nullptr, FColor::White, 0.12f, false);
   return;
  }
  DrawDebugSphere(GetWorld(), GetActorLocation(), EffectiveAttackRange * CurrentAttackRangeMultiplier,
    bBoss ? 24 : 16, TellColor, false, 0.12f, 0, bBoss ? 1.f + WarningScale : 1.f);
  if(bRangedCombatRole&&!LockedRangedAimPoint.ContainsNaN()){
   const FVector Muzzle=GetActorLocation()+GetActorForwardVector()*90.f+FVector(0.f,0.f,65.f);
   const FColor RangedColor=bCasterCombatRole?FColor::Cyan:FColor::Orange;
   DrawDebugLine(GetWorld(),Muzzle,LockedRangedAimPoint,RangedColor,false,0.12f,0,3.f);
   DrawDebugSphere(GetWorld(),LockedRangedAimPoint,bCasterCombatRole?24.f:15.f,12,
     RangedColor,false,0.12f,0,2.f);
   DrawDebugString(GetWorld(),GetActorLocation()+FVector(0.f,0.f,190.f),
     bCasterCombatRole?TEXT("ARCANE BOLT"):TEXT("PROJECTILE"),nullptr,RangedColor,0.12f,false);
  }
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
 // Keep close-range awareness responsive, while reducing costly
 // visibility traces from distant ordinary enemies in crowded encounters.
 // Elites and dungeon scouts retain their existing faster perception rate.
 if (!bBoss && Now >= NextSightCheckAt) {
  const bool bWasVisible = bCanSeeHero;
  const bool bFastPerception = bElite ||
    (ActorHasTag(FName(TEXT("DungeonScout")))
     && ActorHasTag(FName(TEXT("MoonfallDungeonEncounter"))));
  const float SightInterval = !bFastPerception && Distance > 1200.f ? 0.7f : 0.45f;
  NextSightCheckAt = Now + SightInterval
    + 0.05f * static_cast<float>(GetUniqueID() % 3u);
  FCollisionQueryParams PerceptionParams(SCENE_QUERY_STAT(MoonfallEnemyPerception), false);
  PerceptionParams.AddIgnoredActor(this);
  FHitResult PerceptionHit;
  // Trace between upper-body points rather than floor-level actor origins.
  // This prevents small steps and raised dungeon floors from falsely
  // hiding a visible hero without adding another perception trace.
  const FVector EnemySightPoint = EnemyPosition + FVector(0.f, 0.f, 60.f);
  const FVector HeroSightPoint = HeroPosition + FVector(0.f, 0.f, 60.f);
  const bool bSightBlocked = GetWorld()->LineTraceSingleByChannel(
    PerceptionHit, EnemySightPoint, HeroSightPoint, ECC_Visibility, PerceptionParams)
    && PerceptionHit.GetActor() != Hero;
  bCanSeeHero = !bSightBlocked;
  // The old waypoint may lead toward the hero's now-hidden live position.
  // Invalidate it once when visibility changes, not on every sight tick.
  if (bCanSeeHero != bWasVisible) {
   bHasCachedNavigationWaypoint = false;
   CachedWaypoint = FVector::ZeroVector;
   NextPathRefreshAt = 0.f;
   // Give lost-target searches enough travel time to reach the last sighting.
   if (bWasVisible && !bCanSeeHero) {
    const bool bScout = ActorHasTag(FName(TEXT("DungeonScout")))
      && ActorHasTag(FName(TEXT("MoonfallDungeonEncounter")));
    SearchTravelEndsAt = Now + (bScout ? 11.f : 8.f);
   }
  }
  if (bCanSeeHero) {
   LastSawHeroAt = Now;
   LastKnownHeroPosition = HeroPosition;
   InvestigationEndsAt = -1.f;
   SearchTravelEndsAt = -1.f;
  }
 }
 // Scouts remember only the last visible position, never the hero behind walls.
 // Their extra search window adds no additional line traces or path requests.
 const bool bDungeonScout = ActorHasTag(FName(TEXT("DungeonScout")))
   && ActorHasTag(FName(TEXT("MoonfallDungeonEncounter")));
 const float LastSeenGrace = bDungeonScout ? 4.f : 2.5f;
 const bool bHasRecentSight = bBoss || (FMath::IsFinite(LastSawHeroAt)
   && Now - LastSawHeroAt <= LastSeenGrace)
   || (!bCanSeeHero && SearchTravelEndsAt > Now)
   || (!bCanSeeHero && InvestigationEndsAt > Now);
 if (!bHasRecentSight) {
  SearchTravelEndsAt = -1.f;
  InvestigationEndsAt = -1.f;
  bHasCachedNavigationWaypoint = false;
  CachedWaypoint = FVector::ZeroVector;
  NextPathRefreshAt = 0.f;
  ReturnToDungeonPost();
  return;
 }
 // Investigate at the last seen position, then disengage and return.
 // No extra sight traces or path requests during the investigation.
 if (!bBoss && !bCanSeeHero
     && FVector::DistSquared2D(EnemyPosition, LastKnownHeroPosition) <= FMath::Square(85.f)) {
  if (InvestigationEndsAt < 0.f) {
   SearchTravelEndsAt = -1.f;
   InvestigationEndsAt = Now + (bDungeonScout ? 2.f : 1.2f);
  }
  if (Now >= InvestigationEndsAt) {
   InvestigationEndsAt = -1.f;
   SearchTravelEndsAt = -1.f;
   LastSawHeroAt = -100.f;
   bHasCachedNavigationWaypoint = false;
   CachedWaypoint = FVector::ZeroVector;
   NextPathRefreshAt = 0.f;
   ReturnToDungeonPost();
  }
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
 const bool bRangedNeedsReposition = bRangedCombatRole && bCanSeeHero
   && RangedPositionMode != 0;
 if ((Distance > EffectiveAttackRange || (!bBoss && !bCanSeeHero) || bRangedNeedsReposition)
     && Distance < 2200.f && !bDivineRangedCastReady) {
  FHitResult Hit;
  FVector Target = (!bBoss && !bCanSeeHero) ? LastKnownHeroPosition : Hero->GetActorLocation();
  // Give different ordinary enemies stable approach lanes instead of all
  // pursuing the same point. Eight fixed approach lanes avoid per-tick random
  // jitter, extra neighbor searches, and new navigation queries.
  if (bRangedNeedsReposition) {
   // Ranged enemies hold a readable 390-unit lane. They withdraw when rushed
   // and advance when the hero escapes, using the same cached NavMesh route.
   const FVector TowardHero=Difference.GetSafeNormal2D();
   const FVector Side(-TowardHero.Y,TowardHero.X,0.f);
   Target=HeroPosition-TowardHero*MoonfallPackTactics::RangedPreferredRange
     +Side*MoonfallPackTactics::ApproachLaneSign(GetUniqueID())*55.f;
  } else if ((bBoss || bCanSeeHero) && Distance > EffectiveAttackRange * 2.f && Distance < 900.f) {
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
   } else if (bDungeonScout) {
    // Visible-target-only scout flank. Keep cached path budget and collision
    // sweeping identical to other enemies; never flank an unseen hero.
    FlankSign = GetUniqueID() % 2u == 0u ? -1.f : 1.f;
    FlankDistance = 185.f;
   } else {
    // Deterministic actor-local lanes: left/right, near/wide. Collision
    // sweeps still protect against walls and occupied corridors.
    const uint32 LaneActorId = GetUniqueID();
    FlankSign = MoonfallPackTactics::ApproachLaneSign(LaneActorId);
    FlankDistance = MoonfallPackTactics::ApproachLaneDistance(LaneActorId);
   }
   Target += Side.GetSafeNormal() * FlankDistance * FlankSign;
  }
  // Solve paths only when the cache expires. Use the live hero position when
  // navigation is unavailable, and never chase an uninitialized waypoint.
  if (UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld())) {
   if (!FMath::IsFinite(NextPathRefreshAt) || Now >= NextPathRefreshAt) {
    // Spread pack path queries predictably without reducing close combat
    // responsiveness. Ordinary distant pursuers refresh less frequently;
    // bosses and elites keep their existing half-second navigation budget.
    const float PathRefreshBase = (!bBoss && !bElite && Distance > 1100.f) ? 0.85f : 0.5f;
    NextPathRefreshAt = Now + PathRefreshBase
      + 0.05f * static_cast<float>(GetUniqueID() % 5u);
    FNavLocation Point;
    // Never submit corrupt targets or projected goals to Unreal's path solver.
    // Short-circuit invalid vectors without adding any navigation queries.
    if (!Target.ContainsNaN()
        && Nav->ProjectPointToNavigation(Target, Point)
        && !Point.Location.ContainsNaN()) {
     UNavigationPath* Path = Nav->FindPathToLocationSynchronously(GetWorld(), GetActorLocation(), Point.Location, this);
     // Both complete and partial paths may yield a reachable first corner.
     // If a route has no usable segment, do not move directly through walls.
     // Retry only at the existing cached navigation cadence.
     // Reject corrupt path corners before storing or steering toward them.
     // FVector::ContainsNaN also detects infinite components.
     // A live UObject can still contain an invalid navigation path.
     // Valid partial paths remain usable; only invalid routes fail closed.
     const bool bHasPathSegment = IsValid(Path) && Path->IsValid()
       && Path->PathPoints.Num() > 1;
     const FVector FirstCorner = bHasPathSegment
       ? Path->PathPoints[1] : FVector::ZeroVector;
     bNavigationRouteUnavailable = !bHasPathSegment || FirstCorner.ContainsNaN();
     bHasCachedNavigationWaypoint = !bNavigationRouteUnavailable;
     CachedWaypoint = bHasCachedNavigationWaypoint ? FirstCorner : FVector::ZeroVector;
     if (bHasCachedNavigationWaypoint) {
      // Tight doorways can produce several near-identical starting corners.
      // Skip up to eight already-reached corners from this single cached path,
      // rather than re-solving the same route every AI tick.
      // Use full 3D distance so stacked floors do not look reached.
      const int32 MaxCornersToCheck = FMath::Min(Path->PathPoints.Num(), 9);
      for (int32 CornerIndex = 2; CornerIndex < MaxCornersToCheck
           && FVector::DistSquared(GetActorLocation(), CachedWaypoint) < FMath::Square(65.f);
           ++CornerIndex) {
       const FVector NextCorner = Path->PathPoints[CornerIndex];
       if (NextCorner.ContainsNaN()) {
        // Fail closed if a later corner is corrupt, including infinities.
        bNavigationRouteUnavailable = true;
        bHasCachedNavigationWaypoint = false;
        CachedWaypoint = FVector::ZeroVector;
        break;
       }
       CachedWaypoint = NextCorner;
      }
     }
    } else {
     // The target cannot be projected onto the current navigation mesh.
     bNavigationRouteUnavailable = true;
     bHasCachedNavigationWaypoint = false;
     CachedWaypoint = FVector::ZeroVector;
    }
   }
   if (CachedWaypoint.ContainsNaN()) {
    // A corrupted cached corner must not become a direct wall chase or
    // trigger a synchronous path solve on every subsequent AI tick.
    bHasCachedNavigationWaypoint = false;
    CachedWaypoint = FVector::ZeroVector;
    bNavigationRouteUnavailable = true;
    if (!FMath::IsFinite(NextPathRefreshAt) || NextPathRefreshAt <= Now)
     NextPathRefreshAt = Now + 0.25f;
   }
   if (bHasCachedNavigationWaypoint) {
    // A valid waypoint at world origin must remain navigable.
    // Steering remains in the horizontal plane below, but waypoint arrival
    // must include height so an upper-floor corner is never mistaken for
    // a reached lower-floor corner.
    const FVector ToWaypoint = CachedWaypoint - GetActorLocation();
    // Keep following the reachable corner even on the tick that requests
    // a refresh. Falling back to the distant goal here sends partial-path
    // pursuers straight into walls before the next NavMesh result arrives.
    Target = CachedWaypoint;
    if (ToWaypoint.SizeSquared() < FMath::Square(65.f)) {
     // A partial path can end at this corner with no reachable next point.
     // Do not solve the same path every AI tick while standing here: schedule
     // one prompt retry, but never postpone an already-earlier refresh.
     // This bounds repeated synchronous solves in large enemy crowds.
     if (!FMath::IsFinite(NextPathRefreshAt)
         || NextPathRefreshAt > Now + 0.25f)
      NextPathRefreshAt = Now + 0.25f;
    }
   }
  } else {
   // Preserve direct chase without NavSystem, and discard the old NavMesh
   // cache so its first restored tick requests a fresh path.
   bNavigationRouteUnavailable = false;
   bHasCachedNavigationWaypoint = false;
   CachedWaypoint = FVector::ZeroVector;
   NextPathRefreshAt = 0.f;
  }
  // A failed NavMesh solve must not turn into repeated wall collision sweeps.
  // The next regular cached refresh can recover when the mesh/goal changes.
  if (bNavigationRouteUnavailable) return;
  FVector Direction = Target - GetActorLocation();
  Direction.Z = 0.f;
  if (Direction.ContainsNaN()) return;
  // A brief re-entry chase bonus rewards the two-second withdrawal
  // without increasing damage, attack frequency, or extra navigation queries.
  const bool bTacticalReengage = bElite && !bBoss
    && Now >= TacticalRetreatUntil && Now < TacticalReengageUntil;
  const float SafeChaseSpeed = FMath::Clamp(FMath::IsFinite(ChaseSpeed) ? ChaseSpeed : 0.f, 0.f, 2000.f);
  const float ChaseMultiplier = bTacticalReengage ? 1.2f : 1.f;
  // Never walk past a nearby cached corner in one AI tick. Overshooting
  // short segments makes doorway pursuers jitter between two waypoints.
  // Keep the existing capped movement speed and single swept move.
  const float ChaseStep = FMath::Min(Direction.Size(),
    SafeChaseSpeed * ChaseMultiplier * SafeAIDelta);
  // A stationary enemy at a cached corner needs no collision sweep.
  // Keep its existing path refresh deadline and let the next tick retry.
  if (!FMath::IsFinite(ChaseStep) || ChaseStep <= 0.f) return;
  const FVector ChaseDirection = Direction.GetSafeNormal();
  if (ChaseDirection.ContainsNaN() || ChaseDirection.IsZero()) return;
  AddActorWorldOffset(ChaseDirection * ChaseStep, true, &Hit);
  // Track whether this tick's optional swept escape made it past the wall.
  // A successful escape can keep its cached NavMesh route until the next
  // normal refresh instead of forcing a new synchronous solve.
  bool bSidestepClearedObstacle = false;
  // Collision can trap packs at tight doorways. First try one short
  // sideways swept step, alternating sides on successive collisions.
  // Rate-limit attempts so wall contact never triggers an extra movement
  // sweep or synchronous path query every AI tick.
  if (Hit.bBlockingHit && Now >= NextObstacleSidestepAt
      && SafeAIDelta > 0.f && !Direction.IsNearlyZero()) {
   NextObstacleSidestepAt = Now + 0.6f;
   // Slide along the obstacle face, rather than making tiny perpendicular
   // nudges that can repeatedly fail at angled doorways.
   FVector Side = FVector::VectorPlaneProject(Direction.GetSafeNormal2D(), Hit.Normal);
   Side.Z = 0.f;
   if (!Side.Normalize()) {
    Side = FVector(-Direction.Y, Direction.X, 0.f).GetSafeNormal();
    const float SideSign = ((GetUniqueID() + static_cast<uint32>(ObstacleSidestepCount)) % 2u == 0u) ? -1.f : 1.f;
    Side *= SideSign;
   }
   ++ObstacleSidestepCount;
   // A second blocked slide means the surface route failed. Try its
   // opposite direction on the next cooldown, not on this AI tick.
   if (bReverseCombatWallSlide) Side *= -1.f;
   const float EscapeDistance = FMath::Min(80.f,
     FMath::Max(SafeChaseSpeed * SafeAIDelta, SafeChaseSpeed * 0.2f));
   const FVector EscapeStart = GetActorLocation();
   FHitResult EscapeHit;
   AddActorWorldOffset(Side * EscapeDistance, true, &EscapeHit);
   // A non-blocking sweep is not proof that the actor moved. If world
   // movement was suppressed, keep the cached route eligible for recovery.
   // Ignore sub-centimeter displacement that cannot clear a doorway.
   const bool bEscapeMadeProgress = FVector::DistSquared2D(EscapeStart, GetActorLocation())
     > FMath::Square(1.f);
   bSidestepClearedObstacle = !EscapeHit.bBlockingHit && bEscapeMadeProgress;
   // A failed or motionless escape reverses on the next cooldown.
   bReverseCombatWallSlide = bSidestepClearedObstacle ? false : !bReverseCombatWallSlide;
  } else if (!Hit.bBlockingHit) {
   bReverseCombatWallSlide = false;
  }
  // A blocked movement only invalidates the route when the swept escape
  // did not clear the obstacle. Keep the original 0.5-second cooldown even
  // after a successful slide so dense packs do not spam synchronous solves.
  if (Hit.bBlockingHit && Now >= NextBlockedRepathAt) {
   NextBlockedRepathAt = Now + 0.5f;
   if (!bSidestepClearedObstacle) {
    NextPathRefreshAt = 0.f;
    bHasCachedNavigationWaypoint = false;
    CachedWaypoint = FVector::ZeroVector;
   }
  }
 } else if (Distance <= EffectiveAttackRange || bDivineRangedCastReady) {
  // Elites and advanced bosses reposition between strikes, not during
  // locked attack windups or posture breaks. A small sideways sweep keeps
  // combat mobile while preserving walls, hit arcs and dodge windows.
  if (!bDivineRangedCastReady
      && (bRangedCombatRole || bElite || (bBoss && BossPhase >= 2))
      && WindupEndsAt < 0.f && Now < NextAttackAt
      && Now >= HitReactionEndsAt && Now >= PostureBrokenUntil
      && (bRangedCombatRole || Distance > AttackRange * 0.45f) && Distance > 1.f) {
   const FVector StrafeSide(-Difference.Y, Difference.X, 0.f);
   const float StrafeSign = (AttackSequence % 2 == 0) ? -1.f : 1.f;
   // Bonus-pack leaders can pressure a flank faster without changing
   // boss movement, ordinary elites, attack timing, or the sweep collision.
   const bool bDungeonPackLeader = bElite
     && ActorHasTag(FName(TEXT("MoonfallDungeonBonus")))
     && ActorHasTag(FName(TEXT("DungeonPackLeader")));
   const float StrafeSpeed = bRangedCombatRole ? 45.f : (bBoss ? 95.f : (bDungeonPackLeader ? 100.f : 70.f));
   const FVector StrafeStep = StrafeSide.GetSafeNormal() * StrafeSign
     * StrafeSpeed * SafeAIDelta;
   if (!bSanctuaryPractice && !StrafeStep.IsNearlyZero()) {
    FHitResult StrafeHit;
    AddActorWorldOffset(StrafeStep, true, &StrafeHit);
   }
  }
  if ((bBoss || bCanSeeHero) && WindupEndsAt < 0.f && Now >= NextAttackAt) {
   // Small-pack coordination: yield to active nearby attack tells.
   // Leaders retry sooner, but never bypass another committed attack. Scan only at attack decisions, not on every AI tick;
   // cap work even in massive continuously explored regions.
   static const FName DungeonEncounterTag(TEXT("MoonfallDungeonEncounter"));
   const bool bDungeonRole = ActorHasTag(DungeonEncounterTag);
   if (!bBoss && !bSanctuaryPractice && (!bElite || bDungeonRole)) {
    const AMoonfallEnemy* Parent=bBossMinion?EncounterBoss.Get():nullptr;
    const bool bParentWinding=IsValid(Parent)&&!Parent->IsActorBeingDestroyed()
      &&FMath::IsFinite(Parent->WindupEndsAt)&&Parent->WindupEndsAt>Now;
    int32 NearbyWindingAllies = 0;
    int32 ScannedActors = 0;
    int32 ConsideredAllies = 0;
    const FVector PackOrigin = GetActorLocation();
    // Cap the iterator itself, not only eligible allies. Large worlds can
    // contain many bosses, defeated actors, and inactive enemies.
    for (TActorIterator<AMoonfallEnemy> It(GetWorld());
         It && ScannedActors < MoonfallPackTactics::MaxScannedActors
             && ConsideredAllies < MoonfallPackTactics::MaxNearbyCandidates; ++It) {
     ++ScannedActors;
     const AMoonfallEnemy* Other = *It;
     // Ignore teardown and invalid-health actors before consuming the limited
     // candidate budget or delaying another enemy's attack.
     if (!IsValid(Other) || Other == this || Other->IsActorBeingDestroyed()
         || !FMath::IsFinite(Other->Health) || Other->Health <= 0.f
         || (Other->bBoss && !(bBossMinion
             && MoonfallPackTactics::ShouldMinionYieldToBoss(
               bBossMinion,Other->ActorHasTag(BossEncounterTag))))
         || (Other->bElite && !bDungeonRole)
         || Other->ActorHasTag(PracticeSimulationTag)) continue;
     // A dungeon mob only coordinates with enemies from its own dungeon.
     // Never let an adjacent room/dungeon or open-world actor consume a slot.
     const bool bDungeonMember = bDungeonRole;
     const bool bOtherDungeonMember = Other->ActorHasTag(DungeonEncounterTag);
     if (bDungeonMember || bOtherDungeonMember) {
      if (!bDungeonMember || !bOtherDungeonMember
          || !IsValid(GetOwner()) || Other->GetOwner() != GetOwner()
          || DungeonChamberIndex < 0 || Other->DungeonChamberIndex != DungeonChamberIndex) continue;
     }
     // Distant allies must not exhaust our small decision-time candidate
     // budget before nearby windups can be considered. Reject corrupt
     // positions too, while preserving the outer 64-actor scan limit.
     const float AllyDistanceSquared = FVector::DistSquared2D(Other->GetActorLocation(), PackOrigin);
     if (!FMath::IsFinite(AllyDistanceSquared)
         || AllyDistanceSquared >= FMath::Square(550.f)
         || Other->GetActorLocation().ContainsNaN()
         || FMath::Abs(Other->GetActorLocation().Z-PackOrigin.Z)>100.0) continue;
     ++ConsideredAllies;
     // A corrupt infinite windup must not reserve an attack slot forever.
     // Failed/NaN timers also cannot represent a real active warning.
     if (FMath::IsFinite(Other->WindupEndsAt) && Other->WindupEndsAt > Now) {
      if (++NearbyWindingAllies >= MoonfallPackTactics::MaxWindingAllies) break;
     }
    }
    if (bParentWinding || MoonfallPackTactics::ShouldYieldAttackSlot(NearbyWindingAllies)) {
     const bool bLeader = bElite && bDungeonRole
       && ActorHasTag(FName(TEXT("MoonfallDungeonBonus")))
       && ActorHasTag(FName(TEXT("DungeonPackLeader")));
     const float SlotDelay = MoonfallPackTactics::RoleAdjustedSlotDelay(
       MoonfallPackTactics::AttackSlotDelay(FMath::Max(NearbyWindingAllies,bParentWinding?1:0), GetUniqueID()),bLeader);
     NextAttackAt = Now + SlotDelay;
     const float YieldStep=MoonfallPackTactics::YieldRepositionStep(Distance,AttackRange,SafeAIDelta,
       !bElite&&bCanSeeHero&&Now>=HitReactionEndsAt&&Now>=PostureBrokenUntil);
     if(YieldStep>0.f){
      const FVector Side(-Difference.Y,Difference.X,0.f);
      const FVector Offset=Side.GetSafeNormal2D()*MoonfallPackTactics::ApproachLaneSign(GetUniqueID())*YieldStep;
      if(!Offset.ContainsNaN()&&!Offset.IsNearlyZero()){
       FHitResult YieldHit;
       AddActorWorldOffset(Offset,true,&YieldHit);
      }
     }
     return;
    }
   }
   // Alternate quick strikes, heavy attacks and wider sweeps with distinct warnings.
   // An elite cannot chain another attack during its recovery window.
   // Phase-two bosses unlock a fourth, highly telegraphed crush.
   const int32 PatternCount = (bElite || (bBoss && BossPhase >= 2)) ? 4 : 3;
   int32 Pattern = AttackSequence++ % PatternCount;
   // Consume a valid counter opportunity in the normal attack slot.
   // Existing guardian/dungeon boss patterns remain unchanged.
   const bool bEliteAdaptiveSweep = bElite && !bBoss &&
     FMath::IsFinite(AdaptiveSweepReadyUntil) && AdaptiveSweepReadyUntil > Now;
   AdaptiveSweepReadyUntil = -1.f;
   Pattern = MoonfallEliteTactics::SelectPattern(
     Pattern, bElite, bBoss, bEliteAdaptiveSweep);
   // Training sentries alternate strike/sweep; normal foes and gods don't.
   if (bSanctuaryPractice)
    Pattern = MoonfallPracticeAI::PatternFor(AttackSequence);
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
   // Region-specific priorities reuse the five existing dodgeable special
   // moves. Avoid changing native gods or adding extra per-frame AI work.
   const bool bRegionalBoss = bBoss && !DungeonBossFamily.IsNone()
     && Tags.Contains(FName(TEXT("MoonfallDungeonBoss")));
   if (bRegionalBoss && BossPhase >= 2
       && AttackSequence % (BossPhase >= 3 ? 3 : 4) == 0) {
    if (DungeonBossFamily == FName(TEXT("Ashen"))
        || DungeonBossFamily == FName(TEXT("Foundry"))) BossSpecialMove = 2;
    else if (DungeonBossFamily == FName(TEXT("Verdant"))) BossSpecialMove = 4;
    else if (DungeonBossFamily == FName(TEXT("Glacial"))) BossSpecialMove = 5;
    else if (DungeonBossFamily == FName(TEXT("Storm"))) BossSpecialMove = 3;
    else if (DungeonBossFamily == FName(TEXT("Eclipse"))) BossSpecialMove = 1;
   }
   // Later-phase bosses alternate fast opener and a readable delayed follow-up.
   const bool bComboFollowUp = bBoss && BossPhase >= 2 && Pattern == 1;
   CurrentWindup = Pattern == 0 ? 0.38f : (Pattern == 1 ? 0.85f : (Pattern == 2 ? 0.65f : 1.05f));
   CurrentAttackMultiplier = Pattern == 1 ? 1.8f : (Pattern == 2 ? 0.75f : (Pattern == 3 ? 2.25f : 1.f));
   CurrentAttackRangeMultiplier = Pattern == 2 ? 1.25f : (Pattern == 3 ? 1.4f : 1.f);
   if (bEliteAdaptiveSweep)
    CurrentWindup = FMath::Max(CurrentWindup, MoonfallEliteTactics::CounterSweepWindup);
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
   // Opt-in authored guardian tags. Divine Z/Nyra signatures are never replaced.
   ActiveGuardianFootprint = -1;
   if (bBoss && !bDivineSignatureActive && DivineBossId.IsNone()) {
    static const FName TraitTags[4] = {
     FName(TEXT("MoonfallGuardianMoonbound")), FName(TEXT("MoonfallGuardianEmber")),
     FName(TEXT("MoonfallGuardianVenom")), FName(TEXT("MoonfallGuardianEclipse"))
    };
    for (int32 Trait = 0; Trait < 4; ++Trait) {
     if (ActorHasTag(TraitTags[Trait])) { ActiveGuardianFootprint = Trait; break; }
    }
   }
   if (ActiveGuardianFootprint >= 0) {
    GuardianOrigin = GetActorLocation();
    GuardianTarget = HeroPosition;
    CurrentWindup = FMath::Max(CurrentWindup, 1.35f);
    CurrentAttackMultiplier = FMath::Min(CurrentAttackMultiplier, 1.6f);
    CurrentAttackRangeMultiplier = FMath::Max(CurrentAttackRangeMultiplier, 2.0f);
    bComboSecondHitPending = false;
    BossSpecialMove = 0;
   }
   if (bRangedCombatRole) {
    // Physical arrows and arcane bolts have distinct, readable windups.
    CurrentWindup=FMath::Max(CurrentWindup,
      MoonfallPackTactics::RangedProjectileWindup(bCasterCombatRole));
    CurrentAttackMultiplier=FMath::Min(CurrentAttackMultiplier,bCasterCombatRole?1.f:1.1f);
    CurrentAttackRangeMultiplier=1.f;
    bComboSecondHitPending=false;
    BossSpecialMove=0;
   }
   if (bSanctuaryPractice) {
    // Allow new players to read and react to the existing visual tell.
    CurrentWindup = MoonfallPracticeAI::ReadableWindup(CurrentWindup, AttackSequence);
    CurrentAttackMultiplier = FMath::Min(CurrentAttackMultiplier, 1.f);
    CurrentAttackRangeMultiplier = FMath::Min(CurrentAttackRangeMultiplier, 1.f);
   }
   bCompanionAttackCommitted=false;LockedCompanionTarget.Reset();
   if(!bBoss&&!bElite&&!bSanctuaryPractice&&!bRangedCombatRole){
    if(auto* Mode=GetWorld()->GetAuthGameMode<AMoonfallUnrealGameMode>()){
     if(auto* Target=Mode->SelectCompanionCounterTarget(Hero,this,Now,AttackRange*CurrentAttackRangeMultiplier)){
      LockedCompanionTarget=Target;bCompanionAttackCommitted=true;
      LockedAttackDirection=(Target->GetActorLocation()-GetActorLocation()).GetSafeNormal2D();
      SetActorRotation(FRotator(0.f,LockedAttackDirection.Rotation().Yaw,0.f));
      bComboSecondHitPending=false;
     }
    }
   }
   CommittedSwordAngle = bElite && !bBoss && !bSanctuaryPractice && !bCompanionAttackCommitted
     ? MoonfallOmniattack::EnemyAngle(Pattern) : -1;
   WindupEndsAt = Now + CurrentWindup;
   if(bRangedCombatRole){
    LockedRangedAimPoint=HeroPosition+FVector(0.f,0.f,55.f);
    if(!LockedRangedAimPoint.ContainsNaN()){
     OnRangedAttackTelegraphed(LockedRangedAimPoint,CurrentWindup,bCasterCombatRole);
     if(!IsValid(this)||IsActorBeingDestroyed())return;
    }
   } else LockedRangedAimPoint=FVector::ZeroVector;
   // A real visible attack commitment is a defensive assist opportunity,
   // even before the player lands their first strike. Practice foes excluded.
   if(!bBoss&&!bElite&&!bSanctuaryPractice&&bCanSeeHero&&!bCompanionAttackCommitted)
    if(auto* Mode=GetWorld()->GetAuthGameMode<AMoonfallUnrealGameMode>())
     Mode->ReportCompanionThreat(Hero,this,Now);
   if (bBoss) BossWarningEndTime = WindupEndsAt;
   if (bRegionalBoss) {
    // Stable readable move labels published only at warning startup.
    const TCHAR* AttackLabel = BossSpecialMove == 1 ? TEXT("MOON CLEAVE")
      : BossSpecialMove == 2 ? TEXT("HEAVY SLAM")
      : BossSpecialMove == 3 ? TEXT("DASH STRIKE")
      : BossSpecialMove == 4 ? TEXT("SPINNING SWEEP")
      : BossSpecialMove == 5 ? TEXT("SKYFALL SLAM")
      : Pattern == 0 ? TEXT("QUICK STRIKE")
      : Pattern == 1 ? TEXT("POWER STRIKE")
      : Pattern == 2 ? TEXT("WIDE SWEEP")
      : TEXT("CRUSH ATTACK");
    DungeonBossAttackLabel = FText::FromString(AttackLabel);
    if (ActiveGuardianFootprint >= 0) {
     DungeonBossAttackLabel = FText::FromString(UTF8_TO_TCHAR(
       MoonfallGuardianFootprint::Name(
        static_cast<MoonfallGuardianFootprint::Shape>(ActiveGuardianFootprint))));
    }
    DungeonBossAttackWarningDuration = FMath::Clamp(CurrentWindup, 0.1f, 5.f);
    // Red is dodge-only. Blue sweeps are parryable; ordinary strikes
    // preserve the existing neutral guard and parry mechanics.
    bDungeonBossAttackDodgeOnly = BossSpecialMove == 2
      || BossSpecialMove == 3 || BossSpecialMove == 5;
   }
   if (ActiveGuardianFootprint >= 0) {
    OnDungeonBossAttackTelegraphed(FName(TEXT("Guardian")),
      FName(UTF8_TO_TCHAR(MoonfallGuardianFootprint::Name(
        static_cast<MoonfallGuardianFootprint::Shape>(ActiveGuardianFootprint)))), CurrentWindup);
   }
   // Blueprint hooks run synchronously. Publish a fully initialized warning
   // clock before notifying the animation / accessible combat indicator.
   if (bDivineSignatureActive) {
    OnDivineAttackTelegraphed(ActiveDivineMoveId, DivineSignatureCenter, DivineSignatureRadius, CurrentWindup);
   } else if (bRegionalBoss && BossSpecialMove >= 1 && BossSpecialMove <= 5) {
    const FName SpecialId = BossSpecialMove == 1 ? FName(TEXT("MoonCleave"))
      : BossSpecialMove == 2 ? FName(TEXT("EclipseSlam"))
      : BossSpecialMove == 3 ? FName(TEXT("DashStrike"))
      : BossSpecialMove == 4 ? FName(TEXT("SpinningSweep"))
      : FName(TEXT("SkyfallSlam"));
    OnDungeonBossAttackTelegraphed(DungeonBossFamily, SpecialId, CurrentWindup);
   }
  }
 }
}

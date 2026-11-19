#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MoonfallEnemy.generated.h"
class UStaticMeshComponent;
class USoundBase;
class AMoonfallHeroPawn;
UCLASS()
class MOONFALLUNREAL_API AMoonfallEnemy : public AActor
{
 GENERATED_BODY()
public:
 AMoonfallEnemy();
 virtual void Tick(float DeltaSeconds) override;
 virtual void BeginPlay() override;
 // Dungeon encounter level captured on first entry. Zero preserves ordinary world scaling.
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Encounter")
 int32 EncounterLevelOverride = 0;
 UPROPERTY(EditAnywhere, Category="Combat") float ChaseSpeed = 180.f;
 UPROPERTY(EditAnywhere, Category="Combat") float AttackDamage = 8.f;
 UPROPERTY(EditAnywhere, Category="Combat") float AttackRange = 145.f;
 UPROPERTY(EditAnywhere, Category="Combat") float AttackCooldown = 1.3f;
 // Read-only telegraph snapshot: the HUD may ask nearby mobs when a strike
 // will land, without changing private AI timers or doing any trace.
 // Bosses keep their existing dedicated warning display.
 UFUNCTION(BlueprintPure, Category="Combat|Telegraph")
 float GetAttackWarningRemaining(float WorldTime) const;
 UFUNCTION(BlueprintPure, Category="Combat|Telegraph")
 float GetAttackWarningDuration() const;
 UFUNCTION(BlueprintPure, Category="Combat|Telegraph")
 FString GetAttackWarningLabel() const;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat") bool bElite = false;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat") bool bBoss = false;
 // Spawn-initialized only: isolates attack slots between chambers of one dungeon.
 // -1 means no known chamber and must not join dungeon coordination.
 int32 DungeonChamberIndex = -1;
 // Generic dungeon-boss presentation only. Never replaces divine boss IDs.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Dungeon|Boss")
 FName DungeonBossFamily = NAME_None;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Dungeon|Boss")
 FText DungeonBossTitle;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Dungeon|Boss")
 int32 DungeonBossRank = 0;
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Dungeon|Boss")
 void OnDungeonBossIdentityReady(FName FamilyId, FText Title, int32 Rank);
 // Fires once when a regional special warning actually begins. A Blueprint
 // can play a clear icon, sound, animation or controller cue for this move.
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Dungeon|Boss")
 void OnDungeonBossAttackTelegraphed(FName FamilyId, FName MoveId, float WarningSeconds);
 // Read-only native HUD warning. Updated only when an attack is committed;
 // the existing warning deadline determines when this label is visible.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Dungeon|Boss")
 FText DungeonBossAttackLabel;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Dungeon|Boss")
 float DungeonBossAttackWarningDuration = 0.f;
 // Red: dodge-only (cannot block/parry). Blue: standard parryable boss strike.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Dungeon|Boss")
 bool bDungeonBossAttackDodgeOnly = false;
 // Humanoid enemy ancestry, not a divine boss identity or visual asset.
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Race", meta=(ClampMin="0", ClampMax="11")) int32 RaceId = 0;
 UFUNCTION(BlueprintPure, Category="Combat|Race") FString GetRaceName() const;
 // Divine world bosses are NPC encounters, never playable hero identities.
 // The exact source artwork is intentionally not recreated or modified here.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Boss|Divine") FName DivineBossId = NAME_None;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Boss|Divine") FText DivineBossTitle;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Boss|Divine") FName DivineBossRegion = NAME_None;
 // Optional Blueprint animation/VFX callbacks. No artwork or animation asset generated.
 UFUNCTION(BlueprintImplementableEvent, Category="Boss|Divine") void OnDivineAttackTelegraphed(FName MoveId, FVector TargetCenter, float Radius, float WarningSeconds);
 UFUNCTION(BlueprintImplementableEvent, Category="Boss|Divine") void OnDivineAttackResolved(FName MoveId, FVector TargetCenter, bool bHeroInsideMarker);
 UFUNCTION(BlueprintImplementableEvent, Category="Boss|Divine") void OnDivineAttackInterrupted(FName MoveId);
 // Optional presentation hook: use the original god art in derived Blueprints.
 UFUNCTION(BlueprintImplementableEvent, Category="Boss|Divine") void OnDivinePhaseEntered(FName BossId, int32 NewPhase, float RecoverySeconds);
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Combat") int32 BossPhase = 1;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Boss") float BossMaxHealth = 0.f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Boss") float BossWarningEndTime = -1.f;
 UFUNCTION(BlueprintImplementableEvent, Category="Combat") void OnHitFeedback(float Damage, float RemainingHealth);
 UFUNCTION(BlueprintImplementableEvent, Category="Combat") void OnEnemyAttack();
 // Asset-authoring hooks: Blueprint may play attack anticipation animations,
 // voice, and readable cues after a *real* committed windup begins.
 UFUNCTION(BlueprintImplementableEvent, Category="Combat|Presentation")
 void OnAttackTelegraphCommitted(FName MoveId, float WarningSeconds);
 // Resolved marks the actual impact timer, not guaranteed damage to the hero.
 // Interrupted fires on canceled attacks, so animations can be stopped.
 UFUNCTION(BlueprintImplementableEvent, Category="Combat|Presentation")
 void OnAttackTelegraphResolved(FName MoveId);
 UFUNCTION(BlueprintImplementableEvent, Category="Combat|Presentation")
 void OnAttackTelegraphInterrupted(FName MoveId);
 // Optional audio asset. Unassigned by default; elite-only once at commitment.
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Presentation")
 TObjectPtr<USoundBase> EliteAttackTellSound = nullptr;
 UFUNCTION(BlueprintImplementableEvent, Category="Combat") void OnEnemyDefeated(bool bKillingBlowWasShatter);
 // Animation/VFX hook: fires once per successful perfect parry, not every tick.
 UFUNCTION(BlueprintImplementableEvent, Category="Combat") void OnPerfectParried(float StaggerSeconds);
 // Enemy posture is independent of health. Pressure from parries and spells
 // opens a brief vulnerability window when the meter fills.
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Posture", meta=(ClampMin="1.0")) float MaxPosture = 100.f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Combat|Posture") float Posture = 0.f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Posture", meta=(ClampMin="0.0")) float ParryPostureDamage = 40.f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Posture", meta=(ClampMin="0.0", ClampMax="100.0")) float PerfectDodgePostureDamage = 18.f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Posture", meta=(ClampMin="0.0", ClampMax="2.0")) float PerfectDodgeRecoverySeconds = 0.5f;
 UFUNCTION(BlueprintImplementableEvent, Category="Combat|Posture") void OnPerfectDodged(float PosturePressure);
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Posture", meta=(ClampMin="0.0")) float SpellPostureScale = 0.3f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Posture", meta=(ClampMin="0.0")) float PostureRecoveryPerSecond = 9.f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Posture", meta=(ClampMin="1.0", ClampMax="3.0")) float BrokenPostureDamageMultiplier = 1.35f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Combat|Posture") float PostureBrokenUntil = -1.f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Combat|Posture") bool bPostureFinisherAvailable = false;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Posture", meta=(ClampMin="1.0", ClampMax="4.0")) float PostureFinisherDamageMultiplier = 1.8f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Posture", meta=(ClampMin="0.0", ClampMax="6.0")) float BossPostureBreakRecoverySeconds = 2.5f;
 // Bosses adapt after each broken stance; hard caps prevent runaway scaling.
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Posture", meta=(ClampMin="0.0", ClampMax="0.5")) float BossPostureGrowthPerBreak = 0.12f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Posture", meta=(ClampMin="1.0", ClampMax="3.0")) float BossPostureGrowthCap = 1.75f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Posture", meta=(ClampMin="0.0", ClampMax="5.0")) float PosturePressureRecoveryDelaySeconds = 3.f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Combat|Posture") int32 PostureBreakCount = 0;
 UFUNCTION(BlueprintImplementableEvent, Category="Combat|Posture") void OnShatterFinisher(float DamageDealt);
 UFUNCTION(BlueprintImplementableEvent, Category="Combat|Posture") void OnPostureBroken(float VulnerabilitySeconds);
 UFUNCTION(BlueprintImplementableEvent, Category="Combat|Posture") void OnPostureChanged(float CurrentPosture, float MaximumPosture);
 // Optional animation cue for earned hits 4/8/12; no new asset required.
 UFUNCTION(BlueprintImplementableEvent, Category="Combat|Combo") void OnComboMilestone(int32 Stage);
 void RegisterComboMilestone(int32 Stage);
 UFUNCTION(BlueprintCallable, Category="Combat|Posture") void ApplyPostureDamage(float Amount);
 UFUNCTION(BlueprintImplementableEvent, Category="Boss") void OnBossPhaseChanged(int32 NewPhase);
private:
 // Weakly retain the current hero for crowded-world AI ticks; never own the pawn.
 TWeakObjectPtr<AMoonfallHeroPawn> CachedHero;
 float NextHeroCacheRefreshAt = 0.f;
 float MaxHealthAtSpawn = 0.f;
 int32 SpawnBalanceLevel = 1;
 float BossPhaseMessageUntil = 0.f;
 float NextAttackAt = 0.f;
 // Only a successful hero parry can arm one tactical sweep. No extra AI ticks.
 float AdaptiveSweepReadyUntil = -1.f;
 float NextAdaptiveSweepAllowedAt = 0.f;
 bool bAdaptiveSweepCommitted = false;
 // Minimum interval between optional elite warning sounds, even in a pack.
 // Not a saved property and has no influence on actual attack cooldown.
 float NextEliteAttackTellSoundAt = 0.f;
 float NextPathRefreshAt = 0.f;
 float NextBlockedRepathAt = 0.f;
 // Ordinary enemies remember the last visible hero location for a short time.
 float NextSightCheckAt = 0.f;
 float LastSawHeroAt = -100.f;
 // Investigate briefly after reaching the hero's last seen position.
 float InvestigationEndsAt = -1.f;
 // Maximum time to reach the last known player location.
 float SearchTravelEndsAt = -1.f;
 bool bCanSeeHero = false;
 FVector LastKnownHeroPosition = FVector::ZeroVector;
 // Home position for persistent dungeon patrol behavior.
 FVector DungeonHomePosition = FVector::ZeroVector;
 // Cooldown limits wall-escape sweeps when several enemies crowd a doorway.
 float NextObstacleSidestepAt = 0.f;
 int32 ObstacleSidestepCount = 0;
 // Reverse an unsuccessful combat wall-slide on the next cooldown.
 bool bReverseCombatWallSlide = false;
 // Brief tactical withdrawal for wounded elites, with a long re-use cooldown.
 float TacticalRetreatUntil = -1.f;
 float TacticalReengageUntil = -1.f;
 float NextTacticalRetreatAt = 0.f;
 bool bRetreatFlankLeft = false;
 FVector CachedWaypoint = FVector::ZeroVector;
 FVector LockedAttackDirection = FVector::ZeroVector;
 float WindupEndsAt = -1.f;
 // Ephemeral cue identity is neither replicated nor serialized to saved games.
 FName ActiveNonBossTelegraphMove = NAME_None;
 void EndNonBossTelegraph(bool bResolved);
 float HitReactionEndsAt = -1.f;
 float PostureRecoveryStartsAt = 0.f;
 float BasePostureAtSpawn = 100.f;
 float NextPostureBreakAllowedAt = 0.f;
 float DefeatAnimationStartedAt = -100.f;
 FVector DefeatAnimationOrigin = FVector::ZeroVector;
 int32 DefeatAnimationStyle = 1;
 bool bDefeatAnimationShatter = false;
 void TickDefeatAnimation(float DeltaSeconds);
 int32 AttackSequence = 0;
 // Random basic boss attacks cannot repeat immediately, even after phase changes.
 int32 LastBossAttackPattern = -1;
 bool bComboSecondHitPending = false;
 bool bComboSecondHitActive = false;
 int32 BossSpecialMove = 0;
 // Distinct, telegraphed divine signatures use the standard damage pipeline.
 void CancelDivineSignature();
 // Ranged cast sight is cached and throttled, not traced on every AI tick.
 float NextDivineCastSightAt = 0.f;
 bool bDivineCastLineClear = false;
 bool bDivineSignatureActive = false;
 FName ActiveDivineMoveId = NAME_None;
 bool bDivineAlternateGeometry = false;
 int32 DivineSignatureCastCount = 0;
 FVector DivineSignatureCenter = FVector::ZeroVector;
 float DivineSignatureRadius = 0.f;
 float CurrentWindup = 0.5f;
 float CurrentAttackMultiplier = 1.f;
 float CurrentAttackRangeMultiplier = 1.f;
public:
 UPROPERTY(VisibleAnywhere) UStaticMeshComponent* Body;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Combat") float LastHitAmount = 0.f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Combat") float LastHitTime = -10.f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Combat") bool bLastHitWasShatter = false;
 // Fixed-size history preserves quick successive hits without spawning actors.
 static constexpr int32 DamageHistorySize = 4;
 struct FRecentDamageHit {
  float Amount = 0.f;
  float Time = -10.f;
  bool bShatter = false;
  bool bKillingBlow = false;
 };
 FRecentDamageHit DamageHistory[DamageHistorySize];
 int32 NextDamageHistorySlot = 0;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat") float Health = 80.f;
 UFUNCTION(BlueprintCallable, Category="Combat") void BeginFinisherDefeat(int32 Style);
 UFUNCTION(BlueprintCallable, Category="Combat") void TakeSpellDamage(float Amount);
};

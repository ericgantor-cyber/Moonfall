#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MoonfallEnemy.generated.h"
class UStaticMeshComponent;
class USkeletalMeshComponent;
class UAnimMontage;
UCLASS()
class MOONFALLUNREAL_API AMoonfallEnemy : public AActor
{
 GENERATED_BODY()
public:
 AMoonfallEnemy();
 virtual void Tick(float DeltaSeconds) override;
 virtual void BeginPlay() override;
 UPROPERTY(EditAnywhere, Category="Combat") float ChaseSpeed = 180.f;
 UPROPERTY(EditAnywhere, Category="Combat") float AttackDamage = 8.f;
 UPROPERTY(EditAnywhere, Category="Combat") float AttackRange = 145.f;
 UPROPERTY(EditAnywhere, Category="Combat") float AttackCooldown = 1.3f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat") bool bElite = false;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat") bool bBoss = false;
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
 // A successful overcharged hit causes a short, cooldown-limited impact.
 // This is a hit reaction, not an instant full posture break.
 UFUNCTION(BlueprintCallable, Category="Combat|Posture") void ApplyDualCastImpact();
 UFUNCTION(BlueprintImplementableEvent, Category="Combat|Posture") void OnDualCastImpact(float ReactionSeconds);
 UFUNCTION(BlueprintCallable, Category="Combat|Posture") void ApplyPostureDamage(float Amount);
 UFUNCTION(BlueprintImplementableEvent, Category="Boss") void OnBossPhaseChanged(int32 NewPhase);
private:
 float MaxHealthAtSpawn = 0.f;
 int32 SpawnBalanceLevel = 1;
 float BossPhaseMessageUntil = 0.f;
 float NextAttackAt = 0.f;
 float NextDualCastImpactAt = 0.f;
 float NextPathRefreshAt = 0.f;
 float NextBlockedRepathAt = 0.f;
 // Ordinary enemies remember the last visible hero location for a short time.
 float NextSightCheckAt = 0.f;
 float LastSawHeroAt = -100.f;
 bool bCanSeeHero = false;
 FVector LastKnownHeroPosition = FVector::ZeroVector;
 // Cooldown limits wall-escape sweeps when several enemies crowd a doorway.
 float NextObstacleSidestepAt = 0.f;
 int32 ObstacleSidestepCount = 0;
 // Brief tactical withdrawal for wounded elites, with a long re-use cooldown.
 float TacticalRetreatUntil = -1.f;
 float TacticalReengageUntil = -1.f;
 float NextTacticalRetreatAt = 0.f;
 bool bRetreatFlankLeft = false;
 FVector CachedWaypoint = FVector::ZeroVector;
 FVector LockedAttackDirection = FVector::ZeroVector;
 float WindupEndsAt = -1.f;
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
 // Shared optional skeletal rig used by normal enemies, elites and bosses.
 // The original sphere Body stays the physics/navigation root and the
 // fallback visual; assigning a skeletal mesh never alters imported art.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Rig") TObjectPtr<USkeletalMeshComponent> CharacterRig;
 UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Moonfall|Rig") TObjectPtr<UAnimMontage> AttackRigMontage;
 UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Moonfall|Rig") TObjectPtr<UAnimMontage> DeathRigMontage;
 UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Moonfall|Rig") TObjectPtr<UAnimMontage> HitRigMontage;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Rig") bool bCharacterRigActive = false;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Rig") bool bLastRigMontagePlayed = false;
 UFUNCTION(BlueprintCallable, Category="Moonfall|Rig") void RefreshCharacterRig();
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Rig") void OnCharacterRigCue(FName CueId, bool bMontagePlayed);
private:
 void PlayCharacterRigCue(FName CueId, UAnimMontage* Montage);
public:
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

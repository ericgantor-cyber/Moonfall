#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MoonfallEnemy.generated.h"
class UStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;
class UPointLightComponent;
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
 UPROPERTY(EditDefaultsOnly, Category="Moonfall|Enemy Art") UStaticMesh* RegularEnemyArt = nullptr;
 UPROPERTY(EditDefaultsOnly, Category="Moonfall|Enemy Art") UMaterialInterface* RegularEnemyMaterial = nullptr;
 UPROPERTY(EditDefaultsOnly, Category="Moonfall|Enemy Art") UStaticMesh* BossEnemyArt = nullptr;
 UPROPERTY(EditDefaultsOnly, Category="Moonfall|Enemy Art") UMaterialInterface* BossEnemyMaterial = nullptr;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat") bool bCorruptedCrow = false;
 // Optional imported art overrides; default primitive silhouette remains a safe fallback.
 UPROPERTY(EditDefaultsOnly, Category="Moonfall|Crow Art") UStaticMesh* CrowBodyArt = nullptr;
 UPROPERTY(EditDefaultsOnly, Category="Moonfall|Crow Art") UStaticMesh* CrowWingArt = nullptr;
 UPROPERTY(EditDefaultsOnly, Category="Moonfall|Crow Art") UMaterialInterface* CrowFeatherMaterial = nullptr;
 UPROPERTY(EditDefaultsOnly, Category="Moonfall|Crow Art") UMaterialInterface* CrowAccentMaterial = nullptr;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Combat") int32 BossPhase = 1;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Boss") float BossMaxHealth = 0.f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Boss") float BossWarningEndTime = -1.f;
 UFUNCTION(BlueprintImplementableEvent, Category="Combat") void OnHitFeedback(float Damage, float RemainingHealth);
 UFUNCTION(BlueprintImplementableEvent, Category="Combat") void OnEnemyAttack();
 UFUNCTION(BlueprintImplementableEvent, Category="Boss") void OnBossPhaseChanged(int32 NewPhase);
private:
 float CrowAttackEndsAt = -1.f;
 int32 CrowAttackSequence = 0;
 int32 CrowAttackKind = 0; // 0 diving peck, 1 sweeping talon, 2 feint then dive
 bool bCrowHasHit = false;
 UPROPERTY(VisibleAnywhere) UStaticMeshComponent* CrowLeftWing;
 UPROPERTY(VisibleAnywhere) UStaticMeshComponent* CrowRightWing;
 UPROPERTY(VisibleAnywhere) UStaticMeshComponent* CrowBeak;
 UPROPERTY(VisibleAnywhere) UStaticMeshComponent* CrowTail;
 UPROPERTY(VisibleAnywhere) UStaticMeshComponent* CrowLeftTalon;
 UPROPERTY(VisibleAnywhere) UStaticMeshComponent* CrowRightTalon;
 UPROPERTY(VisibleAnywhere) UStaticMeshComponent* CrowLeftEye;
 UPROPERTY(VisibleAnywhere) UStaticMeshComponent* CrowRightEye;
 UPROPERTY(VisibleAnywhere) UPointLightComponent* CrowWarningGlow;
 float CrowRecoverUntil = 0.f;
 bool bCrowDiving = false;
 void TickCorruptedCrow(float DeltaSeconds);
 void UpdateCrowAppearance(float TimeSeconds);
 float MaxHealthAtSpawn = 0.f;
 int32 SpawnBalanceLevel = 1;
 float BossPhaseMessageUntil = 0.f;
 float NextAttackAt = 0.f;
 float NextPathRefreshAt = 0.f;
 float NextBlockedRepathAt = 0.f;
 FVector CachedWaypoint = FVector::ZeroVector;
 float WindupEndsAt = -1.f;
 float HitReactionEndsAt = -1.f;
 int32 AttackSequence = 0;
 bool bComboSecondHitPending = false;
 bool bComboSecondHitActive = false;
 int32 BossSpecialMove = 0;
 float CurrentWindup = 0.5f;
 float CurrentAttackMultiplier = 1.f;
 float CurrentAttackRangeMultiplier = 1.f;
public:
 UPROPERTY(VisibleAnywhere) UStaticMeshComponent* Body;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Combat") float LastHitAmount = 0.f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Combat") float LastHitTime = -10.f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat") float Health = 80.f;
 UFUNCTION(BlueprintCallable, Category="Combat") void TakeSpellDamage(float Amount);
};

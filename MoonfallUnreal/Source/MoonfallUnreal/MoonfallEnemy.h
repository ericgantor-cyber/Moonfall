#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MoonfallEnemy.generated.h"
class UStaticMeshComponent;
UCLASS()
class MOONFALLUNREAL_API AMoonfallEnemy : public AActor
{
 GENERATED_BODY()
public:
 AMoonfallEnemy();
 virtual void Tick(float DeltaSeconds) override;
 UPROPERTY(EditAnywhere, Category="Combat") float ChaseSpeed = 180.f;
 UPROPERTY(EditAnywhere, Category="Combat") float AttackDamage = 8.f;
 UPROPERTY(EditAnywhere, Category="Combat") float AttackRange = 145.f;
 UPROPERTY(EditAnywhere, Category="Combat") float AttackCooldown = 1.3f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat") bool bElite = false;
 UFUNCTION(BlueprintImplementableEvent, Category="Combat") void OnHitFeedback(float Damage, float RemainingHealth);
 UFUNCTION(BlueprintImplementableEvent, Category="Combat") void OnEnemyAttack();
private:
 float NextAttackAt = 0.f;
 float NextPathRefreshAt = 0.f;
 float NextBlockedRepathAt = 0.f;
 FVector CachedWaypoint = FVector::ZeroVector;
 float WindupEndsAt = -1.f;
 float HitReactionEndsAt = -1.f;
 int32 AttackSequence = 0;
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

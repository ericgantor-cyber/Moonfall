#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MoonfallHeroPawn.generated.h"
class USpringArmComponent;
class UCameraComponent;
UCLASS()
class MOONFALLUNREAL_API AMoonfallHeroPawn : public ACharacter
{
 GENERATED_BODY()
public:
 AMoonfallHeroPawn();
 virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
 virtual void BeginPlay() override;
 virtual void Tick(float DeltaSeconds) override;
 UFUNCTION(BlueprintCallable, Category="Moonfall|Combat") void ApplyMoonfallDamage(float Amount);
 // Enemy strikes provide an origin so guard only catches frontal blows.
 bool ApplyMoonfallDamageFrom(float Amount, const FVector& SourceLocation);
 UFUNCTION(BlueprintCallable, Category="Moonfall|Combat") void RewardPerfectDodge();
 // Returns true only when this specific attack consumed the perfect-dodge window.
 bool TryRewardPerfectDodge();
 UPROPERTY(BlueprintReadOnly, Category="Moonfall|Combat") bool bGuarding = false;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Combat") float GuardBrokenUntil = 0.f;
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Combat") void OnGuardBroken();
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Combat") void OnGuardImpact(float DamageBlocked, bool bPerfectParry);
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Combat") void OnHealthChanged(float NewHealth, float MaxHealth);
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Combat") void OnDamageImpact(float DamageTaken);
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Accessibility") bool bReduceMotion = false;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Accessibility") float CombatTextScale = 1.f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Accessibility") float BossWarningScale = 1.f;
 UPROPERTY(BlueprintReadOnly, Category="Moonfall|Options") bool bOptionsOpen = false;
 UPROPERTY(BlueprintReadOnly, Category="Moonfall|Options") int32 OptionsSelection = 0;
 UPROPERTY(BlueprintReadOnly, Category="Moonfall|Options") int32 FrameGenerationPreference = 0;
 UPROPERTY(BlueprintReadOnly, Category="Moonfall|Options") bool bFrameGenerationAvailable = false;
 UPROPERTY(BlueprintReadOnly, Category="Moonfall|Options") bool bFrameGenerationPluginDetected = false;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Combat") float Mana = 100.f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Combat") float MaxMana = 100.f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Combat", meta=(ClampMin="0.0")) float Stamina = 100.f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Combat", meta=(ClampMin="1.0")) float MaxStamina = 100.f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Combat", meta=(ClampMin="0.0")) float DodgeStaminaCost = 25.f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Combat") float Health = 100.f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Progression") float MaxHealth = 100.f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Progression") float SpellPowerMultiplier = 1.f;
 UPROPERTY(BlueprintReadOnly, Category="Moonfall|Progression") int32 Level = 1;
 UPROPERTY(BlueprintReadOnly, Category="Moonfall|Progression") int32 Experience = 0;
 UPROPERTY(BlueprintReadOnly, Category="Moonfall|Progression") int32 ExperienceToNextLevel = 100;
 UFUNCTION(BlueprintCallable, Category="Moonfall|Progression") void AwardExperience(int32 Amount);
 UFUNCTION(BlueprintCallable, Category="Moonfall|Loot") void AwardEquipmentDrop(bool bEliteEnemy, bool bBossEnemy);
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Loot") int32 EquipmentTier = 0;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Loot") int32 Salvage = 0;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Loot") int32 LastLootRarity = 0;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Loot") int32 LastLootTier = 0;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Loot") int32 LastLootSlot = 0;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Loot") int32 LastLootUpgrade = 0;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Loot") TArray<int32> InventoryItemCodes;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Equipment") int32 WeaponTier = 0;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Equipment") int32 ArmorTier = 0;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Equipment") int32 AccessoryTier = 0;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Combat") int32 LeftSpell = 0;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Combat") int32 RightSpell = 1;
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Combat") void OnSpellCast(int32 School, bool bLeftHand);
private:
 UPROPERTY(VisibleAnywhere) USpringArmComponent* CameraBoom;
 UPROPERTY(VisibleAnywhere) UCameraComponent* FollowCamera;
 float LastCastTime[2] = {-100.f,-100.f};
 float NextDodgeAt = 0.f;
 FVector IncomingDamageSource = FVector::ZeroVector;
 bool bHasIncomingDamageSource = false;
 bool bLastIncomingHitParried = false;
 float ParryWindowEndsAt = -1.f;
 float NextParryAt = 0.f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Combat", meta=(ClampMin="0.05", ClampMax="0.3")) float ParryWindowSeconds = 0.18f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Combat", meta=(ClampMin="0.3", ClampMax="2.0")) float ParryCooldownSeconds = 0.8f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Combat", meta=(ClampMin="0.0")) float GuardStaminaCost = 8.f;
 // Guard stance costs stamina over time, even without an incoming strike.
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Combat", meta=(ClampMin="0.0", ClampMax="30.0")) float GuardStaminaDrainPerSecond = 6.f;
 UPROPERTY(EditAnywhere, Category="Moonfall|Combat", meta=(ClampMin="0.0", ClampMax="1.0")) float GuardImpactStaminaScale = 0.15f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Combat", meta=(ClampMin="0.0")) float PerfectParryStaminaReward = 12.f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Combat", meta=(ClampMin="0.0", ClampMax="5.0")) float ParryCounterWindowSeconds = 2.f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Combat", meta=(ClampMin="1.0", ClampMax="2.0")) float ParryCounterDamageMultiplier = 1.25f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Combat") float ParryCounterUntil = -1.f;
 UPROPERTY(EditAnywhere, Category="Moonfall|Combat", meta=(ClampMin="0.0", ClampMax="3.0")) float GuardBreakRecoverySeconds = 1.0f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Movement", meta=(ClampMin="0.2", ClampMax="1.0")) float GuardMoveSpeedMultiplier = 0.65f;
 float DamageImmunityUntil = 0.f;
 float PerfectDodgeWindowUntil = -1.f;
 float LastPerfectDodgeRewardAt = -100.f;
 float ImpactCameraOffset = 0.f;
 UPROPERTY(EditAnywhere, Category="Moonfall|Combat") float ManaRegenPerSecond = 8.f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Combat", meta=(ClampMin="0.24", ClampMax="5.0")) float DodgeCooldown = 1.3f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Movement", meta=(ClampMin="250.0", ClampMax="1200.0")) float DodgeLaunchSpeed = 650.f;
 UPROPERTY(EditAnywhere, Category="Moonfall|Combat", meta=(ClampMin="0.0")) float StaminaRegenPerSecond = 24.f;
 UPROPERTY(EditAnywhere, Category="Moonfall|Combat", meta=(ClampMin="0.0")) float PerfectDodgeStaminaReward = 10.f;
 UPROPERTY(EditAnywhere, Category="Moonfall|Movement", meta=(ClampMin="0.0")) float SprintStaminaDrainPerSecond = 18.f;
 UPROPERTY(EditAnywhere, Category="Moonfall|Movement", meta=(ClampMin="1.0", ClampMax="2.5")) float SprintSpeedMultiplier = 1.45f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Movement", meta=(ClampMin="0.0")) float SprintRestartStamina = 30.f;
 UPROPERTY(EditAnywhere, Category="Moonfall|Movement", meta=(ClampMin="1.0", ClampMax="20.0")) float SprintAccelerationRate = 9.f;
 float NormalWalkSpeed = 600.f;
 bool bSprintHeld = false;
 bool bSprintExhausted = false;
 float StaminaRegenStartsAt = 0.f;
 void MoveForward(float Amount);
 void MoveRight(float Amount);
 void Turn(float Amount);
 void LookUp(float Amount);
 void CastLeft();
 void CastRight();
 void CastBoth();
 void Dodge();
 void BeginSprint();
 void EndSprint();
 void BreakGuard();
 void BeginGuard();
 void EndGuard();
 void OpenOptions();
 void OptionsUp();
 void OptionsDown();
 void OptionsLeft();
 void OptionsRight();
 void AdjustOption(int32 Direction);
 void CycleGraphicsQuality();
 void ToggleDisplayMode();
 bool TryCast(int32 School, int32 Hand, float Multiplier);
};
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MoonfallHeroPawn.generated.h"
class USpringArmComponent;
class UCameraComponent;
class AMoonfallEnemy;
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
 UFUNCTION(BlueprintCallable, Category="Moonfall|Combat") void RewardPerfectDodge();
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
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Map") float MinimapRange = 2200.f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Map") float WorldMapRange = 15000.f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Map") FVector2D WorldMapPan = FVector2D::ZeroVector;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Map") bool bWorldMapOpen = false;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Map") bool bQuestTargetActive = false;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Map") FVector QuestTarget = FVector::ZeroVector;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Map") bool bPersonalWaypointActive = false;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Map") FVector PersonalWaypoint = FVector::ZeroVector;
 UFUNCTION(BlueprintCallable, Category="Moonfall|Map") void SetQuestTarget(FVector WorldLocation);
 UFUNCTION(BlueprintCallable, Category="Moonfall|Map") void ClearQuestTarget();
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Quest") bool bArenaBossQuestActive = false;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Quest") bool bArenaBossQuestComplete = false;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Quest") float ArenaBossQuestCompletedAt = -1.f;
 // Track the actual spawned actor, not an approximate spawn point. Weak references
 // never retain a destroyed enemy after streaming, level reset, or defeat.
 UPROPERTY(Transient) TWeakObjectPtr<AMoonfallEnemy> ActiveArenaBoss;
 UFUNCTION(BlueprintCallable, Category="Moonfall|Quest") void BeginArenaBossQuest(AMoonfallEnemy* Boss);
 UFUNCTION(BlueprintCallable, Category="Moonfall|Quest") void CompleteArenaBossQuest(AMoonfallEnemy* DefeatedBoss);
 UFUNCTION(BlueprintPure, Category="Moonfall|Quest") AMoonfallEnemy* GetArenaBossQuestTarget() const;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Combat") float Mana = 100.f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Combat") float MaxMana = 100.f;
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
 float DamageImmunityUntil = 0.f;
 float PerfectDodgeWindowUntil = -1.f;
 float LastPerfectDodgeRewardAt = -100.f;
 float ImpactCameraOffset = 0.f;
 UPROPERTY(EditAnywhere, Category="Moonfall|Camera", meta=(ClampMin="250.0", ClampMax="850.0")) float CameraDistance = 450.f;
 UPROPERTY(EditAnywhere, Category="Moonfall|Combat") float ManaRegenPerSecond = 8.f;
 UPROPERTY(EditAnywhere, Category="Moonfall|Combat") float DodgeCooldown = 1.3f;
 void MoveForward(float Amount);
 void MoveRight(float Amount);
 void Turn(float Amount);
 void LookUp(float Amount);
 void ZoomCameraIn();
 void ZoomCameraOut();
 void ZoomMinimapIn();
 void ZoomMinimapOut();
 void ToggleWorldMap();
 void RecenterWorldMap();
 void PlacePersonalWaypoint();
 void PlacePersonalWaypointAtCursor();
 void ClearPersonalWaypoint();
 void SavePersonalWaypoint();
 void CastLeft();
 void CastRight();
 void CastBoth();
 void Dodge();
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
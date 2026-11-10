#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MoonfallHeroPawn.generated.h"
class USpringArmComponent;
class UCameraComponent;
class UAnimMontage;
class AMoonfallEnemy;
class UStaticMeshComponent;
class UPointLightComponent;
UCLASS()
class MOONFALLUNREAL_API AMoonfallHeroPawn : public ACharacter
{
 GENERATED_BODY()
public:
 AMoonfallHeroPawn();
 virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
 virtual void BeginPlay() override;
 virtual void Tick(float DeltaSeconds) override;
 // Original Moonfall set themes use existing Force/Ward/Focus traits
 // on three equipped slots. No changes to encoded item IDs or saved gear.
 UFUNCTION(BlueprintPure, Category="Moonfall|Equipment|Sets") int32 GetSetPieceCount(int32 Affix) const;
 // Derived primary attributes, no additional persistent save schema.
 UFUNCTION(BlueprintPure, Category="Moonfall|Equipment|Stats") int32 GetMight() const;
 UFUNCTION(BlueprintPure, Category="Moonfall|Equipment|Stats") int32 GetInsight() const;
 UFUNCTION(BlueprintPure, Category="Moonfall|Equipment|Stats") int32 GetResolve() const;
 UFUNCTION(BlueprintPure, Category="Moonfall|Equipment|Stats") float GetCriticalChance() const;
 UFUNCTION(BlueprintPure, Category="Moonfall|Equipment|Stats") float GetHasteBonus() const;
 UFUNCTION(BlueprintPure, Category="Moonfall|Equipment|Stats") float GetMasteryBonus() const;
 UFUNCTION(BlueprintPure, Category="Moonfall|Equipment|Stats") float GetVersatilityBonus() const;
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
 // Every takedown plays native five-style choreography; authored montages layer over it.
 UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Moonfall|Finishers") TArray<TObjectPtr<UAnimMontage>> FinisherMontages;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Finishers", meta=(ClampMin="0.5", ClampMax="10.0")) float FinisherChainWindowSeconds = 4.f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Finishers") int32 FinisherChainCount = 0;
 // Total kills in the current uninterrupted chain, independent of the capped
 // five-stage intensity. Styles rotate rather than repeating stage five.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Finishers") int32 FinisherSequenceCount = 0;
 // Finisher momentum restores combat resources and briefly strengthens offensive
 // spells. Health is never restored by a kill.
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Finishers", meta=(ClampMin="0.0", ClampMax="25.0")) float FinisherStaminaRestore = 8.f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Finishers", meta=(ClampMin="0.0", ClampMax="25.0")) float FinisherManaRestore = 6.f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Finishers", meta=(ClampMin="1.0", ClampMax="1.5")) float FinisherMomentumDamageMultiplier = 1.15f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Finishers", meta=(ClampMin="1.0", ClampMax="12.0")) float FinisherMomentumDurationSeconds = 6.f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Finishers") float FinisherMomentumUntil = -1.f;
 // Successful click attacks build a twelve-stage same-target combo with
 // bounded damage and posture milestones at hits 4/8/12. Misses, heals and
 // failed casts never advance the sequence.
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Combat", meta=(ClampMin="0.5", ClampMax="4.0")) float HitComboWindowSeconds = 2.f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Combat", meta=(ClampMin="0.0", ClampMax="0.12")) float HitComboDamagePerStage = 0.04f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Combat") int32 HitComboStage = 0;
 // The fourth connected hit adds posture pressure to THAT enemy only.
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Combat", meta=(ClampMin="0.0", ClampMax="30.0")) float HitComboFourthHitPosture = 12.f;
 // Optional authored animations, one slot per hit. Null slots keep gameplay working.
 // These do not replace or alter the original character art.
 UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Moonfall|Combat") TArray<TObjectPtr<UAnimMontage>> ComboMontages;
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Combat") void OnSuccessfulHitCombo(int32 Stage, bool bLeftHand);
  UFUNCTION(BlueprintCallable, Category="Moonfall|Finishers") int32 RegisterFinisherKill(bool bShatterKill, bool bBossKill, FVector DefeatedLocation, AMoonfallEnemy* DefeatedEnemy);
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Finishers") void OnFinisherChain(int32 ChainStep, bool bShatterKill, bool bBossKill);
 UFUNCTION(BlueprintCallable, Category="Moonfall|Progression") void AwardExperience(int32 Amount);
 UFUNCTION(BlueprintCallable, Category="Moonfall|Loot") void AwardEquipmentDrop(bool bEliteEnemy, bool bBossEnemy);
 // Drop equipment in the world; only an explicit pickup moves it to inventory.
 UFUNCTION(BlueprintCallable, Category="Moonfall|Loot") void SpawnGroundEquipmentDrop(bool bEliteEnemy, bool bBossEnemy, FVector DefeatedLocation);
 UFUNCTION(BlueprintCallable, Category="Moonfall|Loot") bool CollectGroundEquipmentCode(int32 ItemCode);
 UFUNCTION(BlueprintCallable, Category="Moonfall|Loot") void PickupNearestGroundLoot();
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Loot") int32 EquipmentTier = 0;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Loot") int32 Salvage = 0;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Loot") int32 LastLootRarity = 0;
 // Trait codes: 0 none, 1 force, 2 ward, 3 focus.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Loot") int32 LastLootAffix = 0;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Equipment") int32 WeaponAffix = 0;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Equipment") int32 ArmorAffix = 0;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Equipment") int32 AccessoryAffix = 0;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Loot") int32 LastLootTier = 0;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Loot") int32 LastLootSlot = 0;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Loot") int32 LastLootUpgrade = 0;
 // UE5 individual-item bag capacity, shared by rewards, pickups and HUD.
 static constexpr int32 MaxInventoryItems = 9999;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Loot") TArray<int32> InventoryItemCodes;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Inventory") bool bInventoryOpen = false;
 // J toggles this read-only UE5 Atlas overlay, separate from gear and options.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Map") bool bAtlasOpen = false;
 UFUNCTION(BlueprintCallable, Category="Moonfall|Map") void ToggleAtlas();
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Inventory") bool bCharacterStatsOpen = false;
 UFUNCTION(BlueprintCallable, Category="Moonfall|Inventory") void ToggleCharacterStats();
 // Controller A switches existing inventory pages, never closes the menu.
 void SwitchInventoryPage();
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Inventory") int32 InventorySelection = 0;
 // Transient top visible row; wheel scrolls the bag without selecting/equipping.
 int32 InventoryViewStart = 0;
 // 0 pickup order, 1 rarity/tier descending, 2 equipment slot then rarity.
 // Sorting reorders codes only; it never equips, salvages or destroys anything.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Inventory") int32 InventorySortMode = 0;
 UFUNCTION(BlueprintCallable, Category="Moonfall|Inventory") void CycleInventorySort();
 UFUNCTION(BlueprintCallable, Category="Moonfall|Inventory") void ToggleInventory();
 // Preserve right-click combat outside inventory while enabling pointer-driven loot management.
 void UpdateInventoryMouseMode();
 UFUNCTION(BlueprintCallable, Category="Moonfall|Inventory") void EquipSelectedInventoryItem();
 // Salvage only un-equipped entries. Returns false for invalid/protected gear.
 UFUNCTION(BlueprintCallable, Category="Moonfall|Inventory") bool SalvageInventoryItem(int32 InventoryIndex);
 UFUNCTION(BlueprintCallable, Category="Moonfall|Inventory") void SalvageSelectedInventoryItem();
 // Spend salvage to upgrade selected gear, including currently equipped pieces. Tier 25 cap.
 UFUNCTION(BlueprintCallable, Category="Moonfall|Crafting") bool UpgradeInventoryItem(int32 InventoryIndex);
 UFUNCTION(BlueprintCallable, Category="Moonfall|Crafting") void UpgradeSelectedInventoryItem();
 // One deliberate press applies up to five existing safe upgrades, stopping
 // on insufficient salvage or tier 25 without deleting any gear.
 UFUNCTION(BlueprintCallable, Category="Moonfall|Crafting") int32 ForgeInventoryItemLevels(int32 InventoryIndex, int32 MaxLevels);
 void ForgeFiveSelectedInventoryItem();
 // Raise rarity one rank through explicit salvage crafting; worn stats update.
 UFUNCTION(BlueprintCallable, Category="Moonfall|Crafting") bool AscendInventoryItemRarity(int32 InventoryIndex);
 UFUNCTION(BlueprintCallable, Category="Moonfall|Crafting") void AscendSelectedInventoryItem();
 // Reforge a rare-or-better item to a different Force/Ward/Focus affix.
 UFUNCTION(BlueprintCallable, Category="Moonfall|Crafting") bool ReforgeInventoryItemAffix(int32 InventoryIndex);
 void ReforgeSelectedInventoryItem();
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Crafting") void OnEquipmentReforged(int32 NewItemCode, int32 SalvageSpent);
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Crafting") void OnEquipmentAscended(int32 NewItemCode, int32 SalvageSpent);
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Crafting") void OnEquipmentCrafted(int32 NewItemCode, int32 SalvageSpent);
 // Equip a validated inventory entry by index; the item remains in inventory.
 UFUNCTION(BlueprintCallable, Category="Moonfall|Equipment") bool EquipInventoryItem(int32 InventoryIndex);
 // Explicit inventory action, never automatic on loot pickup.
 // Lightweight local gear advisor: evaluates worn gear, item quality and
 // Force/Ward/Focus set synergy. It never equips, salvages or changes saves.
 // Returns -1 if there is no strictly better inventory choice.
 UFUNCTION(BlueprintPure, Category="Moonfall|Equipment|Advisor") int32 GetRecommendedEquipmentIndex() const;
 // A positive score means the specified bag item improves its own worn slot.
 // Zero or negative means keep the current gear; invalid indices return 0.
 UFUNCTION(BlueprintPure, Category="Moonfall|Equipment|Advisor") int32 GetEquipmentUpgradeScore(int32 InventoryIndex) const;
 UFUNCTION(BlueprintCallable, Category="Moonfall|Equipment") int32 EquipBestInventoryUpgrades();
 void EquipBestInventoryAction();

 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Equipment") void OnBestLoadoutEquipped(int32 UpgradesEquipped);
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Equipment") int32 EquippedWeaponCode = 0;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Equipment") int32 EquippedArmorCode = 0;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Equipment") int32 EquippedAccessoryCode = 0;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Equipment") int32 WeaponTier = 0;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Equipment") int32 ArmorTier = 0;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Equipment") int32 AccessoryTier = 0;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Combat") int32 LeftSpell = 0;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Combat") int32 RightSpell = 1;
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Combat") void OnSpellCast(int32 School, bool bLeftHand);
private:
 UPROPERTY(VisibleAnywhere) USpringArmComponent* CameraBoom;
 UPROPERTY(VisibleAnywhere) UCameraComponent* FollowCamera;
 float LastFinisherKillAt = -100.f;
 float NativeFinisherStartedAt = -100.f;
 FVector NativeFinisherTarget = FVector::ZeroVector;
 FVector NativeFinisherOrigin = FVector::ZeroVector;
 FVector NativeFinisherForward = FVector::ForwardVector;
 int32 NativeFinisherStyle = 1;
 bool bNativeFinisherShatter = false;
 bool bNativeFinisherBoss = false;
 // Bounded queue keeps nearby rapid kills from erasing unfinished choreography.
 struct FPendingFinisher {
  FVector Target = FVector::ZeroVector;
  TWeakObjectPtr<AMoonfallEnemy> Enemy;
  int32 Style = 1;
  bool bShatter = false;
  bool bBoss = false;
 };
 TArray<FPendingFinisher> PendingFinishers;
 static constexpr int32 MaxPendingFinishers = 8;
 void StartFinisherPresentation(int32 ChainStep, bool bShatterKill, bool bBossKill, const FVector& DefeatedLocation, AMoonfallEnemy* DefeatedEnemy);
 UPROPERTY(VisibleAnywhere, Category="Moonfall|Finishers") UStaticMeshComponent* FinisherSlashA = nullptr;
 UPROPERTY(VisibleAnywhere, Category="Moonfall|Finishers") UStaticMeshComponent* FinisherSlashB = nullptr;
 UPROPERTY(VisibleAnywhere, Category="Moonfall|Finishers") UPointLightComponent* FinisherFlash = nullptr;
 void UpdateNativeFinisher(float DeltaSeconds);
 TWeakObjectPtr<AMoonfallEnemy> LastHitComboTarget;
 float LastSuccessfulHitAt = -100.f;
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
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Combat", meta=(ClampMin="0.0", ClampMax="100.0")) float ParryCounterPostureDamage = 15.f;
 // Different offensive schools can pressure enemy stance differently.
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Combat", meta=(ClampMin="0.0", ClampMax="3.0")) float OffensiveSpellPostureMultiplier = 1.f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Combat") float ParryCounterUntil = -1.f;
 UPROPERTY(EditAnywhere, Category="Moonfall|Combat", meta=(ClampMin="0.0", ClampMax="3.0")) float GuardBreakRecoverySeconds = 1.0f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Movement", meta=(ClampMin="0.2", ClampMax="1.0")) float GuardMoveSpeedMultiplier = 0.65f;
 float DamageImmunityUntil = 0.f;
 float PerfectDodgeWindowUntil = -1.f;
 float LastPerfectDodgeRewardAt = -100.f;
 float ImpactCameraOffset = 0.f;
 // Gameplay mouse wheel zoom; does not alter the minimap or combat HUD.
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Camera", meta=(ClampMin="250.0", ClampMax="850.0")) float CameraDistance = 450.f;
 UPROPERTY(EditAnywhere, Category="Moonfall|Combat") float ManaRegenPerSecond = 8.f;
  UPROPERTY(EditAnywhere, Category="Moonfall|Combat", meta=(ClampMin="0.0", ClampMax="20.0")) float MenuHealthRegenPerSecond = 2.f;
  UPROPERTY(EditAnywhere, Category="Moonfall|Combat", meta=(ClampMin="1.0", ClampMax="2.0")) float MenuRecoveryMultiplier = 1.5f;
  UPROPERTY(EditAnywhere, Category="Moonfall|Combat", meta=(ClampMin="0.0", ClampMax="10.0")) float MenuHealthRegenDelaySeconds = 3.f;
  float MenuHealthRegenStartsAt = 0.f;
  UPROPERTY(EditAnywhere, Category="Moonfall|Combat", meta=(ClampMin="0.0", ClampMax="12.0")) float CombatRecoverySeconds = 5.f;
  UPROPERTY(EditAnywhere, Category="Moonfall|Combat", meta=(ClampMin="0.0", ClampMax="1.0")) float CombatManaRegenMultiplier = 0.5f;
  UPROPERTY(EditAnywhere, Category="Moonfall|Combat", meta=(ClampMin="0.0", ClampMax="15.0")) float PerfectParryManaReward = 5.f;
  float CombatRecoveryEndsAt = 0.f;
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
 void TryJump();
 void MoveForward(float Amount);
 void MoveRight(float Amount);
 void Turn(float Amount);
 void LookUp(float Amount);
 void ZoomCameraIn();
 void ZoomCameraOut();
 void CastLeft();
 // Left mouse selects pointed gear in inventory, casts during combat.
 void LeftClickContextAction();
 void ReleaseLeftClickContext();
 void CastRight();
 // Mouse-specific context action: equip inventory row or cast outside menus.
 void RightClickContextAction();
 void ReleaseRightClickContext();
 void CastBoth();
 // Transient mouse chord state, never serialized to saves.
 bool bLeftMouseHeld = false;
 bool bRightMouseHeld = false;
 bool bMouseChordConsumed = false;
 void ResetMouseCastChord();
 void Dodge();
 void BeginSprint();
 void EndSprint();
 void BreakGuard();
 void BeginGuard();
 void EndGuard();
 void InventorySalvage();
 void OpenOptions();
 void OptionsUp();
 void OptionsDown();
 void OptionsLeft();
 void OptionsRight();
 void AdjustOption(int32 Direction);
 void CycleGraphicsQuality();
 void ToggleDisplayMode();
 int32 RollEquipmentCode(bool bEliteEnemy, bool bBossEnemy);
 bool TryCast(int32 School, int32 Hand, float Multiplier, bool bOvercharged = false);
};
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"
#include "MoonfallRewardSaveGame.h"
#include "MoonfallHeroPawn.generated.h"
class USpringArmComponent;
class UCameraComponent;
class UAnimMontage;
class AMoonfallEnemy;
class AMoonfallKeyPickup;
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
 // Original Moonfall twilight cycle. Time passes only while playing, not in menus.
 // Dawn favors weapon strikes; night empowers offensive spells.
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Twilight",
   meta=(ClampMin="60.0", ClampMax="3600.0")) float TwilightCycleSeconds = 1440.f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Twilight") float TwilightElapsedSeconds = 0.f;
 UFUNCTION(BlueprintPure, Category="Moonfall|Twilight") bool IsTwilightNight() const;
 UFUNCTION(BlueprintPure, Category="Moonfall|Twilight") float GetTwilightWeaponMultiplier() const;
 UFUNCTION(BlueprintPure, Category="Moonfall|Twilight") float GetTwilightSpellMultiplier() const;
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
 // In-session talent allocation; earned points = Level - 1.
 // Persistent talent ranks will follow the versioned progression save migration.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Talents") int32 BladeTalentRank = 0;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Talents") int32 ArcaneTalentRank = 0;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Talents") int32 TwinStrikeTalentRank = 0;
 UFUNCTION(BlueprintPure, Category="Moonfall|Talents") int32 GetUnspentTalentPoints() const;
 UFUNCTION(BlueprintCallable, Category="Moonfall|Talents") bool SpendTalentPoint(int32 TalentIndex);
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Combat") bool bDualWieldStance = false;
 // Paired-strike mode uses the existing main weapon until off-hand equipment is integrated.
 void StrikeWithOffhandWeapon();
 // Godot reward parity: currency auto-awards on defeat, physical keys and
 // equipment require an explicit interaction, with persistent door progression.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Rewards") int64 Gold = 0;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Rewards") int64 ConfirmedRewardKills = 0;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Rewards") TArray<FName> CollectedKeys;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Rewards") TArray<FName> CompletedFactionQuests;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Rewards") TArray<FName> UnlockedGateIds;
 // Protect an unreadable or future-schema save from accidental overwrite.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Rewards") bool bRewardSaveLoadFailed = false;
 // Set after first load attempt: gates may reuse validated hero progress.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Rewards") bool bRewardSaveInitialized = false;
 UFUNCTION(BlueprintPure, Category="Moonfall|Rewards") bool HasUnlockedGate(FName GateId) const;
 UFUNCTION(BlueprintCallable, Category="Moonfall|Rewards") bool CommitGateUnlock(FName GateId, FName RequiredKeyId);
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Rewards") bool bKeyRingExpanded = false;
 // Transient inventory popover; never persisted as character progress.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Rewards") bool bKeyRingInventoryOpen = false;
 // HUD notices update only after deliberate key/door interactions, no polling.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Rewards") FString KeyInteractionNotice;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Rewards") float KeyInteractionNoticeUntil = -1.f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Rewards") bool bKeyInteractionNoticeError = false;
 UFUNCTION(BlueprintCallable, Category="Moonfall|Rewards") bool CompleteFactionQuest(FName QuestId);
 UFUNCTION(BlueprintCallable, Category="Moonfall|Rewards") void ToggleKeyRing();
 UFUNCTION(BlueprintCallable, Category="Moonfall|Rewards") int64 AddGold(int64 Amount);
 UFUNCTION(BlueprintCallable, Category="Moonfall|Rewards") bool CollectQuestKey(FName KeyId);
 UFUNCTION(BlueprintPure, Category="Moonfall|Rewards") bool HasQuestKey(FName KeyId) const;
 UFUNCTION(BlueprintCallable, Category="Moonfall|Rewards") void AwardEnemyCurrency(bool bEliteEnemy, bool bBossEnemy, FVector DefeatedLocation);
 UFUNCTION(BlueprintCallable, Category="Moonfall|Rewards") bool SaveRewardProgress();
 UFUNCTION(BlueprintCallable, Category="Moonfall|Rewards") bool LoadRewardProgress();
 UFUNCTION(BlueprintCallable, Category="Moonfall|Loot") void AwardEquipmentDrop(bool bEliteEnemy, bool bBossEnemy);
 // Drop equipment in the world; only an explicit pickup moves it to inventory.
 UFUNCTION(BlueprintCallable, Category="Moonfall|Loot") void SpawnGroundEquipmentDrop(bool bEliteEnemy, bool bBossEnemy, FVector DefeatedLocation);
 UFUNCTION(BlueprintCallable, Category="Moonfall|Loot") bool CollectGroundEquipmentCode(int32 ItemCode);
 // Transactional pickup: removes the saved world-drop ID with the bag grant.
 bool CollectPersistedGroundEquipmentCode(int32 ItemCode, const FGuid& DropId);
 UFUNCTION(BlueprintCallable, Category="Moonfall|Loot") void PickupNearestGroundLoot();
 // The interaction key remains for quest keys and gates; equipment is automatic.
 void InteractWithNearbyWorld();
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
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Loot") TArray<int32> InventoryItemCodes;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Inventory") bool bInventoryOpen = false;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Inventory") bool bCharacterStatsOpen = false;
 UFUNCTION(BlueprintCallable, Category="Moonfall|Inventory") void ToggleCharacterStats();
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Inventory") int32 InventorySelection = 0;
 UFUNCTION(BlueprintCallable, Category="Moonfall|Inventory") void ToggleInventory();
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
 // V switches between spell + spell and weapon + spell, with no save-schema change.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Combat") bool bWeaponSpellStance = false;
 // Shield-style guard shares the existing stamina, parry and frontal block rules.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Combat") bool bSpellShieldStance = false;
 UFUNCTION(BlueprintCallable, Category="Moonfall|Inventory") void CycleHandSetup();
 UFUNCTION(BlueprintCallable, Category="Moonfall|Inventory") void SelectHandSetup(int32 Setup);
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Combat") void OnWeaponStrike(float Damage, bool bConnected);
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Combat") void OnSpellCast(int32 School, bool bLeftHand);
 // Optional authored animation/VFX hook: no replacement character art needed.
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Combat") void OnDualCastReleased(int32 School);
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
 // Mouse chord inputs are separate from direct keyboard/controller casts.
 FTimerHandle MouseCastTimer;
 int32 PendingMouseHand = -1;
 bool bMouseLeftHeld = false;
 bool bMouseRightHeld = false;
 bool bMouseChordConsumed = false;
 void QueueMouseCast(int32 Hand);
 void CommitPendingMouseCast();
 void CancelPendingMouseCombat();
 void MouseLeftPressed();
 void MouseLeftReleased();
 void MouseRightPressed();
 void MouseRightReleased();
 void CastLeft();
 void CastRight();
 void CastBoth();
 void ToggleWeaponSpell();
 void StrikeWithWeapon();
 float NextWeaponStrikeAt = 0.f;
 float NextOffhandStrikeAt = 0.f;
 void Dodge();
 void BeginSprint();
 void EndSprint();
 void BreakGuard();
 void BeginGuard();
 void EndGuard();
 void UpdateInventoryMouseInput();
 bool TryInteractWithNearbyKey();
 // Weak reference does not keep a collected/destroyed key alive.
 TWeakObjectPtr<AMoonfallKeyPickup> PendingMilestoneKey;
 // Bounded persistent world drops share the reward save transaction.
 UPROPERTY() TArray<FMoonfallWorldLootRecord> PendingWorldLoot;
 bool RegisterWorldEquipmentDrops(const TArray<FMoonfallWorldLootRecord>& NewDrops);
 void RestorePendingWorldEquipmentDrops();
 // Auto-loot runs on a short interval; never scan every rendered frame.
 float NextAutoLootAt = 0.f;
 // Distinguish a failed save from an unreachable/stale ground actor.
 bool bAutoLootSaveFailedThisSweep = false;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Loot") bool bAutoLootEnabled = true;
 void SpawnMilestoneKey(const FVector& Location);
 // During crafting, equipped updates participate in the outer save transaction.
 bool bDeferEquippedSave = false;
 void InventoryConfirm();
 void InventorySalvage();
 void OpenOptions();
 void OptionsUp();
 void OptionsDown();
 void OptionsLeft();
 void OptionsRight();
 void AdjustOption(int32 Direction);
 void CycleGraphicsQuality();
 void ToggleDisplayMode();
 // Optional category keeps a single encounter's first three drops varied.
 int32 RollEquipmentCode(bool bEliteEnemy, bool bBossEnemy, int32 PreferredSlot = INDEX_NONE);
 bool TryCast(int32 School, int32 Hand, float Multiplier);
};
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MoonfallPersistentSave.h"
#include "MoonfallHeroPawn.generated.h"
class USpringArmComponent;
class UCameraComponent;
class UNavigationInvokerComponent;
class UAnimMontage;
class AMoonfallEnemy;
class AMoonfallWorldNPC;
class AMoonfallCheckpointShrine;
class UStaticMeshComponent;
class UPointLightComponent;
UCLASS()
class MOONFALLUNREAL_API AMoonfallHeroPawn : public ACharacter
{
 GENERATED_BODY()
public:
 AMoonfallHeroPawn();
 // Runtime navigation generated near the player, not over the full 30-dungeon
 // world at startup. Requires Dynamic Recast and level NavMeshBoundsVolumes.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Dungeon|Navigation")
 UNavigationInvokerComponent* DungeonNavigationInvoker;
 virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
 virtual void BeginPlay() override;
 virtual void Tick(float DeltaSeconds) override;
 // One-shot, save-safe placement for arenas spawned after hero BeginPlay.
 void TryPlaceNewCharacterAtSanctuary();
 bool bStartingSanctuaryPlaced = false;
 // Cache the authored sanctuary world position once on load, not per HUD frame.
 void CacheStartingSanctuaryAnchor();
 bool bHasStartingSanctuaryAnchor = false;
 FVector StartingSanctuaryAnchor = FVector::ZeroVector;
 bool IsNearStartingSanctuary() const;
 // Original Moonfall set themes use existing Force/Ward/Focus traits
 // on three equipped slots. No changes to encoded item IDs or saved gear.
 // Twelve persistent fantasy ancestries. Choice locks on first selection.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Race") int32 RaceId = 0;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Race") bool bRaceChosen = false;
 UFUNCTION(BlueprintCallable, Category="Moonfall|Race") bool ChooseRace(int32 NewRaceId);
 UFUNCTION(BlueprintPure, Category="Moonfall|Race") FString GetRaceName() const;
 UFUNCTION(BlueprintPure, Category="Moonfall|Race") int32 GetRaceMightBonus() const;
 UFUNCTION(BlueprintPure, Category="Moonfall|Race") int32 GetRaceInsightBonus() const;
 UFUNCTION(BlueprintPure, Category="Moonfall|Race") int32 GetRaceResolveBonus() const;
 UFUNCTION(BlueprintPure, Category="Moonfall|Race") float GetRaceStaminaRegenBonus() const;
 UFUNCTION(BlueprintPure, Category="Moonfall|Race") float GetRaceManaRegenBonus() const;
 // Each ancestry receives a distinct on-demand combat power; no artwork changes.
 // Target-focused context interaction: RMB on NPC; pad D-pad up near NPC.
 // Choice buttons are implemented by UI using ChooseResidentContextAction.
 UFUNCTION(BlueprintCallable, Category="Moonfall|NPC") void OpenFocusedResidentContext();
 UFUNCTION(BlueprintCallable, Category="Moonfall|NPC") void CastRightMouse();
 UFUNCTION(BlueprintCallable, Category="Moonfall|NPC") bool ChooseResidentContextAction(int32 Choice);
 UFUNCTION(BlueprintCallable, Category="Moonfall|NPC") void CloseResidentContext();
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|NPC") void OnResidentContextOpened(AMoonfallWorldNPC* Resident, FText ResidentName, int32 QuestState, int32 SalvageCost);
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|NPC") void OnResidentContextClosed();
 UFUNCTION(BlueprintCallable, Category="Moonfall|NPC") void TalkToNearestNPC();
 UFUNCTION(BlueprintCallable, Category="Moonfall|NPC|Quest") void InteractWithNearestResidentQuest();
 UFUNCTION(BlueprintPure, Category="Moonfall|NPC") AMoonfallWorldNPC* GetNearestResidentNPC() const;
 UPROPERTY(Transient) TWeakObjectPtr<AMoonfallWorldNPC> FocusedResident;
 bool bResidentContextOpen = false;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|NPC") int32 ResidentContextSelection = 0;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|NPC") bool bResidentTurnInPending = false;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|NPC") float ResidentTurnInConfirmUntil = 0.f;
 float ResidentTurnInConfirmAfter = 0.f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|NPC") FString ResidentFeedbackMessage;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|NPC") float ResidentFeedbackUntil = 0.f;
 // Mouse click is context-sensitive: inventory/menus first, otherwise combat.
 void ResidentContextMouseClick();
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|NPC|Quest") void OnResidentQuestFeedback(FText Message, bool bSuccess);

 // Reputation is a permanent value for recognized world factions.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Faction") TMap<FName, int32> FactionReputation;
 UFUNCTION(BlueprintPure, Category="Moonfall|Faction") int32 GetFactionReputation(FName Faction) const;
 UFUNCTION(BlueprintCallable, Category="Moonfall|Faction") bool ChangeFactionReputation(FName Faction, int32 Delta);
 // One save-slot relationship per authored resident ID, never per spawned actor instance.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|NPC|Bond") TMap<FName, int32> ResidentBonds;
 UFUNCTION(BlueprintPure, Category="Moonfall|NPC|Bond") int32 GetResidentBond(FName ResidentId) const;
 UFUNCTION(BlueprintCallable, Category="Moonfall|NPC|Bond") bool ChangeResidentBond(FName ResidentId, int32 Delta);
 // One-shot racial resident contracts: 0 unavailable, 1 accepted, 2 completed.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|NPC|Quest") TMap<FName, int32> ResidentQuestStates;
 UFUNCTION(BlueprintPure, Category="Moonfall|NPC|Quest") int32 GetResidentQuestState(FName ResidentId) const;
 UFUNCTION(BlueprintCallable, Category="Moonfall|NPC|Quest") bool AcceptResidentQuest(FName ResidentId);
 UFUNCTION(BlueprintCallable, Category="Moonfall|NPC|Quest") bool CompleteResidentQuest(AMoonfallWorldNPC* Resident);



 UFUNCTION(BlueprintCallable, Category="Moonfall|Race") bool ActivateRacePower();
 void TriggerRacePower() { ActivateRacePower(); }
 UFUNCTION(BlueprintPure, Category="Moonfall|Race") FString GetRacePowerName() const;
 UFUNCTION(BlueprintPure, Category="Moonfall|Race") float GetRacePowerCooldownRemaining() const;
 UFUNCTION(BlueprintPure, Category="Moonfall|Race") float GetRacePowerDurationRemaining() const;
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Race") void OnRacePowerActivated(FName PowerName);

 UFUNCTION(BlueprintPure, Category="Moonfall|Equipment|Sets") int32 GetSetPieceCount(int32 Affix) const;
 UFUNCTION(BlueprintPure, Category="Moonfall|Equipment|Stats") int32 GetValidatedEquippedTier(int32 Slot) const;
 // Display-only comparison score, never used for combat or save data.
 UFUNCTION(BlueprintPure, Category="Moonfall|Equipment|Stats") int32 GetItemPower(int32 ItemCode) const;
 UFUNCTION(BlueprintPure, Category="Moonfall|Equipment|Stats") int32 GetEquipmentPower() const;
 // Perks activate only on owned, correctly slotted legendary gear.
 UFUNCTION(BlueprintPure, Category="Moonfall|Equipment|Perks") bool HasLegendaryPerk(int32 Slot) const;
 // Read-only owned-item comparison and projected set totals.
 UFUNCTION(BlueprintPure, Category="Moonfall|Equipment|Stats") int32 GetItemPowerDelta(int32 ItemCode) const;
 UFUNCTION(BlueprintPure, Category="Moonfall|Equipment|Sets") int32 GetProjectedSetPieceCount(int32 Affix, int32 CandidateCode) const;
 // Derived primary attributes, no additional persistent save schema.
 UFUNCTION(BlueprintPure, Category="Moonfall|Equipment|Stats") int32 GetMight() const;
 UFUNCTION(BlueprintPure, Category="Moonfall|Equipment|Stats") int32 GetInsight() const;
 UFUNCTION(BlueprintPure, Category="Moonfall|Equipment|Stats") int32 GetResolve() const;
 UFUNCTION(BlueprintPure, Category="Moonfall|Equipment|Stats") float GetCriticalChance() const;
 UFUNCTION(BlueprintPure, Category="Moonfall|Equipment|Stats") float GetHasteBonus() const;
 UFUNCTION(BlueprintPure, Category="Moonfall|Equipment|Stats") float GetMasteryBonus() const;
 UFUNCTION(BlueprintPure, Category="Moonfall|Equipment|Stats") float GetVersatilityBonus() const;
 // Version 8: 30 stable dungeon entrances, six zones x five per zone.
 // Discovery is permanent, independent of shrine respawn progression.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Dungeon") int32 DiscoveredDungeonMask = 0;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Dungeon|Boss")
 int32 DefeatedDungeonBossMask = 0;
 UFUNCTION(BlueprintPure, Category="Moonfall|Dungeon|Boss")
 bool HasDefeatedDungeonBoss(int32 Zone, int32 Dungeon) const;
 // Internal kill-path API only: never allow a generic Blueprint event
 // to persist a victory without the real dungeon boss death callback.
 bool MarkDungeonBossDefeated(AMoonfallEnemy* DefeatedBoss);
 // Permanent WoW-style zone/campaign completion counters (not discovery).
 UFUNCTION(BlueprintPure, Category="Moonfall|Dungeon|Boss")
 int32 GetDefeatedDungeonBossCount(int32 Zone) const;
 UFUNCTION(BlueprintPure, Category="Moonfall|Dungeon|Boss")
 int32 GetTotalDefeatedDungeonBosses() const;
 UFUNCTION(BlueprintPure, Category="Moonfall|Dungeon|Boss")
 int32 GetClearedDungeonZoneCount() const;
 // These fire only AFTER victory + its one-time salvage reward are saved.
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Dungeon|Boss")
 void OnDungeonBossVictory(int32 Zone, int32 Dungeon, int32 SalvageEarned);
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Dungeon|Boss")
 void OnDungeonZoneCleared(int32 Zone);
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Dungeon|Boss")
 void OnAllDungeonsCleared();
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Dungeon|Treasure") TArray<int32> DungeonTreasureStates;
 UFUNCTION(BlueprintPure, Category="Moonfall|Dungeon") bool HasDiscoveredDungeon(int32 Zone, int32 Dungeon) const;
 UFUNCTION(BlueprintPure, Category="Moonfall|Dungeon") int32 GetDiscoveredDungeonCount(int32 Zone) const;
 UFUNCTION(BlueprintPure, Category="Moonfall|Dungeon") int32 GetNextUnchartedDungeonZone() const;
 UFUNCTION(BlueprintPure, Category="Moonfall|Dungeon") int32 GetTotalDiscoveredDungeons() const;
 UFUNCTION(BlueprintCallable, Category="Moonfall|Dungeon|Treasure")
 int32 GetOrCreateDungeonTreasureCode(int32 Zone, int32 Dungeon, int32 Chamber, int32 LayoutSeed);
 UFUNCTION(BlueprintCallable, Category="Moonfall|Dungeon|Treasure")
 bool CollectDungeonTreasureCode(int32 ItemCode, int32 Zone, int32 Dungeon, int32 Chamber);
 UFUNCTION(BlueprintPure, Category="Moonfall|Dungeon|Treasure")
 int32 GetDungeonTreasureState(int32 Zone, int32 Dungeon, int32 Chamber) const;
 UFUNCTION(BlueprintCallable, Category="Moonfall|Dungeon") bool DiscoverDungeonEntrance(int32 Zone, int32 Dungeon);
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Dungeon")
 void OnDungeonDiscovered(int32 Zone, int32 Dungeon);
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Dungeon")
 void OnDungeonZoneCharted(int32 Zone);

 // Shrine discovery controls respawn. Regular two-second saves keep progress
 // without moving the active checkpoint to every room crossed.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Shrines") FName ActiveShrineId = NAME_None;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Shrines") TArray<FName> DiscoveredShrineIds;
 UFUNCTION(BlueprintPure, Category="Moonfall|Shrines") bool HasDiscoveredShrine(FName ShrineId) const;
 // A previously discovered shrine is not the active respawn point when the
 // saved shrine was blocked and recovery moved the checkpoint to PlayerStart.
 bool IsCurrentRespawnLocation(const FVector& Location) const;
 UFUNCTION(BlueprintCallable, Category="Moonfall|Shrines") bool DiscoverShrine(AMoonfallCheckpointShrine* Shrine);
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Shrines") void OnShrineDiscovered(FName ShrineId);
 // A single permanent world checkpoint. Death never erases experience or items.
 UFUNCTION(BlueprintCallable, Category="Moonfall|Persistence") bool SaveWorldProgress();
 UFUNCTION(BlueprintCallable, Category="Moonfall|Persistence") bool LoadWorldProgress();
 UFUNCTION(BlueprintCallable, Category="Moonfall|Persistence") void RespawnAtCheckpoint();
 UFUNCTION(BlueprintCallable, Category="Moonfall|Combat") void ApplyMoonfallDamage(float Amount);
 // Enemy strikes provide an origin so guard only catches frontal blows.
 bool ApplyMoonfallDamageFrom(float Amount, const FVector& SourceLocation);
 // Explicit dodge-only boss hit, preserving the normal attack API.
 bool ApplyMoonfallDodgeOnlyDamageFrom(float Amount, const FVector& SourceLocation);
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
 bool CollectPersistedGroundEquipmentCode(int32 ItemCode, const FGuid& DropId);
 UFUNCTION(BlueprintCallable, Category="Moonfall|Loot") void PickupNearestGroundLoot();
 UFUNCTION(BlueprintCallable, Category="Moonfall|Loot") void SetAutoLootEnabled(bool bEnabled);
 float AutoLootElapsed = 0.f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Loot") bool bAutoLootEnabled = true;
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
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Atlas") bool bAtlasOpen = false;
 // Transient atlas navigation, never written into save data.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Atlas") int32 AtlasSelectedRegion = 0;
 // Read-only lore navigation; these are never serialized with world progress.
 // -1 follows the region's automatically featured discovered chapter.
 UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Atlas|Chronicle") int32 AtlasChronicleChapter = -1;
 UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Atlas|Chronicle") bool bAtlasChronicleFocus = false;
 void ToggleAtlasChronicleFocus();
 void StepAtlasChronicle(int32 Direction);
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Atlas") float AtlasZoom = 1.f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Atlas") float AtlasPanY = -2500.f;
 // Runtime-only shrine cache. Refresh once when the Atlas opens, never in DrawHUD.
 UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Atlas") TArray<FVector> AtlasDiscoveredShrineLocations;
 // Parallel names for the discovered-shrine cache. Never a persisted location leak.
 UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Atlas") TArray<FString> AtlasDiscoveredShrineNames;
 UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Atlas") FVector AtlasActiveRespawnShrineLocation = FVector::ZeroVector;
 UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Atlas") bool bAtlasHasActiveRespawnShrineLocation = false;
 // V12 permanent explored-world cells: six 8-by-7 grids, bounded to 56 bits each.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Atlas") TArray<int64> ExploredZoneFogMasks;
 UFUNCTION(BlueprintPure, Category="Moonfall|Atlas") bool HasExploredAtlasCell(int32 Zone, int32 Column, int32 Row) const;
 UFUNCTION(BlueprintCallable, Category="Moonfall|Atlas") void ToggleAtlas();
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Inventory") bool bInventoryOpen = false;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Inventory") bool bCharacterStatsOpen = false;
 UFUNCTION(BlueprintCallable, Category="Moonfall|Inventory") void ToggleCharacterStats();
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Inventory") int32 InventorySelection = 0;
 // Transient visible row offset; wheel scrolling never changes selection or equipment.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Inventory") int32 InventoryViewStart = 0;
 // Transient display mode: original order, rarity, then equipment slot.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Inventory") int32 InventorySortMode = 0;
 TArray<int32> GetInventoryDisplayOrder() const;
 bool HitTestInventorySortButton() const;
 bool HitTestInventoryEquipButton() const;
 // Pointer close button is independent of gear and sort hit regions.
 bool HitTestInventoryCloseButton() const;
 // Clickable scrollbar track changes only the transient display window.
 bool ScrollInventoryToMouseTrack();
 // Mouse drag is transient and cannot mutate inventory or equipped items.
 bool bInventoryScrollbarDragging = false;
 void ReleaseInventoryPointer();
 // Matches the Canvas inventory row capacity on the active viewport.
 int32 GetInventoryVisibleRows() const;
 void KeepInventorySelectionVisible();
 // Read-only pointer hit test sharing the inventory Canvas row geometry.
 int32 HitTestInventoryRow() const;
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
 // Read-only advisor from #899. Never equips, sorts, saves or mutates the bag.
 UFUNCTION(BlueprintPure, Category="Moonfall|Equipment") int32 GetEquipmentUpgradeScore(int32 InventoryIndex) const;
 UFUNCTION(BlueprintPure, Category="Moonfall|Equipment") int32 GetRecommendedEquipmentIndex() const;
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
 // Transient, read-only counter cue; not persisted in world saves.
 float GetParryCounterRemaining() const;
 // Transient camera lock: never touches saves or original hero art.
 UFUNCTION(BlueprintPure, Category="Moonfall|Combat|LockOn") AMoonfallEnemy* GetLockedEnemy() const;
 UFUNCTION(BlueprintCallable, Category="Moonfall|Combat|LockOn") void ToggleLockOn();
 void SwitchLockedTarget();
 bool IsLockTargetValid(const AMoonfallEnemy* Enemy) const;
 void AcquireLockTarget(int32 Direction);

private:
 // Fallback for levels without a usable PlayerStart. Capture the actual
 // authored spawn before the saved checkpoint is applied during BeginPlay.
 FTransform InitialSpawnTransform = FTransform::Identity;
 bool bHasInitialSpawnTransform = false;
 bool RestoreSafeSpawnLocation();
 FVector RespawnCheckpointLocation = FVector::ZeroVector;
 bool bHasRespawnCheckpointLocation = false;
 // Never write a shrine save before the initial persistent slot has loaded.
 bool bWorldProgressInitialized = false;
 // Mark the occupied XY cell; routine two-second autosave persists discovery.
 void RevealAtlasAtWorldLocation();
 // Collision-aware teleport during load can fire shrine overlaps before all
 // progress has been restored. Never allow nested discovery saves in this phase.
 bool bWorldProgressLoadInProgress = false;
 // Protect an existing unsupported or unreadable slot from routine autosaves.
 // Cleared only after a supported save has been loaded successfully.
 bool bPersistentSaveWritesBlocked = false;
 float SecondsSinceLastWorldSave = 0.f;
 float DefeatElapsedSeconds = 0.f;
 // Delay routine checkpoint overwrites after respawning into danger.
 float NextSafeCheckpointAt = 0.f;
 float RacePowerReadyAt = 0.f;
 float RacePowerActiveUntil = 0.f;
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
 // A short press near cooldown completion is consumed once, never during
 // dialogue, overlays or defeat. This is not persisted to checkpoint saves.
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Combat",
   meta=(ClampMin="0.0", ClampMax="0.18"))
 float DodgeInputBufferSeconds = 0.12f;
 float QueuedDodgeUntil = -1.f;
 TWeakObjectPtr<AMoonfallEnemy> LockedEnemy;
 float NextLockVisibilityCheckAt = 0.f;
 UPROPERTY(EditAnywhere, Category="Moonfall|Combat|LockOn", meta=(ClampMin="300.0", ClampMax="2500.0")) float LockRange = 1600.f;

 // Snapshot the world-space intent at the press. A buffered dodge must not
 // follow later stick movement or a target turn before cooldown finishes.
 FVector QueuedDodgeDirection = FVector::ZeroVector;
 bool bExecutingBufferedDodge = false;
 FVector IncomingDamageSource = FVector::ZeroVector;
 bool bHasIncomingDamageSource = false;
 bool bIncomingDamageDodgeOnly = false;
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
 void CastRight();
 void CastBoth();
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
 bool CollectGroundEquipmentCodeImpl(int32 ItemCode, int32 DungeonTreasureKey);
 TArray<FMoonfallWorldLootRecord> PendingWorldLoot;
 bool RegisterPersistentEquipmentDrops(const TArray<FMoonfallWorldLootRecord>& Drops);
 void RestorePendingWorldEquipmentDrops();
 bool TryCast(int32 School, int32 Hand, float Multiplier);
};
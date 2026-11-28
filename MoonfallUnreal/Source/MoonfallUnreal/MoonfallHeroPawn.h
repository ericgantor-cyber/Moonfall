#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MoonfallPersistentSave.h"
#include "MoonfallOmniattack.h"
#include "MoonfallCombatInputBuffer.h"
#include "MoonfallFirstlightTutorial.h"
#include "MoonfallSpellBook.h"
#include "MoonfallTalentRules.h"
#include "MoonfallPerformanceAudit.h"
#include "MoonfallBugBreadcrumbs.h"
#include "MoonfallQuestJournal.h"
#include "MoonfallGearAdvisorPolicy.h"
#include "MoonfallFishing.h"
#include "MoonfallMoney.h"
#include "MoonfallHeroPawn.generated.h"
class USpringArmComponent;
class UCameraComponent;
class USceneComponent;
class UNavigationInvokerComponent;
class UAnimMontage;
class USoundBase;
class AMoonfallEnemy;
class AMoonfallWorldNPC;
class AMoonfallCheckpointShrine;
class UStaticMeshComponent;
class UPointLightComponent;
class UNiagaraSystem;
class UNiagaraComponent;
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
 virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
 virtual void Tick(float DeltaSeconds) override;
 // Runtime water state; never serialized to character or checkpoint saves.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="Moonfall|Water")
 float Oxygen = 100.f;
 // Runtime-derived from validated equipment; shared with the HUD.
 float WaterBreathScale = 1.f;
 float WaterTonicRemaining = 0.f;
 float WaterSwimScale = 1.f;
 MoonfallFishing::FSession Fishing;
 FVector FishingOrigin=FVector::ZeroVector;
 FVector FishingBobber=FVector::ZeroVector;
 TWeakObjectPtr<class APhysicsVolume> FishingWaterVolume;
 bool IsFishingWaterValid(bool bCheckSight) const;
 float FishingNoticeUntil=0.f;
 bool bFishingCaught=false;
 MoonfallFishing::EEndNotice FishingEndNotice=MoonfallFishing::EEndNotice::None;
 void CancelFishingWithFeedback();
 // Optional authored audio assets. No implicit playback or asset load.
 UPROPERTY(EditDefaultsOnly, Category="Moonfall|Fishing") USoundBase* FishingCastSound=nullptr;
 UPROPERTY(EditDefaultsOnly, Category="Moonfall|Fishing") USoundBase* FishingBiteSound=nullptr;
 UPROPERTY(EditDefaultsOnly, Category="Moonfall|Fishing") USoundBase* FishingReelSound=nullptr;
 UPROPERTY(EditDefaultsOnly, Category="Moonfall|Fishing") USoundBase* FishingCatchSound=nullptr;
 UPROPERTY(BlueprintReadOnly, Transient, Category="Moonfall|Fishing") int32 FishingVisualStage=0;
 void SyncFishingPresentation(bool bCaught=false);
 int32 FishingSchoolId=-1;
 int32 FishingProvince=-1;
 int32 RegionalWaypoint=-1;
 FVector RegionalWaypointLocation=FVector::ZeroVector;
 uint32 RegionalWaypointAvailable=0;
 void RefreshRegionalWaypoints();
 void SelectRegionalWaypoint(int32 Id);
 void StepRegionalWaypoint(int32 Direction);
 bool HasRegionalWaypoint() const;
 bool bCharacterProfessionsPageOpen=false;
 int32 ProfessionScroll=0;
 void StepProfessionScroll(int32 Direction);
 bool HasWaterProfessionsPage() const;
 void StepCharacterPage(int32 Direction);
 TArray<FVector> NearbyFishingSchools;
 float NextFishingSchoolScan=0.f;
 void RefreshFishingSchools(float Now);
 bool CanContinueFishing() const;
 bool TryFishingAction(const FVector* RequestedWater=nullptr);
 bool TryMouseFishingAction();
 void FishingAction() { TryFishingAction(); }
 void GetWaterTraversalBonuses(float& OutBreath, float& OutSwim) const;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="Moonfall|Water")
 bool bWaterSwimming = false;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="Moonfall|Water")
 bool bWaterSubmerged = false;
 // Camera depth drives presentation independently of the hero's breathing.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="Moonfall|Water")
 float WaterCameraDepth = 0.f;
 // Eye depth for readable navigation, independent of camera placement.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="Moonfall|Water")
 float WaterBreathingDepth = 0.f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="Moonfall|Water")
 float WaterStrokeSpeed = 0.f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="Moonfall|Water")
 float WaterVerticalSpeed = 0.f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="Moonfall|Water")
 float WaterCurrentStrength = 0.f;
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Water")
 void OnSwimmingStateChanged(bool bSwimming, bool bSubmerged);
 // Optional presentation pivot for authored underwater effects. Never reparent
 // the original hero mesh or use this transform for hit detection.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Water")
 USceneComponent* SwimPoseAnchor=nullptr;
 void RefreshSwimPose();
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="Moonfall|Water")
 bool bInsideAirPocket = false;
 TArray<TWeakObjectPtr<class AMoonfallAirPocket>> NearbyAirPocketCandidates;
 FVector LastAirPocketQueryEye=FVector::ZeroVector;
 double LastAirPocketQueryTime=-1.0;
 // Advisory only; never grants breathing or moves the hero.
 TWeakObjectPtr<class AMoonfallAirPocket> GuidedAirPocket;
 double LastAirGuideTime=-1.0;
 void RefreshAirPocketGuide();
 bool GetAirPocketGuide(FVector& Direction,float& DistanceMeters) const;
 // Tap-to-toggle vertical swimming; no sustained key hold required.
 int32 WaterVerticalIntent = 0;
 bool CanSteerSwimming() const;
 // Preview-only shoreline recovery. The anchor is transient and captured
 // from a recent grounded position, never from a saved checkpoint.
 FVector WaterRecoveryShore=FVector::ZeroVector;
 double WaterRecoveryShoreAt=-1.0;
 TWeakObjectPtr<class APhysicsVolume> WaterRecoveryVolume;
 bool CanRequestEmergencySurface() const;
 UFUNCTION(BlueprintCallable, Category="Moonfall|Water")
 bool TryEmergencySurface();
 void EmergencySurfaceAction() { TryEmergencySurface(); }
 float DrowningExposureSeconds = 0.f;
 bool bApplyingDrowningDamage = false;
 // One-shot, save-safe placement for arenas spawned after hero BeginPlay.
 void TryPlaceNewCharacterAtSanctuary();
 bool bStartingSanctuaryPlaced = false;
 // Fresh-character, optional onboarding only; not persisted in world saves.
 MoonfallFirstlightTutorial::FProgress FirstlightTutorial;
 void RecordFirstlightAction(MoonfallFirstlightTutorial::EAction Action);
 // Cache the authored sanctuary world position once on load, not per HUD frame.
 void CacheStartingSanctuaryAnchor();
 bool bHasStartingSanctuaryAnchor = false;
 FVector StartingSanctuaryAnchor = FVector::ZeroVector;
 bool IsNearStartingSanctuary() const;
 bool IsRecentlyInCombatForAmbientAI() const;
 AMoonfallEnemy* GetRecentCompanionAssistTarget(float& Age) const;
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
 // Live 12-ancestry passives and active-power details for the C character sheet.
 // Read-only: no new saved IDs, no new ability cooldowns, no character art.
 UFUNCTION(BlueprintPure, Category="Moonfall|Race") FString GetRacePassiveSummary() const;
 UFUNCTION(BlueprintPure, Category="Moonfall|Race") FString GetRacePowerDescription() const;
 // Pre-confirmation read-only snapshot from the SAME 12 canonical runtime
 // profiles used by ChooseRace and actual Might/Insight/Resolve/regen.
 UFUNCTION(BlueprintPure, Category="Moonfall|Race")
 static FString PreviewGameplayAncestry(int32 CandidateRaceId);
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
 bool bResidentCompanionMenu=false;
 bool bResidentMerchantMenu=false;
 bool bMerchantSaleInProgress=false;
 int32 MerchantSellIndex=0;
 int32 MerchantSellCode=0;
 bool CanOpenCoinMerchant() const;
 void SelectMerchantSaleItem(int32 Direction);
 int64 GetMerchantSaleQuote() const;
 FString GetMerchantSaleItemText() const;
 bool SellMerchantItem();
 int32 MerchantTradeMode=0,MerchantStockIndex=0,MerchantStockLevel=1,MerchantBuybackIndex=0;
 TMap<FName,MoonfallMoney::FBuybackHistory> MerchantBuybackRecords;
 const MoonfallMoney::FBuybackHistory& GetMerchantBuybackHistory() const;
 int32 GetMerchantBuybackCode() const;
 int32 MerchantStockFaction=0,MerchantStockStanding=0;
 TArray<int64> MerchantRestockUtc={0,0,0,0};
 bool IsMerchantSpecialtySelected() const;
 int64 GetMerchantSpecialtyWait() const;
 int32 GetMerchantStockCount() const;
 FString GetMerchantStandingText() const;
 int32 GetMerchantStockCode() const;
 int64 GetMerchantTradeQuote() const;
 FString GetMerchantTradeItemText() const;
 FString GetMerchantTradeQuoteText() const;
 bool BuyMerchantItem();
 bool bMerchantConfirmPending=false;
 int32 MerchantConfirmMode=-1,MerchantConfirmCode=0,MerchantConfirmIndex=-1;
 int64 MerchantConfirmPrice=0;
 uint32 MerchantConfirmRevision=0;
 void ClearMerchantConfirmation();
 bool IsMerchantConfirmationCurrent() const;
 FString GetMerchantTradeStatusText() const;
 bool bResidentCompanionTalents=false;
 bool bResidentCompanionTraining=false;
 bool bResidentCompanionGear=false;
 int32 GetResidentContextRowCount() const;
 float GetResidentContextPanelHeight(float ViewportHeight) const;
 float GetResidentContextRowPitch(float PanelHeight) const;
 bool HasResidentContextPointerSurface(float ViewportWidth, float ViewportHeight) const;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|NPC") int32 ResidentContextSelection = 0;
 // -1 merchant, -2 recipe list, 0-2 recipe detail.
 int32 OceanRecipePage = -1;
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
 uint32 GetCompanionProfile(FName ResidentId) const;
 const FMoonfallCompanionPartyRecord& GetCompanionParty() const {return CompanionParty;}
 bool IsCompanionPartyRestoreReady() const {return bWorldProgressInitialized&&!bWorldProgressLoadInProgress&&!bPersistentSaveWritesBlocked;}
 bool CommitCompanionParty(const FMoonfallCompanionPartyRecord& Next);
 void UpdateCompanionVitals(FName ResidentId,float NewHealth,int64 AbilityReadyUtc);
 int32 GetCompanionGearCode(FName ResidentId,int32 Slot) const;
 int32 GetCompanionGearTier(FName ResidentId,int32 Slot) const;
 bool TradeCompanionGear(AMoonfallWorldNPC* Resident,int32 Slot,bool bReturn);
 bool ReportScoutDeeds(AMoonfallWorldNPC* Resident);
 // Internal authoritative transaction used by the companion GameMode APIs.
 bool CommitCompanionProfile(FName ResidentId,uint32 Profile);
 // One-shot racial resident contracts: 0 unavailable, 1 accepted, 2 completed.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|NPC|Quest") TMap<FName, int32> ResidentQuestStates;
 // Read-only quest eligibility shared by dialogue and Canvas/Blueprint UI.
 // Unknown designer-spawned residents can talk but cannot create unsaveable quests.
 UFUNCTION(BlueprintPure, Category="Moonfall|NPC|Quest") bool SupportsResidentContract(FName ResidentId) const;
 UFUNCTION(BlueprintPure, Category="Moonfall|NPC|Quest") int32 GetResidentQuestState(FName ResidentId) const;
 UFUNCTION(BlueprintCallable, Category="Moonfall|NPC|Quest") bool AcceptResidentQuest(FName ResidentId);
 UFUNCTION(BlueprintCallable, Category="Moonfall|NPC|Quest") bool CompleteResidentQuest(AMoonfallWorldNPC* Resident);



 UFUNCTION(BlueprintCallable, Category="Moonfall|Race") bool ActivateRacePower();
 // The already-bound controller Y button opens the spell book ONLY inside
 // inventory; gameplay still activates the same ancestry power.
 void TriggerRacePower() { if (bInventoryOpen) ToggleSpellBook(); else ActivateRacePower(); }
 UFUNCTION(BlueprintPure, Category="Moonfall|Race") FString GetRacePowerName() const;
 UFUNCTION(BlueprintPure, Category="Moonfall|Race") float GetRacePowerCooldownRemaining() const;
 UFUNCTION(BlueprintPure, Category="Moonfall|Race") float GetRacePowerDurationRemaining() const;
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Race") void OnRacePowerActivated(FName PowerName);

 UFUNCTION(BlueprintPure, Category="Moonfall|Equipment|Sets") int32 GetSetPieceCount(int32 Affix) const;
 UFUNCTION(BlueprintPure, Category="Moonfall|Equipment|Stats") int32 GetValidatedEquippedTier(int32 Slot) const;
 // Display-only validated worn code; unsupported slots remain empty.
 UFUNCTION(BlueprintPure, Category="Moonfall|Equipment") int32 GetEquipmentSlotCode(int32 Slot) const;
 // Display-only comparison score, never used for combat or save data.
 UFUNCTION(BlueprintPure, Category="Moonfall|Equipment|Stats") int32 GetItemPower(int32 ItemCode) const;
 UFUNCTION(BlueprintPure, Category="Moonfall|Equipment|Stats") int32 GetEquipmentPower() const;
 // Shared one-pass snapshot for HUD, previews and ring-stat consumers.
 void GetValidatedEquipmentCodes(TArray<int32>& OutCodes) const;
 uint64 GetEquipmentLoadoutFingerprint() const;
 // Read-only mutation token for presentation caches; no inventory access.
 uint32 GetEquipmentInventoryRevision() const { return EquipmentInventoryRevision; }
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
 UFUNCTION(BlueprintPure, Category="Moonfall|Equipment|Stats") float GetRingLuckBonus() const;
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
 // In the isolated Northern preview only, touching ANY previously discovered
 // island shrine restores resources without touching the campaign save.
 void RestAtDiscoveredPreviewShrine(AMoonfallCheckpointShrine* Shrine);
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Shrines") void OnShrineDiscovered(FName ShrineId);
 // A single permanent world checkpoint. Death never erases experience or items.
 UFUNCTION(BlueprintCallable, Category="Moonfall|Persistence") bool SaveWorldProgress();
 UFUNCTION(BlueprintCallable, Category="Moonfall|Persistence") bool LoadWorldProgress();
 UFUNCTION(BlueprintCallable, Category="Moonfall|Persistence") void RespawnAtCheckpoint();
 UFUNCTION(BlueprintCallable, Category="Moonfall|Combat") void ApplyMoonfallDamage(float Amount);
 // Enemy strikes provide an origin so guard only catches frontal blows.
 bool ApplyMoonfallDamageFrom(float Amount, const FVector& SourceLocation);
 bool ApplyDirectionalSwordDamageFrom(float Amount, const FVector& SourceLocation, int32 Angle);
 // Explicit dodge-only boss hit, preserving the normal attack API.
 bool ApplyMoonfallDodgeOnlyDamageFrom(float Amount, const FVector& SourceLocation);
 UFUNCTION(BlueprintCallable, Category="Moonfall|Combat") void RewardPerfectDodge();
 // Returns true only when this specific attack consumed the perfect-dodge window.
 bool TryRewardPerfectDodge();
 UFUNCTION(BlueprintPure, Category="Moonfall|Combat") float GetParryCounterRemaining() const;
 UPROPERTY(BlueprintReadOnly, Category="Moonfall|Combat") bool bGuarding = false;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Combat") float GuardBrokenUntil = 0.f;
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Combat") void OnGuardBroken();
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Combat") void OnGuardImpact(float DamageBlocked, bool bPerfectParry);
 // Optional directional guard animation; the existing guard input auto-matches.
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Combat") void OnDirectionalGuardImpact(int32 Angle, bool bPerfectParry);
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Combat") void OnHealthChanged(float NewHealth, float MaxHealth);
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Combat") void OnDamageImpact(float DamageTaken);
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Accessibility") bool bReduceMotion = false;
 // Optional authored systems indexed by the six existing spell schools.
 UPROPERTY(EditDefaultsOnly, Category="Moonfall|Combat|VFX")
 TArray<TObjectPtr<UNiagaraSystem>> SpellImpactSystems;
 UPROPERTY(EditDefaultsOnly, Category="Moonfall|Combat|VFX")
 bool bEnableAuthoredSpellVFX = true;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Accessibility") float CombatTextScale = 1.f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Accessibility") float BossWarningScale = 1.f;
 const MoonfallPerformance::FSnapshot& GetPerformanceAudit() const { return PerformanceAudit.Snapshot; }
 // Cached RHI-reported bytes; negative means unavailable.
 const MoonfallPerformance::FStageAudit& GetStageAudit() const { return StageAudit; }
 int64 GetAuditDedicatedVideoMemory() const { return AuditDedicatedVideoMemory; }
 int64 GetAuditTextureMemory() const { return AuditTextureMemory; }
 int64 GetAuditTexturePool() const { return AuditTexturePool; }
 const FString& GetBugReportStatus() const { return BugReportStatus; }
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
 // Canonical 141-node allocation. Persisted explicitly in V13 world saves.
 // Live talent bonuses are reconciled as validated resource deltas; saves
 // store base health capacity to avoid stacking rank bonuses on reload.
 MoonfallTalents::Ranks TalentRanks{};
 UFUNCTION(BlueprintPure, Category="Moonfall|Talents")
 int32 GetAvailableTalentPoints() const;
 UFUNCTION(BlueprintPure, Category="Moonfall|Talents")
 int32 GetTalentRank(int32 NodeIndex) const;
 UFUNCTION(BlueprintCallable, Category="Moonfall|Talents")
 bool SpendTalentPoint(int32 NodeIndex);
 UFUNCTION(BlueprintCallable, Category="Moonfall|Talents")
 bool RespecTalentPoints();
 // Shared atomic rank/resource update, with save rollback on failure.
 bool CommitTalentAllocation(const MoonfallTalents::Ranks& Candidate);
 // UI-only state. Never serialized: ranks are the single persistent authority.
 // Transient C-page state. The selection and all racial bonuses remain
 // owned by the canonical hero race, not by a second character UI catalog.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Race") bool bCharacterRacePageOpen = false;
 // Stats and equipped gear share the first read-only character page.
 // Saved faction standings are displayed on a third, read-only page.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Faction") bool bCharacterRepPageOpen = false;
 // Transient selected faction row, shared by mouse and controller; no save changes.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Faction") int32 CharacterRepSelection = 0;
 UFUNCTION(BlueprintCallable, Category="Moonfall|Faction") void SelectCharacterReputation(int32 FactionIndex);
 UFUNCTION(BlueprintCallable, Category="Moonfall|Race") void SelectCharacterPage(int32 Page);
 UFUNCTION(BlueprintCallable, Category="Moonfall|Race") void ToggleCharacterRacePage();
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Talents") bool bTalentWebOpen = false;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Talents") int32 TalentWebBranch = 0;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Talents") int32 TalentWebSelected = 0;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Talents") int32 TalentWebFirstTier = 1;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Talents") bool bTalentRespecArmed = false;
 UFUNCTION(BlueprintCallable, Category="Moonfall|Talents") void ToggleTalentWeb();
 void TalentSpendSelected();
 void TalentConfirmRespec();
 void TalentCloseWeb();
 void TalentNextBranch();
 void TalentPreviousBranch();
 void TalentMove(int32 TierDirection, int32 RowDirection);
 void TalentChangeBranch(int32 Direction);
 void TalentPointerClick();
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
 // Optional four angle-specific montages. Existing combo art remains the fallback.
 UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Moonfall|Combat") TArray<TObjectPtr<UAnimMontage>> SwordAngleMontages;
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Combat") void OnSuccessfulHitCombo(int32 Stage, bool bLeftHand);
 // A committed melee swing uses an automatic angle: high/right/low/left.
 // Optional Blueprint hook can play a montage or weapon trail for that angle.
 // Skyrim-inspired right-hand slot: weapon/unarmed or explicitly readied spell.
 // Defaults to weapon; right-click NPC interactions and spell hotkeys remain separate.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Combat|Hands") bool bRightHandSpellReadied = false;
 UFUNCTION(BlueprintCallable, Category="Moonfall|Combat|Hands") void RightHandAction();
  UFUNCTION(BlueprintCallable, Category="Moonfall|Combat|Hands") bool ReadyRightHandSpell(int32 School);
 UFUNCTION(BlueprintCallable, Category="Moonfall|Combat|Hands") void ReadyRightHandWeapon();
 // Favorites are equipped from Inventory, not a dedicated gameplay toggle.
 UFUNCTION(BlueprintCallable, Category="Moonfall|Combat|Hands") bool EquipRightHandFavorite(int32 FavoriteIndex);
 int32 HitTestRightHandFavorite() const;
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Combat|Hands") void OnRightHandLoadoutChanged(bool bUsingSpell, int32 SpellSchool);
 UFUNCTION(BlueprintCallable, Category="Moonfall|Combat") void MeleeAttack();
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Combat") void OnMeleeSwing(int32 Angle, bool bConnected);
 // Optional directional sword VFX/audio. No cue for misses or absorbed hits.
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Combat|Presentation")
 void OnMeleeImpact(int32 Angle, FVector ImpactPoint, float AppliedDamage,
   bool bRiposte, bool bKillingBlow);
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Combat") int32 CurrentSwordAngle = -1;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Combat") int32 SwordFlowStep = 0;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Combat", meta=(ClampMin="0.1", ClampMax="1.5")) float MeleeAttackCooldown = 0.48f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Combat", meta=(ClampMin="0.0", ClampMax="50.0")) float MeleeStaminaCost = 8.f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Combat", meta=(ClampMin="60.0", ClampMax="350.0")) float MeleeAttackRange = 225.f;
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
 bool bAutoLootDispatch = false;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Loot") bool bAutoLootEnabled = true;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Loot") int32 EquipmentTier = 0;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Loot") int32 Salvage = 0;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Economy") int64 WalletCopper=0;
 bool AwardEnemyCoins(int32 EnemyLevel,bool bElite,bool bBoss);
 FString GetWalletText() const;
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
 // Quest log reuses Atlas input and modal safety. Tracking preferences are session-only.
 bool bQuestLogOpen = false;
 bool bManualQuestTracking = false;
 MoonfallQuestJournal::FMask ManualQuestMask{};
 int32 QuestLogSelection = 0;
 MoonfallQuestJournal::FMask GetActiveQuestMask() const;
 MoonfallQuestJournal::FMask GetTrackedQuestMask() const;
 void ToggleQuestLog();
 void StepQuestLog(int32 Direction);
 void StepQuestLogPage(int32 Direction);
 void ToggleSelectedQuestTracking();
 void ResetQuestTracking();
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
 // Separate spell book page inside the existing B/Select inventory modal.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|SpellBook") bool bSpellBookOpen = false;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|SpellBook") int32 SpellBookSelection = 0;
 UFUNCTION(BlueprintCallable, Category="Moonfall|SpellBook") void ToggleSpellBook();
 // Direct P shortcut: opens the grimoire from gameplay, closes it on repeat.
 UFUNCTION(BlueprintCallable, Category="Moonfall|SpellBook") void ToggleSpellBookShortcut();
 UFUNCTION(BlueprintCallable, Category="Moonfall|SpellBook") bool EquipSpellBookSelection(bool bLeftHand);
 // Read-only per-hand status. Target/line of sight is verified by TryCast
 // only when actually casting, never by the UI every frame.
 MoonfallSpellBook::FCastState GetSpellBookCastState(int32 School, bool bLeftHand) const;
 bool HandleSpellBookPointer();

 UFUNCTION(BlueprintCallable, Category="Moonfall|Inventory") void ToggleCharacterStats();
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Inventory") int32 InventorySelection = 0;
 // Transient visible row offset; wheel scrolling never changes selection or equipment.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Inventory") int32 InventoryViewStart = 0;
 // Transient display mode: original order, rarity, then equipment slot.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Inventory") int32 InventorySortMode = 0;
 TArray<int32> GetInventoryDisplayOrder() const;
 bool HitTestInventorySortButton() const;
 bool HitTestInventoryEquipButton() const;
 int32 HitTestInventoryRingStrip() const;
 int32 GetFocusedRingIndex() const;
 int32 GetRingNavigationSwapSource() const;
 bool IsRingNavigationSwapInterrupted() const;
 bool HasRingNavigationFocus() const;
 int32 GetRingPointerDragCode() const;
 int32 GetRingPointerDragFinger() const { return RingDragFingerIndex; }
 bool CanDropDraggedRingOnFinger(int32 FingerIndex) const;
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
 bool HitTestInventoryBagDropSurface() const;
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
 // RingSlotIndex is zero-based in the stable ten-finger order.
 UFUNCTION(BlueprintCallable, Category="Moonfall|Equipment") bool EquipInventoryRing(int32 InventoryIndex, int32 RingSlotIndex);
 UFUNCTION(BlueprintCallable, Category="Moonfall|Equipment") bool MoveEquippedRing(int32 FromRingIndex,int32 ToRingIndex);
 UFUNCTION(BlueprintCallable, Category="Moonfall|Equipment") bool UnequipRing(int32 RingSlotIndex);
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
 // Optional authored VFX/audio, emitted only after actual hostile HP damage.
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Combat|Presentation")
 void OnSpellImpact(int32 School, bool bLeftHand, FVector ImpactPoint,
   float AppliedDamage, bool bCritical, bool bKillingBlow);
private:
 // Raw worn references are for validation and ownership protection only.
 int32 GetRawEquipmentSlotCode(int32 Slot) const;
 int32 GetCraftEquipmentSlot(int32 Code) const;
 bool CanCraftEquipmentCopy(int32 Code,int32 Slot) const;
 bool CanEditEquippedRings() const;
 int32 RingNavigationIndex=INDEX_NONE;
 int32 RingNavigationSwapSource=INDEX_NONE;
 int32 RingNavigationSwapCode=0;
 uint32 RingNavigationSwapRevision=0;
 uint64 RingNavigationSwapFingerprint=0;
 bool bRingNavigationSwapInterrupted=false;
 void ClearRingNavigationSwap();
 void ClearRingNavigationFocus();
 void ArmOrCancelRingNavigationSwap();
 void ConfirmRingNavigationAction();
 bool CanFocusRingNavigation() const;
 void ClearRingPointerDrag();
 void BeginRingPointerDrag(int32 BagIndex,int32 FingerIndex);
 int32 RingDragBagIndex=INDEX_NONE;
 int32 RingDragFingerIndex=INDEX_NONE;
 int32 RingDragCode=0;
 uint32 RingDragInventoryRevision=0;
 uint64 RingDragLoadoutFingerprint=0;
 mutable std::array<int8,MoonfallEquipmentLayout::RingSlotCount> RingDragTargetCache{};
 int32 GetLootComparisonTier(int32 Slot) const;
 void MarkEquipmentInventoryDirty();
 uint32 EquipmentInventoryRevision=1;
 mutable uint32 CachedInventoryOrderRevision=0;
 mutable int32 CachedInventoryOrderMode=-1;
 mutable TArray<int32> CachedInventoryOrder;
 mutable TArray<int32> CachedInventoryOrderSource;
 mutable uint32 CachedValidatedInventoryRevision=0;
 mutable uint64 CachedValidatedLoadoutFingerprint=0;
 mutable TArray<int32> CachedValidatedEquipmentCodes;
 mutable uint32 CachedRingBonusInventoryRevision=0;
 mutable uint64 CachedRingBonusLoadoutFingerprint=0;
 mutable MoonfallEquipmentLayout::FRingBonuses CachedNativeRingBonuses{};
 int32 EquipBestExpandedLoadout();
 MoonfallEquipmentLayout::FRingBonuses GetNativeRingBonuses() const;
 // Fixed storage for slot IDs 3–23. Public gameplay remains gated until
 // the expanded inventory, loot and equipment UI are integrated.
 int32 ExtraEquippedCodes[21] = {};
 void RefreshEquipmentTier();
 MoonfallPerformance::FAudit PerformanceAudit;
 MoonfallPerformance::FStageAudit StageAudit;
 double NextStageAuditAt=-1;
 double StageAuditWarmupUntil=-1;
 int64 AuditDedicatedVideoMemory=-1;
 int64 AuditTextureMemory=-1;
 int64 AuditTexturePool=-1;
 void RefreshGraphicsMemoryAudit();
 void ExportBugReport();
 FString BugReportStatus;
 MoonfallDiagnostics::FBreadcrumbs BugBreadcrumbs;
 double LastBugReportAt=-1;
 double NextRecoverySnapshotAt=-1;
 FString RecoverySnapshotPath;
 void UpdateRecoverySnapshot(double Now);
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
 void ResetMeleeHitCombo();
 MoonfallOmniattack::FFlow SwordFlow;
 MoonfallCombatInputBuffer::FSingleSwing SwordBuffer;
 float NextMeleeAttackAt = 0.f;
 TWeakObjectPtr<AMoonfallEnemy> LastHitComboTarget;
 FMoonfallCompanionPartyRecord CompanionParty;
 TMap<FName,int32> CompanionProfiles;
 TMap<FName,FMoonfallCompanionGearRecord> CompanionGear;
 TWeakObjectPtr<AMoonfallEnemy> RecentCompanionAssistTarget;
 float RecentCompanionAssistHitAt=-100.f;
 float LastSuccessfulHitAt = -100.f;
 float LastCastTime[2] = {-100.f,-100.f};
 float NextDodgeAt = 0.f;
 MoonfallCombatInputBuffer::FSingleSwing DodgeBuffer;
 FVector BufferedDodgeDirection = FVector::ZeroVector;
 bool bDispatchingBufferedDodge = false;
 FVector IncomingDamageSource = FVector::ZeroVector;
 bool bHasIncomingDamageSource = false;
 bool bIncomingDamageDodgeOnly = false;
 int32 IncomingSwordAngle = -1;
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
 float LastOffensiveImpactAt = -100.f;
 // Prevent synchronous cosmetic callbacks from recursively resolving attacks.
 bool bResolvingCombatAction = false;
 void ApplyConfirmedHitFeedback(float Now, float Damage, bool bBoss, bool bLethal);
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
 // Nonpersistent, timestamped only when health actually decreases. Gives
 // open live-world menus a reliable hit cue without polling enemy actors.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Combat")
 float LastRealDamageAt = -100.f;
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
 virtual void StopJumping() override;
 void TryJump();
 void ControllerConfirm();
 void ControllerConfirmRelease();
 void ControllerBack();
 void ControllerLootOrSalvage();
 void ControllerLeftShoulder();
 void ControllerRightShoulder();
 void ControllerRightTrigger();
 void ControllerLeftTrigger();
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
 void ControllerRightStickPress();
 void ControllerRightStickRelease();
 bool bControllerGuardStarted = false;
 bool bControllerJumpStarted = false;
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
 bool TryPlayAuthoredSpellImpact(int32 School, const FVector& Point, float Multiplier);
 TArray<TWeakObjectPtr<UNiagaraComponent>> ActiveSpellImpactFX;
 bool bEquipmentCraftInProgress = false;
 bool bEquipBestInProgress = false;
 bool bTalentAllocationInProgress = false;
 // Session-only owned gear choices made at zero HP; never serialized as loot.
 int32 DefeatedEquipmentSelection[24] = {};
 bool EquipInventoryItemWithPreviousTier(int32 InventoryIndex, int32 PreviousTierOverride, int32 TargetSlot = INDEX_NONE);
};
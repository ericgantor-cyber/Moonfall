#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TimerManager.h"
class AMoonfallArena;
class AMoonfallWorldNPC;
class AMoonfallHeroPawn;
class AMoonfallEnemy;
#include "MoonfallUnrealGameMode.generated.h"

UENUM(BlueprintType)
enum class EMoonfallCompanionStance : uint8 { Follow, Hold, Passive };
UENUM(BlueprintType)
enum class EMoonfallCompanionRole : uint8 { Balanced, Protector, Striker };

// Thirty physical dungeons in six world regions. Geometry is spawned in small
// time-sliced batches; the GameMode never polls each dungeon every frame.
UCLASS()
class MOONFALLUNREAL_API AMoonfallUnrealGameMode : public AGameModeBase
{
 GENERATED_BODY()
public:
 AMoonfallUnrealGameMode();
 virtual void BeginPlay() override;

 static constexpr int32 DungeonZoneCount = 6;
 static constexpr int32 DungeonsPerZone = 5;
 static constexpr int32 TotalDungeons = DungeonZoneCount * DungeonsPerZone;
 static constexpr float DungeonLaneSpacing = 1350.f;

 UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Moonfall|Dungeon|World")
 bool bGenerateDungeonWorld = true;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Dungeon|World")
 int32 ReadyDungeonCount = 0;
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Dungeon|World")
 void OnDungeonWorldReady(int32 ReadyCount);
 // Allows the atlas/quest system to locate a specific dungeon without
 // scanning every spawned actor or changing the existing save format.
 UFUNCTION(BlueprintPure, Category="Moonfall|Dungeon|World")
 bool GetDungeonEntranceTransform(int32 ZoneIndex, int32 DungeonIndex,
    FTransform& OutTransform) const;
 // Read-only HUD hint from the single cached ambient event. Never scans
 // all actors and never changes any player quest or world-save state.
 FString GetNearbyAmbientEventHint(const FVector& HeroLocation) const;
 // Lifecycle registration keeps late-spawned and designer-placed residents
 // participating in the living world without rescanning the entire map.
 void RegisterLivingResident(AMoonfallWorldNPC* Resident);
 void UnregisterLivingResident(AMoonfallWorldNPC* Resident);
 // Explicit local recruitment; only one registered trusted scout per world.
 UFUNCTION(BlueprintCallable, Category="Moonfall|Companion")
 bool RecruitLocalCompanion(AMoonfallHeroPawn* Hero, AMoonfallWorldNPC* Resident);
 UFUNCTION(BlueprintCallable, Category="Moonfall|Companion")
 bool SetLocalCompanionStance(AMoonfallHeroPawn* Hero, EMoonfallCompanionStance Stance);
 UFUNCTION(BlueprintCallable, Category="Moonfall|Companion")
 bool SetLocalCompanionRole(AMoonfallHeroPawn* Hero, EMoonfallCompanionRole Role);
 UFUNCTION(BlueprintCallable, Category="Moonfall|Companion")
 bool TrainLocalCompanionTalent(AMoonfallHeroPawn* Hero,int32 Node);
 UFUNCTION(BlueprintCallable, Category="Moonfall|Companion")
 bool ResetLocalCompanionTalents(AMoonfallHeroPawn* Hero);
 uint32 GetLocalCompanionTraining() const { return LocalCompanionTraining; }
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Companion")
 EMoonfallCompanionRole LocalCompanionRole=EMoonfallCompanionRole::Balanced;
 UFUNCTION(BlueprintCallable, Category="Moonfall|Companion")
 bool DismissLocalCompanion(AMoonfallHeroPawn* Hero);
 UFUNCTION(BlueprintCallable, Category="Moonfall|Companion")
 bool OrderCompanionMove(AMoonfallHeroPawn* Hero,FVector Destination);
 UFUNCTION(BlueprintCallable, Category="Moonfall|Companion")
 bool ReviveLocalCompanion(AMoonfallHeroPawn* Hero);
 UFUNCTION(BlueprintPure, Category="Moonfall|Companion")
 AMoonfallWorldNPC* GetLocalCompanion() const;
 // Internal hit-time support; does not attack enemies or grant immunity.
 float ConsumeLocalCompanionCover(AMoonfallHeroPawn* Hero,float Damage,const FVector& Source);
 void ReportCompanionThreat(AMoonfallHeroPawn* Hero,AMoonfallEnemy* Enemy,float Now);
 AMoonfallWorldNPC* SelectCompanionCounterTarget(AMoonfallHeroPawn* Hero,AMoonfallEnemy* Enemy,float Now,float Range) const;
 void ResolveCompanionCounterAttack(AMoonfallHeroPawn* Hero,AMoonfallEnemy* Enemy,
      AMoonfallWorldNPC* Target,const FVector& Direction,float Damage,float Range);
 UFUNCTION(BlueprintCallable, Category="Moonfall|Companion")
 bool ActivateLocalCompanionAbility(AMoonfallHeroPawn* Hero);
 float GetCompanionAbilityRemaining() const;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Companion")
 EMoonfallCompanionStance LocalCompanionStance=EMoonfallCompanionStance::Passive;

private:
 FTimerHandle DungeonSpawnTimer;
 int32 NextDungeonSlot = 0;
 TArray<uint8> AuthoredDungeonSlots;
 void SpawnDungeonBatch();
 // One 4-second simulation pulse, no NPC Tick and no per-frame world scan.
 void PulseLivingWorld();
 FTimerHandle LivingWorldPulseTimer;
 FTimerHandle FollowerPulseTimer;
 void PulseFollower();
 void TryRestoreCompanionParty(AMoonfallHeroPawn* Hero);
 void AttachCompanionParty(AMoonfallHeroPawn* Hero,AMoonfallWorldNPC* Resident,int32 Stance,float Health);
 void SyncCompanionVitals(AMoonfallHeroPawn* Hero);
 int64 CompanionAbilityReadyUtc=0;
 TWeakObjectPtr<AMoonfallWorldNPC> ReturningCompanion;
 FVector OrderedFollowerDestination=FVector::ZeroVector;
 bool bFollowerMoveOrdered=false;
 float FollowerSeparatedAt=-1.f;
 float NextFollowerCatchupAt=-1.f;
 bool bLivingResidentCacheInitialized = false;
 TWeakObjectPtr<AMoonfallArena> LivingArena;
 TArray<TWeakObjectPtr<AMoonfallWorldNPC>> LivingResidents;
 // Round-robin slices 20 nearby residents every four seconds. Registers
 // more than one settlement without unbounded per-pulse NPC updates.
 int32 NextResidentPulseIndex = 0;
 // Reuse the existing resident pulse; no additional timer or actor scan.
 TWeakObjectPtr<AMoonfallWorldNPC> LocalCompanion;
 TWeakObjectPtr<AMoonfallHeroPawn> CompanionOwner;
 uint32 LocalCompanionTraining=0;
 float LastCompanionCoverAt=-1000.f;
 float LastCompanionStrikeAt=-1000.f;
 float CompanionAbilityReadyAt=-1000.f;
 float CompanionAbilityEndsAt=-1000.f;
 EMoonfallCompanionRole CompanionAbilityRole=EMoonfallCompanionRole::Balanced;
 bool CanReceiveCompanionAttack(const AMoonfallHeroPawn* Hero,const AMoonfallWorldNPC* Scout) const;
 TWeakObjectPtr<AMoonfallEnemy> CompanionThreat;
 float CompanionThreatAt=-1000.f;
 void TryLocalCompanionStrike(AMoonfallHeroPawn* Hero,float Now);
 bool IsLocalCompanionOwner(const AMoonfallHeroPawn* Hero) const;
 int32 SocialSpeakerCursor = 0;
 int32 LastSocialCombatPhase = -1;
 void ShareNearbyResidentRumor(const TArray<TWeakObjectPtr<AMoonfallWorldNPC>>& Candidates);
 TWeakObjectPtr<AActor> ActiveAmbientEncounter;
 // World-pulse ownership prevents the sole active encounter despawning
 // during an open NPC conversation or an active nearby monster battle.
 float ActiveAmbientExpiresAt = -1.f;
 float NextAmbientEncounterAt = 120.f;
 int32 LastAmbientEventPhase = -1;
};

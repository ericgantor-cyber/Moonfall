#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MoonfallArena.generated.h"
class AMoonfallEnemy;
UCLASS()
class MOONFALLUNREAL_API AMoonfallArena : public AActor {
 GENERATED_BODY()
public:
 AMoonfallArena();
 virtual void BeginPlay() override;
 virtual void Tick(float DeltaSeconds) override;
 UPROPERTY(EditAnywhere, Category="World") bool bContinuousWorld = true;
 UPROPERTY(EditAnywhere, Category="Dungeon") int32 EnemyCount = 5;
 // Read-only C++ access for the compact explored-only minimap.
 const TSet<int32>& GetDiscoveredChambers() const { return DiscoveredChambers; }
 const TSet<int32>& GetDiscoveredRegions() const { return DiscoveredRegions; }
 const TSet<int32>& GetDiscoveredDungeonEntrances() const { return DiscoveredDungeonEntrances; }
 // One grounded quest per dungeon, completed only when its actual final guardian dies.
 UFUNCTION(BlueprintCallable, Category="Moonfall|Quests") bool TrackDungeonQuest(int32 EntranceId);
 UFUNCTION(BlueprintPure, Category="Moonfall|Quests") bool IsDungeonQuestCompleted(int32 EntranceId) const;
 UFUNCTION(BlueprintPure, Category="Moonfall|Quests") int32 GetTrackedDungeonQuestId() const { return TrackedDungeonQuestId; }
 UFUNCTION(BlueprintPure, Category="Moonfall|Quests") int32 GetCompletedDungeonQuestCount() const;
 UFUNCTION(BlueprintPure, Category="Moonfall|Quests") int32 GetDungeonResidentDefeatCount(int32 EntranceId) const;
 // Guide interactions only select a quest that the player already discovered.
 UFUNCTION(BlueprintCallable, Category="Moonfall|Quests") bool SelectNextDiscoveredDungeonQuestInRegion(int32 RegionIndex);
 // Region-scoped NPC memory: social choices are persisted and earned guardian
 // reputation is derived from verified encounter defeats rather than a timer.
 UFUNCTION(BlueprintCallable, Category="Moonfall|NPC") bool RecordGuideSocialChoice(int32 RegionIndex, bool bIntimidate);
 UFUNCTION(BlueprintPure, Category="Moonfall|NPC") int32 GetGuideTrust(int32 RegionIndex) const;
 UFUNCTION(BlueprintPure, Category="Moonfall|NPC") int32 GetGuideFear(int32 RegionIndex) const;
 UFUNCTION(BlueprintCallable, Category="Moonfall|NPC") bool RecordNpcSocialChoice(int32 NpcId, bool bIntimidate);
 UFUNCTION(BlueprintPure, Category="Moonfall|NPC") int32 GetNpcTrust(int32 NpcId) const;
 UFUNCTION(BlueprintPure, Category="Moonfall|NPC") int32 GetNpcFear(int32 NpcId) const;
private:
 TSet<int32> BuiltRooms;
 TSet<int32> SpawnedEncounters;
 TSet<int32> DefeatedEncounters;
 TSet<int32> DiscoveredChambers;
 TSet<int32> DiscoveredRegions;
 TSet<int32> DiscoveredDungeonEntrances;
 int32 TrackedDungeonQuestId = -1;
 TArray<int32> GuideTrustMemory;
 TArray<int32> GuideFearMemory;
 TArray<int32> NpcTrustMemory;
 TArray<int32> NpcFearMemory;
 TMap<int32, float> LastNpcSocialActionAt;
 TMap<int32, float> LastGuideSocialActionAt;
 TMap<int32, TWeakObjectPtr<AMoonfallEnemy>> ActiveEncounters;
 bool bWorldSavePending = false;
 void LoadWorldProgress();
 void ObserveDefeatedEncounters();
 void SaveWorldProgress();
 static bool IsValidEncounterId(int32 Id);
 void BuildWorld();
 void StreamWorld();
 bool SpawnEncounter(int32 Id, FVector Position, int32 Region, bool Elite, bool Boss);
 void CreateBlock(FVector Position, FVector Scale);
};

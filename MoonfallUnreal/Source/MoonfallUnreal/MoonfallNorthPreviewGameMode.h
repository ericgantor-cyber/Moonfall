#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "MoonfallAquaticRewards.h"
#include "MoonfallIslandQuest.h"
#include "MoonfallOceanEvents.h"
#include "MoonfallOceanGathering.h"
#include "MoonfallOceanCrafting.h"
#include "MoonfallFishing.h"
#include "MoonfallRegionalFishing.h"
#include "MoonfallRegionalDiscovery.h"
#include "MoonfallNorthPreviewGameMode.generated.h"

// Isolated Northern Expanse QA game mode. Never spawns the legacy arena,
// 30-dungeon world, bosses, divine characters or save-state content.
UCLASS()
class MOONFALLUNREAL_API AMoonfallNorthPreviewGameMode : public AGameModeBase {
 GENERATED_BODY()
public:
 AMoonfallNorthPreviewGameMode();
 virtual void StartPlay() override;
 virtual void EndPlay(const EEndPlayReason::Type Reason) override;
 bool IsProfessionSaveBlocked() const { return bProfessionSaveBlocked; }
 bool IsProfessionSaveDirty() const { return bProfessionDirty; }
 bool IsProfessionSaveEnabled() const { return bProfessionSaveEnabled; }
 bool HasGrottoClaim(int32 Id) const { return Id>=0&&Id<6&&(GrottoGear.Claims&(1u<<Id)); }
 // Rewards and claims are session-only and survive hero respawn.
 bool ClaimGrotto(int32 Index) { const bool OK=GrottoGear.Claim(Index,FMath::RandRange(0,99)); bProfessionDirty|=OK; return OK; }
 int32 GetGrottoClaimCount() const { return GrottoGear.Count(); }
 int32 GetGrottoSetPieces() const { return GrottoGear.Pieces(); }
 int32 GetGrottoGearRarity(int32 Piece) const { return GrottoGear.Rarity(Piece); }
 int32 GetLastGrottoRewardCode() const { return GrottoGear.LastCode; }
 int32 GetBreathingTonicCharges() const { return GrottoGear.TonicCharges; }
 bool ConsumeBreathingTonic() { const bool OK=GrottoGear.ConsumeTonic(); bProfessionDirty|=OK; return OK; }
 // Wreck discoveries are separate from grotto rewards and reset with the world.
 void DiscoverWreck(int32 Index) { if (Index>=0 && Index<2 && !(WreckClaims&(1u<<Index))) { WreckClaims|=uint8(1u<<Index); bProfessionDirty=true; } }
 int32 GetWreckDiscoveryCount() const { return int32((WreckClaims&1u)!=0)+int32((WreckClaims&2u)!=0); }
 void DiscoverCave(int32 Index) { if (Index>=0 && Index<2 && !(CaveClaims&(1u<<Index))) { CaveClaims|=uint8(1u<<Index); bProfessionDirty=true; } }
 int32 GetCaveDiscoveryCount() const { return int32((CaveClaims&1u)!=0)+int32((CaveClaims&2u)!=0); }
 int32 GetIslandQuestProgress(int32 Id) const {
  return Id==0?GetWreckDiscoveryCount():Id==1?GetCaveDiscoveryCount():Id==2?GetGrottoClaimCount():0;
 }
 int32 TalkIslandQuest(int32 Id) {
  const int32 Before=IslandJournal.State(Id); const int32 Result=IslandJournal.Talk(Id,GetIslandQuestProgress(Id));
  bProfessionDirty|=Before!=IslandJournal.State(Id); return Result;
 }
 int32 GetIslandQuestState(int32 Id) const { return IslandJournal.State(Id); }
 int32 GetIslandQuestCount() const { return IslandJournal.Count(); }
 int32 GetIslandReputation() const { return IslandJournal.Reputation(); }
 int32 GetIslandCredits() const { return IslandJournal.Credits; }
 int32 GetIslandTonicPrice() const { return IslandJournal.TonicPrice(); }
 bool BuyIslandTonic() { const bool OK=IslandJournal.BuyTonic(GrottoGear.TonicCharges); bProfessionDirty|=OK; return OK; }
 int32 GetOceanActivity() const;
 void DiscoverRegionalWater(int32 Province) { if(HasAuthority()) bProfessionDirty|=Regions.Visit(Province); }
 int32 GetRegionalVisited() const { return Regions.Count(Regions.Visited); }
 int32 GetRegionalFished() const { return Regions.Count(Regions.Caught); }
 int32 GetRegionalGathered() const { return Regions.Count(Regions.Harvested); }
 int32 GetRegionalCompleted() const { return Regions.Completed(); }
 int32 GetRegionalMilestones(int32 Id) const { return Regions.Milestones(Id); }
 bool HasVisitedRegionalWater(int32 Id) const { return Regions.HasVisited(Id); }
 bool DiscoverAquaticCreature(int32 Id) { const bool OK=Sightings.Discover(Id); bProfessionDirty|=OK; return OK; }
 int32 GetAquaticSightingCount() const { return Sightings.Count(); }
 bool HasAquaticSighting(int32 Id) const { return Sightings.Has(Id); }
 bool CollectOceanResource(int32 Id) { const bool OK=HasAuthority() && GetWorld() && Gathering.Collect(Id,GetOceanActivity(),GetWorld()->GetTimeSeconds()); if(OK&&Id>=MoonfallOceanGathering::BaseNodeCount) Regions.Gather((Id-MoonfallOceanGathering::BaseNodeCount)/4); bProfessionDirty|=OK; return OK; }
 float GetOceanResourceCooldown(int32 Id) const {
  return GetWorld()?float(Gathering.RemainingCooldown(Id,GetWorld()->GetTimeSeconds())):0.f;
 }
 int32 GetOceanMaterialCount() const { return Gathering.Materials(); }
 int32 GetOceanCatchCount() const { return Gathering.Catches(); }
 bool CanCatchOceanFish() const { return Gathering.CanCatch(); }
 bool IsFishingSchoolAvailable(int32 Id,int32 Province=-1) const { return GetWorld() && Schools[SchoolBank(Province)].Available(Id,GetWorld()->GetTimeSeconds()); }
 bool CatchOceanFish(int32 SchoolId=-1,int32 Province=-1) {
  if(!HasAuthority()||!GetWorld()||!Gathering.CanCatch()||SchoolId<-1||SchoolId>=MoonfallFishing::SchoolCount) return false;
  const bool bSchool=SchoolId>=0 && Schools[SchoolBank(Province)].Take(SchoolId,GetWorld()->GetTimeSeconds());
  const int32 Event=GetOceanActivity();
  const bool bBait=Workshop.Bait>0 && Workshop.Bait<=MoonfallOceanCrafting::BaitCapacity;
  const int32 Rank=MoonfallFishing::Rank(Gathering.Catches());
  const int32 Bonus=MoonfallRegionalFishing::TrophyBonus(Province,Rank,bSchool)+(bSchool&&Regions.Complete(Province)?2:0);
  const bool bTrophy=FMath::RandRange(0,99)<MoonfallFishing::TrophyChance(Gathering.Catches(),bSchool,Event,bBait)+Bonus;
  const int32 Legendary=bSchool?MoonfallFishing::LegendarySpecies(Gathering.Catches(),SchoolId,Event,FMath::RandRange(0,99),bBait):-1;
  const bool bCaught=Gathering.Catch(Event,FMath::RandRange(0,99),bSchool?MoonfallFishing::SchoolSpecies(SchoolId):MoonfallRegionalFishing::Preferred(Province,Rank),bTrophy,Legendary);
  if(bCaught&&bBait) Workshop.UseBait();
  if(bCaught) Regions.Fish(Province);
  bProfessionDirty|=bCaught;
  return bCaught;
 }
 int32 GetFishingBaitCount() const { return Workshop.Bait; }
 int32 GetLastFishSpecies() const { return Gathering.LastSpecies; }
 bool WasLastFishLegendary() const { return Gathering.LastLegendary; }
 int32 GetLegendaryFishCount() const { return Gathering.Legendaries(); }
 bool HasLegendaryFish(int32 Id) const { return Gathering.HasLegendary(Id); }
 bool WasLastFishTrophy() const { return Gathering.LastTrophy; }
 int32 GetTrophyCatchCount() const { return Gathering.TrophyCatches; }
 int32 GetTrophySpeciesCount(int32 Id) const { return Gathering.Trophies(Id); }
 int32 GetDiscoveredFishSpecies() const { return Gathering.DiscoveredSpecies(); }
 int32 GetLifetimeSpeciesCount(int32 Id) const { return Gathering.Species(Id); }
 int32 GetOceanMaterialStock(int32 Id) const { return Workshop.RemainingMaterial(Id,Gathering); }
 int32 GetOceanFishCount(int32 Id) const { return Workshop.RemainingSpecies(Id,Gathering); }
 int32 GetOceanGatheringRank() const { return MoonfallOceanGathering::GatheringRank(Gathering.Materials()); }
 bool CraftOceanRecipe(int32 Recipe) { const bool OK=Workshop.Craft(Recipe,Gathering,GrottoGear); bProfessionDirty|=OK; return OK; }
 bool CanCraftOceanRecipe(int32 Recipe) const { return Workshop.CanCraft(Recipe,Gathering,GrottoGear); }
 bool HasOceanCraftMaterial(int32 Id) const { return Workshop.HasMaterial(Id,Gathering); }
 int32 GetAvailableOceanMaterials() const { return Workshop.AvailableMaterials(Gathering); }
 int32 GetAvailableOceanFish() const { return Workshop.RemainingFish(Gathering); }
private:
 bool bProfessionDirty=false,bProfessionSaveBlocked=false,bProfessionSaveEnabled=false;
 int64 ProfessionSequence=0;
 FTimerHandle ProfessionSaveTimer;
 void LoadProfessions();
 void FlushProfessions();
 // One neutral bank and one per authored northern province.
 static int32 SchoolBank(int32 Province) { return MoonfallRegionalFishing::Valid(Province)?Province+1:0; }
 MoonfallFishing::FSchoolStock Schools[MoonfallNorth::ZoneCount+1];
 MoonfallRegionalDiscovery::FProgress Regions;
 MoonfallOceanCrafting::FWorkshop Workshop;
 MoonfallOceanGathering::FCollection Gathering;
 MoonfallOceanEvents::FSightings Sightings;
 MoonfallIslandQuest::FJournal IslandJournal;
 uint8 CaveClaims=0;
 uint8 WreckClaims=0;
 MoonfallAquaticRewards::FSessionGear GrottoGear;
};

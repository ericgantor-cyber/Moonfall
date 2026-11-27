#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Engine/World.h"
#include "MoonfallAquaticRewards.h"
#include "MoonfallIslandQuest.h"
#include "MoonfallOceanEvents.h"
#include "MoonfallOceanGathering.h"
#include "MoonfallOceanCrafting.h"
#include "MoonfallFishing.h"
#include "MoonfallNorthPreviewGameMode.generated.h"

// Isolated Northern Expanse QA game mode. Never spawns the legacy arena,
// 30-dungeon world, bosses, divine characters or save-state content.
UCLASS()
class MOONFALLUNREAL_API AMoonfallNorthPreviewGameMode : public AGameModeBase {
 GENERATED_BODY()
public:
 AMoonfallNorthPreviewGameMode();
 // Rewards and claims are session-only and survive hero respawn.
 bool ClaimGrotto(int32 Index) { return GrottoGear.Claim(Index,FMath::RandRange(0,99)); }
 int32 GetGrottoClaimCount() const { return GrottoGear.Count(); }
 int32 GetGrottoSetPieces() const { return GrottoGear.Pieces(); }
 int32 GetGrottoGearRarity(int32 Piece) const { return GrottoGear.Rarity(Piece); }
 int32 GetLastGrottoRewardCode() const { return GrottoGear.LastCode; }
 int32 GetBreathingTonicCharges() const { return GrottoGear.TonicCharges; }
 bool ConsumeBreathingTonic() { return GrottoGear.ConsumeTonic(); }
 // Wreck discoveries are separate from grotto rewards and reset with the world.
 void DiscoverWreck(int32 Index) { if (Index>=0 && Index<2) WreckClaims|=uint8(1u<<Index); }
 int32 GetWreckDiscoveryCount() const { return int32((WreckClaims&1u)!=0)+int32((WreckClaims&2u)!=0); }
 void DiscoverCave(int32 Index) { if (Index>=0 && Index<2) CaveClaims|=uint8(1u<<Index); }
 int32 GetCaveDiscoveryCount() const { return int32((CaveClaims&1u)!=0)+int32((CaveClaims&2u)!=0); }
 int32 GetIslandQuestProgress(int32 Id) const {
  return Id==0?GetWreckDiscoveryCount():Id==1?GetCaveDiscoveryCount():Id==2?GetGrottoClaimCount():0;
 }
 int32 TalkIslandQuest(int32 Id) {
  return IslandJournal.Talk(Id,GetIslandQuestProgress(Id));
 }
 int32 GetIslandQuestState(int32 Id) const { return IslandJournal.State(Id); }
 int32 GetIslandQuestCount() const { return IslandJournal.Count(); }
 int32 GetIslandReputation() const { return IslandJournal.Reputation(); }
 int32 GetIslandCredits() const { return IslandJournal.Credits; }
 int32 GetIslandTonicPrice() const { return IslandJournal.TonicPrice(); }
 bool BuyIslandTonic() { return IslandJournal.BuyTonic(GrottoGear.TonicCharges); }
 int32 GetOceanActivity() const;
 bool DiscoverAquaticCreature(int32 Id) { return Sightings.Discover(Id); }
 int32 GetAquaticSightingCount() const { return Sightings.Count(); }
 bool HasAquaticSighting(int32 Id) const { return Sightings.Has(Id); }
 bool CollectOceanResource(int32 Id) { return HasAuthority() && GetWorld() && Gathering.Collect(Id,GetOceanActivity(),GetWorld()->GetTimeSeconds()); }
 int32 GetOceanMaterialCount() const { return Gathering.Materials(); }
 int32 GetOceanCatchCount() const { return Gathering.Catches(); }
 bool CanCatchOceanFish() const { return Gathering.CanCatch(); }
 bool IsFishingSchoolAvailable(int32 Id) const { return GetWorld() && Schools.Available(Id,GetWorld()->GetTimeSeconds()); }
 bool CatchOceanFish(int32 SchoolId=-1) {
  if(!HasAuthority()||!GetWorld()||!Gathering.CanCatch()||SchoolId<-1||SchoolId>=MoonfallFishing::SchoolCount) return false;
  const bool bSchool=SchoolId>=0 && Schools.Take(SchoolId,GetWorld()->GetTimeSeconds());
  const int32 Event=GetOceanActivity();
  const bool bTrophy=FMath::RandRange(0,99)<MoonfallFishing::TrophyChance(Gathering.Catches(),bSchool,Event);
  return Gathering.Catch(Event,FMath::RandRange(0,99),bSchool?MoonfallFishing::SchoolSpecies(SchoolId):-1,bTrophy);
 }
 int32 GetLastFishSpecies() const { return Gathering.LastSpecies; }
 bool WasLastFishTrophy() const { return Gathering.LastTrophy; }
 int32 GetTrophyCatchCount() const { return Gathering.TrophyCatches; }
 int32 GetTrophySpeciesCount(int32 Id) const { return Gathering.Trophies(Id); }
 int32 GetDiscoveredFishSpecies() const { return Gathering.DiscoveredSpecies(); }
 int32 GetLifetimeSpeciesCount(int32 Id) const { return Gathering.Species(Id); }
 int32 GetOceanMaterialStock(int32 Id) const { return Workshop.RemainingMaterial(Id,Gathering); }
 int32 GetOceanFishCount(int32 Id) const { return Workshop.RemainingSpecies(Id,Gathering); }
 int32 GetOceanGatheringRank() const { return MoonfallOceanGathering::GatheringRank(Gathering.Materials()); }
 bool CraftOceanRecipe(int32 Recipe) { return Workshop.Craft(Recipe,Gathering,GrottoGear); }
 bool CanCraftOceanRecipe(int32 Recipe) const { return Workshop.CanCraft(Recipe,Gathering,GrottoGear); }
 bool HasOceanCraftMaterial(int32 Id) const { return Workshop.HasMaterial(Id,Gathering); }
 int32 GetAvailableOceanMaterials() const { return Workshop.AvailableMaterials(Gathering); }
 int32 GetAvailableOceanFish() const { return Workshop.RemainingFish(Gathering); }
private:
 // Eight shared schools for the isolated single-basin preview.
 MoonfallFishing::FSchoolStock Schools;
 MoonfallOceanCrafting::FWorkshop Workshop;
 MoonfallOceanGathering::FCollection Gathering;
 MoonfallOceanEvents::FSightings Sightings;
 MoonfallIslandQuest::FJournal IslandJournal;
 uint8 CaveClaims=0;
 uint8 WreckClaims=0;
 MoonfallAquaticRewards::FSessionGear GrottoGear;
};

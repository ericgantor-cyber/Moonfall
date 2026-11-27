#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MoonfallAquaticRewards.h"
#include "MoonfallIslandQuest.h"
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
private:
 MoonfallIslandQuest::FJournal IslandJournal;
 uint8 CaveClaims=0;
 uint8 WreckClaims=0;
 MoonfallAquaticRewards::FSessionGear GrottoGear;
};

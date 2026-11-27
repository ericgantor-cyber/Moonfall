#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MoonfallAquaticRewards.h"
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
private:
 MoonfallAquaticRewards::FSessionGear GrottoGear;
};

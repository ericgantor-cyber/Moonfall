#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MoonfallNorthPreviewGameMode.generated.h"

// Isolated Northern Expanse QA game mode. Never spawns the legacy arena,
// 30-dungeon world, bosses, divine characters or save-state content.
UCLASS()
class MOONFALLUNREAL_API AMoonfallNorthPreviewGameMode : public AGameModeBase {
 GENERATED_BODY()
public:
 AMoonfallNorthPreviewGameMode();
 // Six distinct session discoveries, reset only when the preview world ends.
 bool ClaimGrotto(int32 Index) {
  if (Index < 0 || Index >= 6) return false;
  const uint8 Bit = uint8(1u << Index);
  if (GrottoClaims & Bit) return false;
  GrottoClaims |= Bit;
  return true;
 }
 int32 GetGrottoClaimCount() const {
  int32 Count = 0;
  for (int32 I = 0; I < 6; ++I) if (GrottoClaims & (1u << I)) ++Count;
  return Count;
 }
private:
 uint8 GrottoClaims = 0;
};

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
};

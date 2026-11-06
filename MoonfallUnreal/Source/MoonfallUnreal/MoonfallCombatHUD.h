#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
class AMoonfallHeroPawn;
#include "MoonfallCombatHUD.generated.h"
UCLASS()
class MOONFALLUNREAL_API AMoonfallCombatHUD : public AHUD
{
 GENERATED_BODY()
public:
 virtual void DrawHUD() override;
 virtual void BeginPlay() override;
 virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
 float DisplayHealth = -1.f;
 float DisplayMana = -1.f;
 float DisplayXP = -1.f;
 int32 LastShownLevel = -1;
 float LevelGlowUntil = 0.f;
 // Ten-metre exploration tiles retain substantially more of the continuous world.
 // The bounded list preserves save order while the set makes per-frame lookups cheap.
 static constexpr float ExplorationCellSizeCm = 1000.f;
 static constexpr int32 MaxExplorationCells = 4096;
 TArray<FIntPoint> ExploredMapCells;
 TSet<FIntPoint> ExploredMapIndex;
 bool bExplorationDirty = false;
 float NextExplorationSaveAt = 0.f;
 void SaveExploration();
 void DrawExplorationAtlas(const AMoonfallHeroPawn* Hero);
};

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
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
 // Remember only visited 2-metre cells; bounded for long play sessions.
 TArray<FIntPoint> ExploredMapCells;
 bool bExplorationDirty = false;
 float NextExplorationSaveAt = 0.f;
 void SaveExploration();
};

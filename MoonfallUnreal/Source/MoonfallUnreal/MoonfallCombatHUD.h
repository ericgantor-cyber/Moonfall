#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "MoonfallCombatHUD.generated.h"
class AMoonfallHeroPawn;
UCLASS()
class MOONFALLUNREAL_API AMoonfallCombatHUD : public AHUD
{
 GENERATED_BODY()
public:
 virtual void DrawHUD() override;
private:
 float DisplayHealth = -1.f;
 float DisplayMana = -1.f;
 float DisplayStamina = -1.f;
 float DisplayXP = -1.f;
 int32 LastShownLevel = -1;
 float LevelGlowUntil = 0.f;
 // Cache only the read-only selected-gear score, not the inventory itself.
 TWeakObjectPtr<const AMoonfallHeroPawn> CachedAdvisorHero;
 bool bAdvisorScoreCached = false;
 float AdvisorScoreUpdatedAt = -1.f;
 int32 CachedAdvisorCount = -1;
 int32 CachedAdvisorSelected = INDEX_NONE;
 int32 CachedAdvisorSelectedCode = 0;
 int32 CachedAdvisorWeapon = 0;
 int32 CachedAdvisorArmor = 0;
 int32 CachedAdvisorAccessory = 0;
 int32 CachedAdvisorScore = 0;
};

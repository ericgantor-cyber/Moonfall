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
 double AdvisorScoreUpdatedAt = -1.0;
 int32 CachedAdvisorCount = -1;
 int32 CachedAdvisorSelected = INDEX_NONE;
 int32 CachedAdvisorSelectedCode = 0;
 int32 CachedAdvisorWeapon = 0;
 int32 CachedAdvisorArmor = 0;
 int32 CachedAdvisorAccessory = 0;
 uint64 CachedAdvisorLoadoutHash = 0;
 uint32 CachedAdvisorInventoryRevision = 0;
 TArray<int32> CachedValidatedEquipmentCodes;
 int32 CachedAdvisorScore = 0;
 TWeakObjectPtr<const AMoonfallHeroPawn> CachedSetPreviewHero;
 uint64 CachedSetPreviewLoadout = 0;
 uint32 CachedSetPreviewRevision = 0;
 int32 CachedSetPreviewCode = 0;
 double SetPreviewUpdatedAt = -1.0;
 FString CachedSetPreviewLine;
 TWeakObjectPtr<const AMoonfallHeroPawn> CachedRingCopiesHero;
 TArray<int32> CachedRingCopiesSource,CachedRingCopiesLoadout;
 TMap<int32,int32> CachedWornRingCopies,CachedOwnedRingCopies;
};

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "MoonfallCombatHUD.generated.h"
class AMoonfallHeroPawn;
class UTexture2D;
UCLASS()
class MOONFALLUNREAL_API AMoonfallCombatHUD : public AHUD
{
 GENERATED_BODY()
public:
 AMoonfallCombatHUD();
 virtual void DrawHUD() override;
private:
 // Painted six-icon atlas, imported to a cooked texture by Build/Import-Loot-Art.py.
 // Native silhouettes remain visible if the artist asset has not been imported.
 UPROPERTY() TObjectPtr<UTexture2D> LootIconAtlas = nullptr;
 // Optional nine-icon atlas expands the three equipment families by trait.
 UPROPERTY() TObjectPtr<UTexture2D> LootVariantAtlas = nullptr;
 // Optional third atlas: nine more relic motifs for epic+ tier-8+ gear.
 UPROPERTY() TObjectPtr<UTexture2D> LootRelicAtlas = nullptr;
 // Highest-end cosmetic art, without introducing a new rarity or save field.
 UPROPERTY() TObjectPtr<UTexture2D> LootMasterworkAtlas = nullptr;
 // A fifth optional atlas for already-Legendary tier-22+ item illustrations.
 UPROPERTY() TObjectPtr<UTexture2D> LootAscendantAtlas = nullptr;
 void DrawLootIcon(int32 Slot, int32 Rarity, int32 Trait, int32 Tier,
  float X, float Y, float Side);

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

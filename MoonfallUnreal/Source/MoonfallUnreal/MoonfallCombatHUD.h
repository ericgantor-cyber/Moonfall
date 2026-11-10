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
 // Current one-bag visible capacity, shared with mouse-wheel bounds.
 int32 GetInventoryVisibleRowCount() const;
 // Read-only hit test for the visible rows of the single scrolling bag.
 // Returns -1 if the pointer is outside a visible equipment row.
 int32 GetInventoryItemAtScreenPosition(const AMoonfallHeroPawn* Hero, float X, float Y) const;
 // C screen: 0 previous, 1 next, 2 equip, 3 upgrade, 4 ascend,
 // 5 reforge, 6 forge five, 7 open bag; -1 outside clickable controls.
 int32 GetCharacterActionAtScreenPosition(const AMoonfallHeroPawn* Hero, float X, float Y) const;
 // Settings: 0..6 previous value, 7..13 next value, 14 close.
 int32 GetOptionsActionAtScreenPosition(const AMoonfallHeroPawn* Hero, float X, float Y) const;
 // B bag: 0 equip, 1 upgrade, 2 ascend, 3 reforge, 4 forge five,
 // 5 salvage, 6 open character, 7 close backpack.
 int32 GetInventoryActionAtScreenPosition(const AMoonfallHeroPawn* Hero, float X, float Y) const;
 // Map and constellation: clickable close, with page controls for talents.
 int32 GetAtlasMenuActionAtScreenPosition(const AMoonfallHeroPawn* Hero, float X, float Y) const;
 // Read-only constellation node hit test, returns -1 outside the nine visible stars.
 int32 GetTalentNodeAtScreenPosition(const AMoonfallHeroPawn* Hero, float X, float Y) const;
 // Visible SORT button hit test, shared with the inventory drawing geometry.
 bool IsInventorySortButtonAtScreenPosition(const AMoonfallHeroPawn* Hero, float X, float Y) const;
private:
 // Throttle the full-bag advisor scan so 9,999 items aren't evaluated on
 // every frame. All resulting advice remains read-only and never equips.
 const AMoonfallHeroPawn* CachedAdvisorHero = nullptr;
 int32 CachedAdvisorIndex = -1;
 int32 CachedAdvisorCount = -1;
 int32 CachedAdvisorWeapon = -1;
 int32 CachedAdvisorArmor = -1;
 int32 CachedAdvisorAccessory = -1;
 int32 CachedAdvisorSortMode = -1;
 int32 CachedAdvisorItemCode = -1;
 float NextAdvisorRefreshAt = -1.f;
 float DisplayHealth = -1.f;
 float DisplayMana = -1.f;
 float DisplayStamina = -1.f;
 float DisplayXP = -1.f;
 int32 LastShownLevel = -1;
 float LevelGlowUntil = 0.f;
};

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
 // Read-only hit test for the currently displayed inventory page.
 // Returns -1 if the pointer is outside a visible equipment row.
 int32 GetInventoryItemAtScreenPosition(const AMoonfallHeroPawn* Hero, float X, float Y) const;
 // Visible SORT button hit test, shared with the inventory drawing geometry.
 bool IsInventorySortButtonAtScreenPosition(const AMoonfallHeroPawn* Hero, float X, float Y) const;
 // Click-to-equip in the wide inspection pane. Never targets a hidden item.
private:
 float DisplayHealth = -1.f;
 float DisplayMana = -1.f;
 float DisplayStamina = -1.f;
 float DisplayXP = -1.f;
 int32 LastShownLevel = -1;
 float LevelGlowUntil = 0.f;
};

#include "MoonfallCombatHUD.h"
#include "MoonfallHeroPawn.h"
#include "MoonfallEnemy.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
int32 AMoonfallCombatHUD::GetInventoryItemAtScreenPosition(
    const AMoonfallHeroPawn* Hero, float X, float Y) const
{
 // Use the same geometry and pagination as DrawHUD. The hit test is read-only
 // and never chooses a hidden or off-page entry.
 if (!Canvas || !IsValid(Hero) || !Hero->bInventoryOpen ||
     Hero->bCharacterStatsOpen || Hero->bOptionsOpen ||
     !FMath::IsFinite(X) || !FMath::IsFinite(Y)) return -1;
 const int32 Count = Hero->InventoryItemCodes.Num();
 if (Count <= 0) return -1;
 const bool bWideInventory = Canvas->SizeX >= 1100.f;
 const float PanelWidth = FMath::Min(bWideInventory ? 1040.f : 680.f, FMath::Max(280.f, Canvas->SizeX - 24.f));
 const float ListWidth = bWideInventory ? PanelWidth - 340.f : PanelWidth;
 const float PanelHeight = FMath::Min(580.f, FMath::Max(280.f, Canvas->SizeY - 24.f));
 const float Left = (Canvas->SizeX - PanelWidth) * 0.5f;
 const float Top = (Canvas->SizeY - PanelHeight) * 0.5f;
 const int32 PerPage = FMath::Clamp(FMath::FloorToInt((PanelHeight - 290.f) / 25.f), 2, 8);
 const int32 Selected = FMath::Clamp(Hero->InventorySelection, 0, Count - 1);
 const int32 PageStart = Selected / PerPage * PerPage;
 const int32 PageEnd = FMath::Min(Count, PageStart + PerPage);
 const float RowSpacing = FMath::Max(21.f, (PanelHeight - 290.f) / PerPage);
 if (X < Left + 12.f || X > Left + ListWidth - 12.f) return -1;
 for (int32 Index = PageStart; Index < PageEnd; ++Index) {
  const float RowTop = Top + 145.f + (Index - PageStart) * RowSpacing - 3.f;
  if (Y >= RowTop && Y < RowTop + RowSpacing - 3.f)
   return Index;
 }
 return -1;
}


bool AMoonfallCombatHUD::IsInventorySortButtonAtScreenPosition(
    const AMoonfallHeroPawn* Hero, float X, float Y) const
{
 if (!Canvas || !IsValid(Hero) || !Hero->bInventoryOpen ||
     Hero->bCharacterStatsOpen || Hero->bOptionsOpen ||
     !FMath::IsFinite(X) || !FMath::IsFinite(Y)) return false;
 const bool bWideInventory = Canvas->SizeX >= 1100.f;
 const float PanelWidth = FMath::Min(bWideInventory ? 1040.f : 680.f,
   FMath::Max(280.f, Canvas->SizeX - 24.f));
 const float ListWidth = bWideInventory ? PanelWidth - 340.f : PanelWidth;
 const float PanelHeight = FMath::Min(580.f, FMath::Max(280.f, Canvas->SizeY - 24.f));
 const float Left = (Canvas->SizeX - PanelWidth) * 0.5f;
 const float Top = (Canvas->SizeY - PanelHeight) * 0.5f;
 // Compact windows put the button on the bottom row to avoid title overlap.
 const float ButtonX = Left + ListWidth - 166.f;
 const float ButtonY = ListWidth >= 500.f ? Top + 12.f : Top + PanelHeight - 109.f;
 return X >= ButtonX && X < ButtonX + 148.f &&
   Y >= ButtonY && Y < ButtonY + 33.f;
}

void AMoonfallCombatHUD::DrawHUD()
{
 Super::DrawHUD();
 if (!Canvas) return;
 const AMoonfallHeroPawn* Hero = Cast<AMoonfallHeroPawn>(UGameplayStatics::GetPlayerPawn(this,0));
 if (!IsValid(Hero)) return;
 if (Hero->bOptionsOpen) {
  const float MenuWidth = FMath::Min(560.f, FMath::Max(240.f, Canvas->SizeX - 24.f));
  const float MenuHeight = FMath::Min(550.f, FMath::Max(280.f, Canvas->SizeY - 24.f));
  const float MenuLeft = Canvas->SizeX * 0.5f - MenuWidth * 0.5f;
  const float MenuTop = Canvas->SizeY * 0.5f - MenuHeight * 0.5f;
  const float MenuRowGap = (MenuHeight - 130.f) / 7.f;
  DrawRect(FLinearColor(0.04f,0.03f,0.11f,0.94f),MenuLeft,MenuTop,MenuWidth,MenuHeight);
  DrawText(TEXT("MOONFALL  |  GRAPHICS & ACCESSIBILITY"),FLinearColor::White,MenuLeft+18.f,MenuTop+17.f,nullptr,1.25f);
  UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
  if (Settings) {
   float ScalePct = 100.f, ScaleNormalized = 0.f, ScaleMin = 50.f, ScaleMax = 100.f;
   Settings->GetResolutionScaleInformationEx(ScalePct, ScaleNormalized, ScaleMin, ScaleMax);
   const FIntPoint Res = Settings->GetScreenResolution();
   const FString Rows[] = {FString::Printf(TEXT("Resolution: %d x %d"),Res.X,Res.Y),FString::Printf(TEXT("Render scale: %.0f%%"),ScalePct),FString::Printf(TEXT("Quality preset: %d (0 Low - 3 Epic)"),Settings->GetOverallScalabilityLevel()),FString::Printf(TEXT("Frame Generation: %s (%s)"),Hero->FrameGenerationPreference==0?TEXT("Off"):Hero->FrameGenerationPreference==1?TEXT("Auto"):TEXT("On"),Hero->bFrameGenerationPluginDetected?TEXT("plugin detected; inactive"):TEXT("plugin unavailable")),FString::Printf(TEXT("Reduced motion: %s"),Hero->bReduceMotion?TEXT("On"):TEXT("Off")),FString::Printf(TEXT("Combat text size: %.0f%%"),Hero->CombatTextScale * 100.f),FString::Printf(TEXT("Boss warning visibility: %.0f%%"),Hero->BossWarningScale * 100.f)};
   for (int32 i=0;i<7;++i) DrawText(Rows[i],i==Hero->OptionsSelection?FLinearColor::Yellow:FLinearColor::White,MenuLeft+20.f,MenuTop+75.f+i*MenuRowGap,nullptr,1.1f);
  }
  DrawText(TEXT("WASD / Arrows / D-pad: navigate    Esc / Start: close"),FLinearColor::White,MenuLeft+18.f,MenuTop+MenuHeight-28.f,nullptr,0.85f);
  return;
 }
 if (Hero->bAtlasOpen) {
  // UE5 Atlas foundation: live world coordinates, no invented discovered
  // terrain, locations, or markers. Future exploration data can fill cells.
  const float Width = FMath::Min(700.f, FMath::Max(280.f, Canvas->SizeX - 24.f));
  const float Height = FMath::Min(650.f, FMath::Max(280.f, Canvas->SizeY - 24.f));
  const float Left = (Canvas->SizeX - Width) * 0.5f;
  const float Top = (Canvas->SizeY - Height) * 0.5f;
  DrawRect(FLinearColor(0.025f,0.035f,0.075f,0.97f),Left,Top,Width,Height);
  DrawText(TEXT("MOONFALL  |  ATLAS"),FLinearColor::White,
    Left + 20.f,Top + 17.f,nullptr,1.3f);
  const FVector WorldPos = Hero->GetActorLocation();
  const bool bValidWorldPos = FMath::IsFinite(WorldPos.X) &&
    FMath::IsFinite(WorldPos.Y) && FMath::IsFinite(WorldPos.Z);
  const float MapSize = FMath::Max(140.f, FMath::Min(Width - 48.f, Height - 150.f));
  const float MapLeft = Left + (Width - MapSize) * 0.5f;
  const float MapTop = Top + 61.f;
  DrawRect(FLinearColor(0.085f,0.12f,0.17f),MapLeft,MapTop,MapSize,MapSize);
  for (int32 GridIndex = 1; GridIndex < 10; ++GridIndex) {
   const float Offset = MapSize * GridIndex / 10.f;
   DrawRect(FLinearColor(0.20f,0.28f,0.35f,0.50f),MapLeft + Offset,MapTop,1.f,MapSize);
   DrawRect(FLinearColor(0.20f,0.28f,0.35f,0.50f),MapLeft,MapTop + Offset,MapSize,1.f);
  }
  if (bValidWorldPos) {
   const float CenterX = MapLeft + MapSize * 0.5f;
   const float CenterY = MapTop + MapSize * 0.5f;
   DrawRect(FLinearColor(0.98f,0.82f,0.25f),CenterX - 5.f,CenterY - 5.f,10.f,10.f);
   DrawText(TEXT("YOU"),FLinearColor::Yellow,CenterX + 9.f,CenterY - 8.f,nullptr,0.85f);
   DrawText(FString::Printf(TEXT("World X %.0f   Y %.0f   Z %.0f"),
     WorldPos.X,WorldPos.Y,WorldPos.Z),FLinearColor(0.8f,0.9f,1.f),
     Left + 20.f,Top + Height - 76.f,nullptr,0.9f);
  } else {
   DrawText(TEXT("World coordinates unavailable"),FLinearColor::White,
      Left + 20.f,Top + Height - 76.f,nullptr,0.9f);
  }
  DrawText(TEXT("Undiscovered terrain is not shown  |  J / Esc: close"),
    FLinearColor(0.7f,0.85f,0.88f),Left + 20.f,Top + Height - 43.f,nullptr,0.78f);
  return;
 }
 if (Hero->bInventoryOpen) {
  // Canvas-only paginated inventory: no extra actors, widgets, or hidden controls.
  const bool bWideInventory = Canvas->SizeX >= 1100.f;
 const float PanelWidth = FMath::Min(bWideInventory ? 1040.f : 680.f, FMath::Max(280.f, Canvas->SizeX - 24.f));
 const float ListWidth = bWideInventory ? PanelWidth - 340.f : PanelWidth;
  const float PanelHeight = FMath::Min(580.f, FMath::Max(280.f, Canvas->SizeY - 24.f));
  const float Left = (Canvas->SizeX - PanelWidth) * 0.5f;
  const float Top = (Canvas->SizeY - PanelHeight) * 0.5f;
  DrawRect(FLinearColor(0.04f, 0.035f, 0.10f, 0.96f), Left, Top, PanelWidth, PanelHeight);
  DrawText(TEXT("MOONFALL  |  INVENTORY"), FLinearColor::White, Left + 18.f, Top + 16.f, nullptr, 1.3f);
  // Explicit clickable control, with a matching hit region above.
  // No sorting happens during DrawHUD; only a deliberate click changes mode.
  if (!Hero->bCharacterStatsOpen) {
   const float ButtonX = Left + ListWidth - 166.f;
   const float ButtonY = ListWidth >= 500.f ? Top + 12.f : Top + PanelHeight - 109.f;
   DrawRect(FLinearColor(0.13f,0.34f,0.43f,0.97f),ButtonX,ButtonY,148.f,33.f);
   const TCHAR* NextSort = Hero->InventorySortMode == 1 ? TEXT("SLOT")
     : TEXT("QUALITY");
   DrawText(FString::Printf(TEXT("SORT: %s"),NextSort),
     FLinearColor::White,ButtonX + 10.f,ButtonY + 7.f,nullptr,0.88f);
  }

  if (Hero->bCharacterStatsOpen) {
   // Character sheet is a read-only page. Inventory operations remain locked
   // while it is open; controller and keyboard switch pages with the same input.
   DrawText(TEXT("MOONFALL  |  CHARACTER"), FLinearColor::White,
     Left + 18.f, Top + 17.f, nullptr, 1.35f);
   DrawText(FString::Printf(TEXT("Level %d  |  XP %d / %d  |  Salvage %d"),
      Hero->Level, Hero->Experience, Hero->ExperienceToNextLevel, Hero->Salvage),
      FLinearColor(0.78f,0.83f,1.f), Left + 18.f, Top + 57.f, nullptr, 0.95f);
   const float RowHeight = FMath::Clamp((PanelHeight - 145.f) / 11.f, 13.f, 36.f);
   const float StartY = Top + 96.f;
   const FString CharacterLines[] = {
     TEXT("PRIMARY ATTRIBUTES"),
     FString::Printf(TEXT("Might: %d    |    +%.1f%% hostile spell power"),
       Hero->GetMight(), FMath::Min(Hero->GetMight() * 0.1f, 10.f)),
     FString::Printf(TEXT("Insight: %d    |    +%.1f%% mana regeneration"),
       Hero->GetInsight(), FMath::Min(Hero->GetInsight() * 0.2f, 20.f)),
     FString::Printf(TEXT("Resolve: %d    |    +%.1f%% stamina regeneration"),
       Hero->GetResolve(), FMath::Min(Hero->GetResolve() * 0.3f, 30.f)),
     TEXT("SECONDARY STATS"),
     FString::Printf(TEXT("Critical Strike: %.1f%%  |  Haste: %.1f%%"),
       Hero->GetCriticalChance() * 100.f, Hero->GetHasteBonus() * 100.f),
     FString::Printf(TEXT("Mastery: %.1f%%  |  Versatility: %.1f%%"),
       Hero->GetMasteryBonus() * 100.f, Hero->GetVersatilityBonus() * 100.f),
     TEXT("EQUIPMENT SET BONUSES"),
     FString::Printf(TEXT("Force %d/3: 2pc +3%% crit; 3pc +4%% mastery"),
       Hero->GetSetPieceCount(1)),
     FString::Printf(TEXT("Ward %d/3: 2pc +3%% vers; 3pc +2.5%% protection"),
       Hero->GetSetPieceCount(2)),
     FString::Printf(TEXT("Focus %d/3: 2pc +4%% haste; 3pc +6%% mana regen"),
       Hero->GetSetPieceCount(3))
   };
   for (int32 Row = 0; Row < UE_ARRAY_COUNT(CharacterLines); ++Row)
    DrawText(CharacterLines[Row],
      Row == 0 || Row == 4 || Row == 7 ? FLinearColor(0.52f,0.91f,0.76f) : FLinearColor::White,
      Left + 18.f, StartY + Row * RowHeight, nullptr,
      PanelHeight < 360.f ? 0.55f : (Canvas->SizeX < 900.f ? 0.75f : 0.90f));
   DrawText(TEXT("C / A: close character info    B / Select / Esc: close"),
     FLinearColor(0.81f,0.83f,0.97f), Left + 18.f, Top + PanelHeight - 29.f,
     nullptr, 0.8f);
   return;
  }
  const int32 Count = Hero->InventoryItemCodes.Num();
  // Reserve height for a real equipment-stat line and three controls rows.
  const int32 PerPage = FMath::Clamp(FMath::FloorToInt((PanelHeight - 290.f) / 25.f), 2, 8);
  const int32 Selected = FMath::Clamp(Hero->InventorySelection, 0, FMath::Max(0, Count - 1));
  const int32 PageStart = Selected / PerPage * PerPage;
  const int32 PageEnd = FMath::Min(Count, PageStart + PerPage);
  // Local advisor reads inventory and current set bonuses, never mutates gear.
  // Show the inventory index even when its recommendation is on another page.
  const int32 AdvisedIndex = Hero->GetRecommendedEquipmentIndex();
  const int32 SelectedImprovement = Hero->GetEquipmentUpgradeScore(Selected);
  DrawText(AdvisedIndex >= 0
    ? FString::Printf(TEXT("ADVISOR: item %d  (+%d points)  |  equip manually"),
        AdvisedIndex + 1, Hero->GetEquipmentUpgradeScore(AdvisedIndex))
    : Count == 0 ? TEXT("ADVISOR: no items to compare")
      : TEXT("ADVISOR: no higher-scoring gear found"),
    FLinearColor(0.54f,0.94f,0.76f), Left + 18.f, Top + 128.f,
    nullptr, Canvas->SizeX < 900.f ? 0.64f : 0.75f);
  const bool bBagFull = Count >= AMoonfallHeroPawn::MaxInventoryItems;
  const TCHAR* SortName = Hero->InventorySortMode == 1 ? TEXT("Quality")
    : (Hero->InventorySortMode == 2 ? TEXT("Slot") : TEXT("Pickup"));
  DrawText(FString::Printf(TEXT("%d / %d items  |  %d salvage  |  page %d / %d  |  %s"),
    Count, AMoonfallHeroPawn::MaxInventoryItems, Hero->Salvage,
    PageStart / PerPage + 1, FMath::Max(1, (Count + PerPage - 1) / PerPage), SortName),
    bBagFull ? FLinearColor(1.f,0.45f,0.35f) : FLinearColor(0.75f,0.72f,0.91f),
    Left + 18.f, Top + 49.f, nullptr, 0.9f);
  if (bBagFull && !bWideInventory)
   DrawText(TEXT("BAG FULL - salvage unused gear to collect more loot"),
     FLinearColor(1.f,0.45f,0.35f), Left + 18.f, Top + 83.f,
     nullptr, Canvas->SizeX < 900.f ? 0.65f : 0.82f);
  if (Count == 0)
   DrawText(TEXT("No equipment yet. Defeat enemies to find gear."), FLinearColor::White,
     Left + 18.f, Top + 100.f, nullptr, 0.92f);
  const float RowSpacing = FMath::Max(21.f, (PanelHeight - 290.f) / PerPage);
  for (int32 Index = PageStart; Index < PageEnd; ++Index) {
   const int32 Code = Hero->InventoryItemCodes[Index];
   const int32 Affix = Code / 100000;
   const int32 Slot = (Code / 10000) % 10;
   const int32 Rarity = (Code / 1000) % 10;
   const int32 Tier = Code % 1000;
   const TCHAR* Slots[] = {TEXT("Weapon"), TEXT("Armor"), TEXT("Accessory")};
   const TCHAR* Rarities[] = {TEXT("Common"), TEXT("Uncommon"), TEXT("Rare"), TEXT("Epic"), TEXT("Emerald"), TEXT("Legendary")};
   const TCHAR* Traits[] = {TEXT("None"), TEXT("Force"), TEXT("Ward"), TEXT("Focus")};
   const bool bValid = Code > 0 && Slot >= 0 && Slot <= 2
     && Affix >= 0 && Affix <= 3 && Rarity >= 0 && Rarity <= 5
     && Tier >= 1 && Tier <= 25 && (Rarity != 0 || Affix == 0)
     && Code == Affix * 100000 + Slot * 10000 + Rarity * 1000 + Tier;
   const bool bEquipped = bValid && Code == (Slot == 0 ? Hero->EquippedWeaponCode
     : Slot == 1 ? Hero->EquippedArmorCode : Hero->EquippedAccessoryCode);
   const bool bAdvised = Index == AdvisedIndex;
   const float Y = Top + 145.f + (Index - PageStart) * RowSpacing;
   if (Index == Selected) DrawRect(FLinearColor(0.25f,0.18f,0.43f,0.90f),
     Left + 12.f, Y - 3.f, ListWidth - 24.f, RowSpacing - 3.f);
   const FString Row = bValid ? FString::Printf(TEXT("%02d  %s  T%d  %s  %s%s%s"),
      Index + 1, Slots[Slot], Tier, Rarities[Rarity], Traits[Affix],
      bEquipped ? TEXT(" [EQUIPPED]") : TEXT(""),
      bAdvised ? TEXT(" [ADVISOR]") : TEXT(""))
    : FString::Printf(TEXT("%02d  Invalid item"), Index + 1);
   // Item names retain a clear text label plus a distinct quality color.
   const FLinearColor RarityColors[] = {FLinearColor(0.90f,0.90f,0.90f),
     FLinearColor(0.25f,0.69f,1.f), FLinearColor(0.96f,0.79f,0.25f),
     FLinearColor(0.72f,0.35f,0.96f), FLinearColor(0.23f,0.84f,0.42f),
     FLinearColor(1.f,0.52f,0.12f)};
   DrawText(Row, Index == Selected ? FLinearColor::Yellow
     : (bValid ? RarityColors[Rarity] : FLinearColor::White),
     Left + 20.f, Y, nullptr, Canvas->SizeX < 900.f ? 0.75f : 0.9f);
  }
  // Expanded two-column inventory on desktop; compact layout remains on small screens.
  // Inspection only: viewing an item never mutates gear or materials.
  if (bWideInventory) {
   const float DetailLeft = Left + ListWidth + 10.f;
   const float DetailTop = Top + 43.f;
   DrawRect(FLinearColor(0.075f,0.065f,0.15f,0.98f),
     DetailLeft, DetailTop, 312.f, PanelHeight - 94.f);
   DrawText(TEXT("GEAR INSPECTION"), FLinearColor(0.56f,0.93f,0.80f),
     DetailLeft + 14.f, DetailTop + 13.f, nullptr, 1.08f);
   if (Hero->InventoryItemCodes.IsValidIndex(Selected)) {
    const int32 Code = Hero->InventoryItemCodes[Selected];
    const int32 Affix = Code / 100000;
    const int32 Slot = (Code / 10000) % 10;
    const int32 Rarity = (Code / 1000) % 10;
    const int32 Tier = Code % 1000;
    const bool bValid = Code > 0 && Slot >= 0 && Slot <= 2
      && Affix >= 0 && Affix <= 3 && Rarity >= 0 && Rarity <= 5
      && Tier >= 1 && Tier <= 25 && (Rarity != 0 || Affix == 0)
      && Code == Affix * 100000 + Slot * 10000 + Rarity * 1000 + Tier;
    if (bValid) {
     const TCHAR* Slots[] = {TEXT("Weapon"),TEXT("Armor"),TEXT("Accessory")};
     const TCHAR* Rarities[] = {TEXT("Common"),TEXT("Uncommon"),TEXT("Rare"),
       TEXT("Epic"),TEXT("Emerald"),TEXT("Legendary")};
     const TCHAR* Traits[] = {TEXT("None"),TEXT("Force"),TEXT("Ward"),TEXT("Focus")};
     const int32 Equipped = Slot == 0 ? Hero->EquippedWeaponCode
       : (Slot == 1 ? Hero->EquippedArmorCode : Hero->EquippedAccessoryCode);
     const int32 EquippedTier = Equipped % 1000;
     const int32 EquippedRarity = (Equipped / 1000) % 10;
     const int32 SalvageReward = 1 + Rarity * 2 + Tier / 5;
     const int32 UpgradeCost = 4 + Tier * 2 + Rarity * 3;
     const int32 AscendCost = 20 + Tier * 3 + Rarity * 15;
     const int32 ReforgeCost = 12 + Tier * 2 + Rarity * 8;
     const float TextX = DetailLeft + 14.f;
     const FLinearColor DetailColor(0.85f,0.89f,1.f);
     DrawText(FString::Printf(TEXT("%s  /  %s"),Slots[Slot],Rarities[Rarity]),
       FLinearColor(1.f,0.83f,0.46f),TextX,DetailTop + 50.f,nullptr,0.91f);
     DrawText(FString::Printf(TEXT("Tier %d / 25  |  Trait: %s"),Tier,Traits[Affix]),
       DetailColor,TextX,DetailTop + 77.f,nullptr,0.87f);
     DrawText(Code == Equipped ? TEXT("CURRENTLY EQUIPPED") : TEXT("IN BACKPACK"),
       Code == Equipped ? FLinearColor(0.48f,0.96f,0.65f) : DetailColor,
       TextX,DetailTop + 104.f,nullptr,0.85f);
     DrawText(FString::Printf(TEXT("Worn: Tier %d  /  rarity %d"),
       EquippedTier,EquippedRarity),DetailColor,TextX,DetailTop + 137.f,nullptr,0.85f);
     DrawText(FString::Printf(TEXT("Advisor: %+d points"),
       Hero->GetEquipmentUpgradeScore(Selected)),DetailColor,
       TextX,DetailTop + 164.f,nullptr,0.85f);
     DrawText(TEXT("CRAFTING & SALVAGE"),FLinearColor(0.56f,0.93f,0.80f),
       TextX,DetailTop + 200.f,nullptr,0.93f);
     DrawText(FString::Printf(TEXT("Salvage value: %d"),SalvageReward),
       DetailColor,TextX,DetailTop + 227.f,nullptr,0.85f);
     DrawText(Tier < 25 ? FString::Printf(TEXT("Upgrade: %d salvage"),UpgradeCost)
       : TEXT("Upgrade: MAX TIER"),DetailColor,TextX,DetailTop + 254.f,nullptr,0.85f);
     DrawText(Rarity < 5 ? FString::Printf(TEXT("Ascend: %d salvage"),AscendCost)
       : TEXT("Ascend: MAX RARITY"),DetailColor,TextX,DetailTop + 281.f,nullptr,0.85f);
     DrawText(Rarity >= 2 ? FString::Printf(TEXT("Reforge: %d salvage"),ReforgeCost)
       : TEXT("Reforge: Rare+ only"),DetailColor,TextX,DetailTop + 308.f,nullptr,0.85f);
     DrawText(TEXT("Actions are manual; no auto-equip."),
       FLinearColor(0.69f,0.75f,0.85f),TextX,DetailTop + 339.f,nullptr,0.73f);
    } else {
     DrawText(TEXT("Invalid equipment code"),
       FLinearColor(1.f,0.45f,0.35f),DetailLeft + 14.f,
       DetailTop + 55.f,nullptr,0.9f);
    }
   } else {
    DrawText(TEXT("Select an item to inspect gear."),
      FLinearColor::White,DetailLeft + 14.f,DetailTop + 55.f,nullptr,0.82f);
   }
   if (bBagFull)
    DrawText(TEXT("BAG FULL: salvage spare gear"),
      FLinearColor(1.f,0.45f,0.35f),DetailLeft + 14.f,
      DetailTop + PanelHeight - 132.f,nullptr,0.76f);
  }
  // The selected item's score is advisory only; it never calls EquipInventoryItem.
  if (Count > 0)
   DrawText(SelectedImprovement > 0
     ? FString::Printf(TEXT("Selected: +%d advisor points vs worn slot"), SelectedImprovement)
     : TEXT("Selected: no advised improvement vs worn slot"),
     SelectedImprovement > 0 ? FLinearColor(0.56f,0.94f,0.75f)
       : FLinearColor(0.77f,0.77f,0.85f),
     Left + 18.f, Top + PanelHeight - 178.f, nullptr,
     Canvas->SizeX < 900.f ? 0.65f : 0.75f);
  // Live stat summary mirrors the modifiers applied in combat and recovery.
  const int32 WeaponRarity = FMath::Clamp((Hero->EquippedWeaponCode / 1000) % 10, 0, 5);
  const int32 ArmorRarity = FMath::Clamp((Hero->EquippedArmorCode / 1000) % 10, 0, 5);
  const int32 AccessoryRarity = FMath::Clamp((Hero->EquippedAccessoryCode / 1000) % 10, 0, 5);
  const float WeaponBonusPct = 100.f * (FMath::Clamp(0.018f * Hero->WeaponTier, 0.f, 0.35f)
    + (Hero->WeaponAffix == 1 ? 0.06f : 0.f) + WeaponRarity * 0.015f);
  const float ArmorMitigationPct = 100.f * FMath::Min(0.34f, FMath::Min(0.28f, Hero->ArmorTier * 0.01f)
    + (Hero->ArmorAffix == 2 ? 0.06f : 0.f) + ArmorRarity * 0.005f
    + Hero->GetVersatilityBonus() * 0.5f
    + (Hero->GetSetPieceCount(2) >= 3 ? 0.025f : 0.f));
  const float ManaRegenBonusPct = 100.f * (FMath::Clamp(Hero->AccessoryTier * 0.012f, 0.f, 0.25f)
    + (Hero->AccessoryAffix == 3 ? 0.08f : 0.f) + AccessoryRarity * 0.02f
    + (Hero->GetSetPieceCount(3) >= 3 ? 0.06f : 0.f));
  DrawText(FString::Printf(TEXT("Equipped: +%.1f%% spell damage  |  %.1f%% protection  |  +%.1f%% mana regen"),
    WeaponBonusPct, ArmorMitigationPct, ManaRegenBonusPct),
    FLinearColor(0.76f,0.90f,0.99f), Left + 18.f, Top + 75.f,
    nullptr, Canvas->SizeX < 900.f ? 0.62f : 0.74f);
  // Secondary stats and equipped-set thresholds always reflect active gear.
  DrawText(FString::Printf(TEXT("Crit %.1f%%   Haste %.1f%%   Mastery %.1f%%   Versatility %.1f%%"),
    Hero->GetCriticalChance() * 100.f, Hero->GetHasteBonus() * 100.f,
    Hero->GetMasteryBonus() * 100.f, Hero->GetVersatilityBonus() * 100.f),
    FLinearColor(0.81f,0.89f,1.f), Left + 18.f, Top + 94.f,
    nullptr, Canvas->SizeX < 900.f ? 0.63f : 0.76f);
  const int32 ForcePieces = Hero->GetSetPieceCount(1);
  const int32 WardPieces = Hero->GetSetPieceCount(2);
  const int32 FocusPieces = Hero->GetSetPieceCount(3);
  DrawText(FString::Printf(TEXT("Sets: Force %d/3  Ward %d/3  Focus %d/3    2pc + 3pc unlock bonuses"),
    ForcePieces, WardPieces, FocusPieces), FLinearColor(0.59f,0.95f,0.77f),
    Left + 18.f, Top + 113.f, nullptr, Canvas->SizeX < 900.f ? 0.61f : 0.72f);
  DrawText(TEXT("F8 / L3: sort gear  |  Left-click: select  |  Up/Down: browse  |  Right-click / Enter / Y: equip"),
     FLinearColor(0.8f,0.93f,0.98f), Left + 18.f, Top + PanelHeight - 73.f, nullptr, 0.72f);
  DrawText(TEXT("Left-click: select  Up/Down: browse  Left/Right: skip 5  Right-click / Enter / Y: equip  Delete / X: salvage"),
     FLinearColor::White, Left + 18.f, Top + PanelHeight - 51.f, nullptr, 0.75f);
  // Display cost for the actual selected item, rather than a formula
  // players must calculate while navigating the inventory.
  if (Hero->InventoryItemCodes.IsValidIndex(Selected)) {
   const int32 SelectedCode = Hero->InventoryItemCodes[Selected];
   const int32 SelectedTier = SelectedCode % 1000;
   const int32 SelectedRarity = (SelectedCode / 1000) % 10;
   const int32 SelectedSlot = (SelectedCode / 10000) % 10;
   const int32 SelectedAffix = SelectedCode / 100000;
   const bool bForgeable = SelectedCode > 0 && SelectedTier >= 1 && SelectedTier < 25
     && SelectedSlot >= 0 && SelectedSlot <= 2
     && SelectedRarity >= 0 && SelectedRarity <= 5
     && SelectedAffix >= 0 && SelectedAffix <= 3
     && (SelectedRarity != 0 || SelectedAffix == 0)
     && SelectedCode == SelectedAffix * 100000 + SelectedSlot * 10000
       + SelectedRarity * 1000 + SelectedTier;
   const bool bAscendable = SelectedCode > 0 && SelectedTier >= 1 && SelectedTier <= 25
     && SelectedSlot >= 0 && SelectedSlot <= 2 && SelectedRarity >= 0 && SelectedRarity < 5
     && SelectedAffix >= 0 && SelectedAffix <= 3
     && (SelectedRarity != 0 || SelectedAffix == 0)
     && SelectedCode == SelectedAffix * 100000 + SelectedSlot * 10000
       + SelectedRarity * 1000 + SelectedTier;
   const bool bReforgeable = SelectedCode > 0 && SelectedTier >= 1 && SelectedTier <= 25
     && SelectedSlot >= 0 && SelectedSlot <= 2 && SelectedRarity >= 2 && SelectedRarity <= 5
     && SelectedAffix >= 0 && SelectedAffix <= 3
     && SelectedCode == SelectedAffix * 100000 + SelectedSlot * 10000
       + SelectedRarity * 1000 + SelectedTier;
   const int32 ReforgeCost = 12 + SelectedTier * 2 + SelectedRarity * 8;
   DrawText(bReforgeable
     ? FString::Printf(TEXT("Reforge trait: %d salvage  [%s]"),
         ReforgeCost, Hero->Salvage >= ReforgeCost ? TEXT("READY") : TEXT("NEED MORE"))
     : TEXT("Reforge trait: requires Rare or better"),
     bReforgeable && Hero->Salvage >= ReforgeCost
       ? FLinearColor(0.55f,0.92f,0.75f) : FLinearColor(0.96f,0.66f,0.46f),
     Left + 18.f, Top + PanelHeight - 156.f, nullptr, 0.70f);
   const int32 AscendCost = 20 + SelectedTier * 3 + SelectedRarity * 15;
   DrawText(bAscendable
     ? FString::Printf(TEXT("Ascend rarity: %d salvage  [%s]"),
         AscendCost, Hero->Salvage >= AscendCost ? TEXT("READY") : TEXT("NEED MORE"))
     : TEXT("Ascend rarity: LEGENDARY / invalid"),
     bAscendable && Hero->Salvage >= AscendCost
       ? FLinearColor(0.55f,0.92f,0.75f) : FLinearColor(0.96f,0.66f,0.46f),
     Left + 18.f, Top + PanelHeight - 135.f, nullptr, 0.74f);
   const int32 NextCost = 4 + SelectedTier * 2 + SelectedRarity * 3;
   const FString ForgeStatus = !bForgeable ? TEXT("Crafting: MAX TIER or invalid")
     : FString::Printf(TEXT("Next tier: %d salvage  [%s]"),
       NextCost, Hero->Salvage >= NextCost ? TEXT("READY") : TEXT("NEED MORE"));
   DrawText(ForgeStatus, bForgeable && Hero->Salvage >= NextCost
     ? FLinearColor(0.50f,0.95f,0.62f) : FLinearColor(0.98f,0.67f,0.45f),
     Left + 18.f, Top + PanelHeight - 89.f, nullptr, 0.82f);
  }
  DrawText(TEXT("LT: equip best   U / RT: +1 tier   H / RB: forge 5   R / B: ascend   G / LB: reforge"),
     FLinearColor(0.78f,0.90f,0.95f), Left + 18.f, Top + PanelHeight - 70.f, nullptr, 0.74f);
  DrawText(TEXT("C / A: character info    B / Select / Esc: close"), FLinearColor::White,
     Left + 18.f, Top + PanelHeight - 28.f, nullptr, 0.8f);
  return;
 }
 // Nearest living boss, displayed without creating additional widget actors.
 const AMoonfallEnemy* Boss = nullptr;
 const AMoonfallEnemy* PostureTarget = nullptr;
 float BossDistanceSq = TNumericLimits<float>::Max();
 float PostureDistanceSq = FMath::Square(1200.f);
 for (TActorIterator<AMoonfallEnemy> It(GetWorld()); It; ++It) {
  const AMoonfallEnemy* E = *It;
  if (!IsValid(E) || E->Health <= 0.f) continue;
  const float DistSq = FVector::DistSquared(E->GetActorLocation(), Hero->GetActorLocation());
  if (E->bBoss) {
   if (DistSq < BossDistanceSq) { Boss = E; BossDistanceSq = DistSq; }
  } else if (DistSq < PostureDistanceSq && (E->Posture > 0.f
      || E->PostureBrokenUntil > GetWorld()->GetTimeSeconds())) {
   PostureTarget = E;
   PostureDistanceSq = DistSq;
  }
 }
 if (Boss && BossDistanceSq < FMath::Square(3000.f)) {
  const float Width = FMath::Min(480.f, Canvas->SizeX * 0.68f);
  const float X = (Canvas->SizeX - Width) * 0.5f;
  const float Y = 72.f;
  const float MaxHP = FMath::Max(Boss->Health, Boss->BossMaxHealth);
  const float Percent = FMath::Clamp(Boss->Health / FMath::Max(1.f, MaxHP), 0.f, 1.f);
  DrawRect(FLinearColor(0.07f,0.04f,0.10f,0.94f),X-6.f,Y-30.f,Width+12.f,88.f);
  DrawRect(FLinearColor(0.19f,0.10f,0.16f),X,Y,Width,16.f);
  DrawRect(Boss->BossPhase >= 3 ? FLinearColor(0.86f,0.18f,0.67f) : FLinearColor(0.84f,0.15f,0.22f),X,Y,Width*Percent,16.f);
  DrawText(FString::Printf(TEXT("BOSS  |  PHASE %d  |  %.0f / %.0f HP"),Boss->BossPhase,Boss->Health,MaxHP),FLinearColor::White,X,Y-25.f,nullptr,0.95f);
  const bool bBossPostureBroken = Boss->PostureBrokenUntil > GetWorld()->GetTimeSeconds();
  const float BossPostureRatio = FMath::Clamp(Boss->Posture / FMath::Max(1.f, Boss->MaxPosture), 0.f, 1.f);
  DrawRect(FLinearColor(0.15f,0.14f,0.24f),X,Y+24.f,Width,7.f);
  DrawRect(bBossPostureBroken ? FLinearColor(1.f,0.77f,0.30f) : FLinearColor(0.84f,0.59f,1.f),
    X,Y+24.f,Width*(bBossPostureBroken?1.f:BossPostureRatio),7.f);
  DrawText(bBossPostureBroken ? TEXT("POSTURE SHATTERED") : TEXT("POSTURE"),
    bBossPostureBroken ? FLinearColor(1.f,0.84f,0.48f) : FLinearColor(0.85f,0.79f,1.f),
    X,Y+35.f,nullptr,0.78f);
  if (Boss->BossWarningEndTime > GetWorld()->GetTimeSeconds())
   DrawText(TEXT("INCOMING ATTACK - DODGE"),FLinearColor(1.f,0.83f,0.24f),X,Y+55.f,nullptr,0.9f);
 }
 // Show only the nearest engaged non-boss posture target to avoid clutter.
 if (PostureTarget && GetOwningPlayerController()) {
  FVector2D Screen;
  if (GetOwningPlayerController()->ProjectWorldLocationToScreen(
      PostureTarget->GetActorLocation() + FVector(0.f,0.f,175.f),Screen)
      && Screen.X >= 0.f && Screen.X + 80.f < Canvas->SizeX
      && Screen.Y >= 0.f && Screen.Y + 18.f < Canvas->SizeY) {
   const bool bShattered = PostureTarget->PostureBrokenUntil > GetWorld()->GetTimeSeconds();
   const float Ratio = FMath::Clamp(PostureTarget->Posture /
       FMath::Max(1.f, PostureTarget->MaxPosture),0.f,1.f);
   DrawRect(FLinearColor(0.12f,0.11f,0.19f),Screen.X,Screen.Y,80.f,5.f);
   DrawRect(bShattered ? FLinearColor(1.f,0.76f,0.24f) : FLinearColor(0.78f,0.47f,0.98f),
     Screen.X,Screen.Y,80.f*(bShattered?1.f:Ratio),5.f);
   if (bShattered) DrawText(TEXT("SHATTERED"),FLinearColor(1.f,0.83f,0.48f),
     Screen.X,Screen.Y+7.f,nullptr,0.75f);
  }
 }
 // Compact circular resource HUD at bottom left, with actual gameplay values.
 const float Radius = FMath::Clamp(FMath::Min(Canvas->SizeX, Canvas->SizeY) * 0.092f, 50.f, 80.f);
 const float CX = Radius + 25.f;
 const float CY = Canvas->SizeY - Radius - 25.f;
 const float LabelScale = FMath::Clamp(Radius / 65.f, 0.78f, 1.16f);
 const bool bCompactScreen = Canvas->SizeX < 760.f || Canvas->SizeY < 480.f;
 const float ActualHealth = FMath::Clamp(Hero->Health / FMath::Max(1.f, Hero->MaxHealth), 0.f, 1.f);
 const float ActualMana = FMath::Clamp(Hero->Mana / FMath::Max(1.f, Hero->MaxMana), 0.f, 1.f);
 const float ActualStamina = FMath::Clamp(Hero->Stamina / FMath::Max(1.f, Hero->MaxStamina), 0.f, 1.f);
 const float ActualXP = FMath::Clamp(static_cast<float>(Hero->Experience) / FMath::Max(1, Hero->ExperienceToNextLevel), 0.f, 1.f);
 const float Delta = GetWorld() ? FMath::Clamp(GetWorld()->GetDeltaSeconds(), 0.f, 0.1f) : 0.f;
 if (DisplayHealth < 0.f) { DisplayHealth = ActualHealth; DisplayMana = ActualMana; DisplayXP = ActualXP; DisplayStamina = ActualStamina; }
 if (LastShownLevel < 0) LastShownLevel = Hero->Level;
 if (Hero->Level != LastShownLevel) {
  LastShownLevel = Hero->Level;
  DisplayXP = 0.f;
  LevelGlowUntil = GetWorld() ? GetWorld()->GetTimeSeconds() + 1.25f : 0.f;
 }
 DisplayHealth = FMath::FInterpTo(DisplayHealth, ActualHealth, Delta, 9.f);
 DisplayMana = FMath::FInterpTo(DisplayMana, ActualMana, Delta, 9.f);
 DisplayStamina = FMath::FInterpTo(DisplayStamina, ActualStamina, Delta, 11.f);
 DisplayXP = FMath::FInterpTo(DisplayXP, ActualXP, Delta, 7.f);
 const float HealthFraction = DisplayHealth;
 const float ManaFraction = DisplayMana;
 const float XpFraction = DisplayXP;
 // Keep critical health unmistakable without flashing the entire screen.
 const bool bCriticalHealth = Hero->Health > 0.f && ActualHealth <= 0.25f;
 const float LowHealthPulse = bCriticalHealth && GetWorld()
     ? 0.5f + 0.5f * FMath::Sin(GetWorld()->GetTimeSeconds() * 4.f) : 0.f;
 const float InnerRadius = Radius * 0.69f;
 // Fill by horizontal rows to give the orb a true circular silhouette, not a rectangle.
 for (int32 I = -static_cast<int32>(InnerRadius); I <= static_cast<int32>(InnerRadius); I += 2) {
  const float HalfWidth = FMath::Sqrt(FMath::Max(0.f, InnerRadius * InnerRadius - I * I));
  const bool bFilled = static_cast<float>(I) >= InnerRadius * (1.f - 2.f * HealthFraction);
  DrawRect(bFilled ? FLinearColor(0.91f, 0.09f, 0.17f, 0.96f) : FLinearColor(0.16f, 0.04f, 0.09f, 0.88f),
           CX - HalfWidth, CY + I, HalfWidth * 2.f, 2.5f);
 }
 auto DrawArc = [this, CX, CY](float ArcRadius, float Fraction, const FLinearColor& Track, const FLinearColor& Fill, float Thickness) {
  const int32 Segments = FMath::Clamp(FMath::RoundToInt(ArcRadius * 0.9f), 48, 96);
  const float Progress = FMath::Clamp(Fraction, 0.f, 1.f);
  for (int32 I = 0; I < Segments; ++I) {
   const float StartFraction = static_cast<float>(I) / Segments;
   const float EndFraction = static_cast<float>(I + 1) / Segments;
   const float A0 = -PI * 0.5f + 2.f * PI * StartFraction;
   const float A1 = -PI * 0.5f + 2.f * PI * EndFraction;
   const FVector2D Start(CX + FMath::Cos(A0) * ArcRadius, CY + FMath::Sin(A0) * ArcRadius);
   const FVector2D End(CX + FMath::Cos(A1) * ArcRadius, CY + FMath::Sin(A1) * ArcRadius);
   Canvas->K2_DrawLine(Start, End, Thickness, Track);
   if (Progress <= StartFraction) continue;
   const float FilledEnd = FMath::Min(Progress, EndFraction);
   const float AFilled = -PI * 0.5f + 2.f * PI * FilledEnd;
   Canvas->K2_DrawLine(Start,
       FVector2D(CX + FMath::Cos(AFilled) * ArcRadius, CY + FMath::Sin(AFilled) * ArcRadius),
       Thickness, Fill);
  }
 };
 DrawArc(Radius * 0.78f, 1.f, FLinearColor(0.42f, 0.27f, 0.32f), FLinearColor(0.42f, 0.27f, 0.32f), 2.5f);
 if (bCriticalHealth) {
  DrawArc(Radius * 1.055f, 1.f, FLinearColor(0.4f, 0.05f, 0.09f, 0.35f),
          FLinearColor(1.f, 0.10f, 0.16f, 0.35f + LowHealthPulse * 0.45f), 2.5f);
 }
 DrawArc(Radius * 0.85f, ManaFraction, FLinearColor(0.06f, 0.11f, 0.23f), FLinearColor(0.19f, 0.59f, 1.f), 5.f);
 DrawArc(Radius * 0.97f, XpFraction, FLinearColor(0.19f, 0.10f, 0.28f), FLinearColor(0.73f, 0.32f, 1.f), 4.f);
 // Thin green stamina arc stays outside the original red HP / blue mana / purple XP orb.
 const bool bStaminaLow = Hero->Stamina < Hero->DodgeStaminaCost;
 const bool bSprintRecovery = Hero->Stamina < FMath::Clamp(Hero->SprintRestartStamina, 0.f, Hero->MaxStamina);
 DrawArc(Radius * 1.13f, DisplayStamina, FLinearColor(0.06f, 0.17f, 0.10f),
   bStaminaLow ? FLinearColor(1.f, 0.68f, 0.22f) : FLinearColor(0.22f, 0.86f, 0.43f), 3.f);
 if ((bStaminaLow || bSprintRecovery) && !bCompactScreen) {
  DrawText(bStaminaLow ? TEXT("LOW STAMINA") : TEXT("SPRINT RECOVERING"),
    FLinearColor(1.f,0.80f,0.40f),
    CX + Radius + 14.f, CY + 15.f, nullptr, 0.82f * LabelScale);
 }
 const float GlowLeft = GetWorld() ? FMath::Max(0.f, LevelGlowUntil - GetWorld()->GetTimeSeconds()) : 0.f;
 if (GlowLeft > 0.f) {
  const float Intensity = FMath::Clamp(GlowLeft / 1.25f, 0.f, 1.f);
  DrawArc(Radius * 1.055f, 1.f, FLinearColor(0.f,0.f,0.f,0.f),
          FLinearColor(0.85f, 0.48f, 1.f, Intensity), 2.f);
 }
 DrawText(FString::Printf(TEXT("%.0f / %.0f HP"), FMath::Max(0.f, Hero->Health), Hero->MaxHealth),
          bCriticalHealth ? FLinearColor(1.f, 0.95f, 0.95f) : FLinearColor::White,
          CX - Radius * 0.39f, CY - Radius * 0.19f, nullptr, 0.88f * LabelScale);
 DrawText(FString::Printf(TEXT("LV %d"), Hero->Level), FLinearColor(0.91f,0.80f,1.f),
          CX - Radius * 0.26f, CY + Radius * 0.28f, nullptr, 0.73f * LabelScale);
 if (GlowLeft > 0.f) {
  const float Alpha = FMath::Clamp(GlowLeft / 1.25f, 0.f, 1.f);
  DrawText(TEXT("LEVEL UP"), FLinearColor(0.91f, 0.73f, 1.f, Alpha),
           CX + Radius + 14.f, CY - 13.f, nullptr, 0.96f * LabelScale);
 }
 if (!bCompactScreen) {
  // All six item rarities must be represented before indexing the label.
  // A received item is reported independently of the current bag size.
  const TCHAR* RarityNames[] = { TEXT("COMMON"), TEXT("UNCOMMON"),
    TEXT("RARE"), TEXT("EPIC"), TEXT("EMERALD"), TEXT("LEGENDARY") };
  const int32 Rarity = FMath::Clamp(Hero->LastLootRarity, 0,
    static_cast<int32>(UE_ARRAY_COUNT(RarityNames)) - 1);
  DrawText(FString::Printf(TEXT("Gear W:%d A:%d C:%d | Salvage %d | Bag %d"),
   Hero->WeaponTier, Hero->ArmorTier, Hero->AccessoryTier, Hero->Salvage, Hero->InventoryItemCodes.Num()),
   FLinearColor(0.85f,0.84f,0.95f), 24.f, Canvas->SizeY - 75.f, nullptr, 0.8f);
  if (Hero->LastLootTier > 0) {
   DrawText(FString::Printf(TEXT("Last received: %s Tier %d (%+d vs equipped)"),
    RarityNames[Rarity], Hero->LastLootTier, Hero->LastLootUpgrade),
    FLinearColor(0.95f,0.84f,0.47f), 24.f, Canvas->SizeY - 55.f, nullptr, 0.8f);
  }
 }
 if (!bCompactScreen) DrawText(TEXT("ESC  OPTIONS"), FLinearColor(0.73f, 0.70f, 0.83f, 0.85f),
          FMath::Max(16.f, Canvas->SizeX - 142.f), Canvas->SizeY - 37.f, nullptr, 0.82f);
 int32 VisibleDamageNumbers = 0;
 for (TActorIterator<AMoonfallEnemy> It(GetWorld()); It && VisibleDamageNumbers < 32; ++It) {
  const AMoonfallEnemy* Enemy = *It;
  for (int32 Slot = 0; Slot < AMoonfallEnemy::DamageHistorySize && VisibleDamageNumbers < 32; ++Slot) {
   const AMoonfallEnemy::FRecentDamageHit& Hit = Enemy->DamageHistory[Slot];
   const float Age = GetWorld()->GetTimeSeconds() - Hit.Time;
   if (Age < 0.f || Age > 0.8f || !FMath::IsFinite(Hit.Amount) || Hit.Amount <= 0.f) continue;
   FVector2D Screen;
   // Offset entries so consecutive attacks on the same target remain readable.
   const float HorizontalOffset = static_cast<float>(Slot - 1) * 24.f;
   const FVector WorldTextPosition = Enemy->GetActorLocation() + FVector(0.f, 0.f, 175.f + Age * 65.f);
   if (GetOwningPlayerController() && GetOwningPlayerController()->ProjectWorldLocationToScreen(WorldTextPosition, Screen)
       && Screen.X + HorizontalOffset >= 0.f && Screen.X + HorizontalOffset < Canvas->SizeX
       && Screen.Y >= 0.f && Screen.Y < Canvas->SizeY) {
    ++VisibleDamageNumbers;
    const bool bShatter = Hit.bShatter;
    // Lethal hits retain ordinary damage numbers. No KO text overlays the
    // finisher choreography; the combo is communicated through animation.
    const FLinearColor DamageColor = bShatter
      ? FLinearColor(1.f, 0.56f, 0.22f, 1.f - Age / 0.8f)
      : FLinearColor(1.f, 0.84f, 0.2f, 1.f - Age / 0.8f);
    const FString DamageLabel = bShatter
      ? FString::Printf(TEXT("SHATTER -%.0f"), Hit.Amount)
      : FString::Printf(TEXT("-%.0f"), Hit.Amount);
    DrawText(DamageLabel, DamageColor, Screen.X + HorizontalOffset, Screen.Y, nullptr,
      (bShatter ? 1.55f : 1.3f) * Hero->CombatTextScale);
   }
  }
 }
}

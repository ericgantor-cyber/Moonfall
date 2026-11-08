#include "MoonfallCombatHUD.h"
#include "MoonfallHeroPawn.h"
#include "MoonfallEnemy.h"
#include "MoonfallArena.h"
#include "MoonfallQuestGuide.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
void AMoonfallCombatHUD::DrawHUD()
{
 Super::DrawHUD();
 if (!Canvas) return;
 const AMoonfallHeroPawn* Hero = Cast<AMoonfallHeroPawn>(UGameplayStatics::GetPlayerPawn(this,0));
 if (!IsValid(Hero)) return;
 // Moonfall's own Valhalla-style journal layout: no copied art or UMG assets.
 if (Hero->bInventoryOpen && Hero->InventoryMenuTab != 0) {
  const float Width = FMath::Max(280.f, Canvas->SizeX - 32.f);
  const float Height = FMath::Max(270.f, Canvas->SizeY - 32.f);
  const float L = 16.f, T = 16.f;
  DrawRect(FLinearColor(0.025f,0.035f,0.065f,0.97f), L, T, Width, Height);
  DrawRect(FLinearColor(0.73f,0.58f,0.3f,1.f), L + 14.f, T + 51.f, Width - 28.f, 2.f);
  const TCHAR* Tabs[] = {TEXT("GEAR"), TEXT("CHARACTER"), TEXT("QUESTS"), TEXT("WORLD")};
  const float TabSpace = (Width - 32.f) / 4.f;
  for (int32 Tab = 0; Tab < 4; ++Tab) {
   const float X = L + 16.f + Tab * TabSpace;
   if (Hero->InventoryMenuTab == Tab)
    DrawRect(FLinearColor(0.35f,0.27f,0.48f,0.86f), X - 4.f, T + 9.f, TabSpace - 8.f, 37.f);
   DrawText(Tabs[Tab], Hero->InventoryMenuTab == Tab ?
     FLinearColor(1.f,0.87f,0.48f) : FLinearColor(0.68f,0.76f,0.87f),
     X + 3.f, T + 18.f, nullptr, Canvas->SizeX < 900.f ? 0.77f : 1.f);
  }
  const float Left = L + 24.f;
  const float ContentTop = T + 75.f;
  if (Hero->InventoryMenuTab == 1) {
   DrawText(TEXT("CHARACTER  /  POWER"), FLinearColor(1.f,0.88f,0.56f),Left,ContentTop,nullptr,1.28f);
   const FString Rows[] = {
    FString::Printf(TEXT("Level %d    XP %d"), Hero->Level, Hero->Experience),
    FString::Printf(TEXT("Health       %.0f / %.0f"), Hero->Health, Hero->MaxHealth),
    FString::Printf(TEXT("Mana         %.0f / %.0f"), Hero->Mana, Hero->MaxMana),
    FString::Printf(TEXT("Stamina      %.0f / %.0f"), Hero->Stamina, Hero->MaxStamina),
    FString::Printf(TEXT("Spell power  %.2fx"), Hero->SpellPowerMultiplier),
    FString::Printf(TEXT("Equipment   Weapon T%d    Armor T%d    Accessory T%d"),
     Hero->WeaponTier, Hero->ArmorTier, Hero->AccessoryTier),
    FString::Printf(TEXT("Salvage %d    Gear in bag %d"), Hero->Salvage, Hero->InventoryItemCodes.Num())
   };
   const float Gap = FMath::Clamp((Height - 180.f) / 7.f, 24.f, 53.f);
   for (int32 i = 0; i < 7; ++i)
    DrawText(Rows[i], FLinearColor(0.88f,0.9f,0.98f), Left + 10.f,
     ContentTop + 43.f + i * Gap, nullptr, Canvas->SizeX < 850.f ? 0.78f : 1.05f);
  } else if (Hero->InventoryMenuTab == 2) {
   DrawText(TEXT("QUEST JOURNAL  /  KNOWN OBJECTIVES"),FLinearColor(1.f,0.88f,0.56f),
    Left,ContentTop,nullptr,1.12f);
   bool bHasQuests = false;
   for (TActorIterator<AMoonfallArena> It(GetWorld()); It; ++It) {
    const AMoonfallArena* Arena = *It;
    if (!IsValid(Arena) || !Arena->bContinuousWorld) continue;
    const int32 Tracked = Arena->GetTrackedDungeonQuestId();
    DrawText(FString::Printf(TEXT("Guardians defeated: %d / 30"), Arena->GetCompletedDungeonQuestCount()),
      FLinearColor::White,Left + 10.f,ContentTop + 45.f,nullptr,1.f);
    const int32 JournalPageStart = Hero->QuestJournalSelection / 9 * 9;
    int32 KnownIndex = 0, Shown = 0;
    for (int32 Entrance = 0; Entrance < 30 && Shown < 9; ++Entrance) {
     if (!Arena->GetDiscoveredDungeonEntrances().Contains(Entrance)) continue;
     bHasQuests = true;
     const int32 ThisKnownIndex = KnownIndex++;
     if (ThisKnownIndex < JournalPageStart) continue;
     const bool bComplete = Arena->IsDungeonQuestCompleted(Entrance);
     const int32 Progress = Arena->GetDungeonResidentDefeatCount(Entrance);
     const bool bSelected = ThisKnownIndex == Hero->QuestJournalSelection;
     if (bSelected)
      DrawRect(FLinearColor(0.26f,0.23f,0.41f,0.88f), Left+6.f,
       ContentTop + 75.f + Shown * FMath::Clamp((Height - 215.f)/9.f,19.f,38.f),
       Width-75.f, 23.f);
     const FString Line = FString::Printf(TEXT("%s Region %d  /  Dungeon %d     %s"),
       bSelected ? TEXT(">") : Entrance == Tracked ? TEXT("*") : TEXT(" "),
       Entrance / 5 + 1, Entrance % 5 + 1,
       bComplete ? TEXT("COMPLETE") : Progress >= 4 ? TEXT("GUARDIAN") : TEXT("RESIDENTS"));
     DrawText(Line, bComplete ? FLinearColor(0.45f,0.87f,0.55f) :
       bSelected || Entrance == Tracked ? FLinearColor(1.f,0.85f,0.36f) : FLinearColor::White,
       Left + 12.f, ContentTop + 79.f + Shown * FMath::Clamp((Height - 215.f)/9.f,19.f,38.f),
       nullptr,Canvas->SizeX < 850.f ? 0.7f : 0.91f);
     ++Shown;
    }
    break;
   }
   if (!bHasQuests)
    DrawText(TEXT("Explore the world to discover dungeon quests."), FLinearColor::White,
      Left + 12.f,ContentTop + 95.f,nullptr,0.92f);
   DrawText(TEXT("W/S: choose discovered quest   Enter/Y: track"), FLinearColor(0.82f,0.85f,0.95f),
    Left+8.f, T+Height-69.f, nullptr, 0.8f);
  } else if (Hero->InventoryMenuTab == 3) {
   DrawText(TEXT("RAVEN ATLAS  /  EXPLORED WORLD"),FLinearColor(1.f,0.88f,0.56f),
    Left,ContentTop,nullptr,1.14f);
   const float MapLeft = Left + 8.f, MapTop = ContentTop + 40.f;
   const float MapWidth = FMath::Max(200.f, Width - 70.f);
   const float MapHeight = FMath::Max(150.f, Height - 165.f);
   DrawRect(FLinearColor(0.015f,0.025f,0.04f,1.f),MapLeft,MapTop,MapWidth,MapHeight);
   for (TActorIterator<AMoonfallArena> It(GetWorld()); It; ++It) {
    const AMoonfallArena* Arena = *It;
    if (!IsValid(Arena) || !Arena->bContinuousWorld) continue;
    // Deterministic coordinate projection; never draw undiscovered locations.
    const float ScaleX = MapWidth / 41000.f, ScaleY = MapHeight / 17000.f;
    const float CenterX = MapLeft + 35.f, CenterY = MapTop + MapHeight - 40.f;
    for (int32 Region = 0; Region < 6; ++Region) {
     if (!Arena->GetDiscoveredRegions().Contains(Region)) continue;
     const float X = CenterX + Region * 6000.f * ScaleX;
     DrawRect(FLinearColor(0.21f,0.25f,0.35f), X-15.f, CenterY-6.f, 30.f,12.f);
     DrawText(FString::Printf(TEXT("R%d"),Region+1),FLinearColor(0.8f,0.85f,1.f),
      X-9.f,CenterY+12.f,nullptr,0.65f);
    }
    for (const int32 Entrance : Arena->GetDiscoveredDungeonEntrances()) {
     if (Entrance < 0 || Entrance >= 30) continue;
     const float X = CenterX + (Entrance / 5 * 6000.f - 2200.f + (Entrance % 5)*1100.f) * ScaleX;
     const float Y = CenterY - 3600.f * ScaleY;
     DrawRect(Arena->IsDungeonQuestCompleted(Entrance) ?
       FLinearColor(0.32f,0.85f,0.49f) :
       Entrance == Arena->GetTrackedDungeonQuestId() ?
       FLinearColor(1.f,0.89f,0.3f) : FLinearColor(0.88f,0.61f,0.26f),X-4.f,Y-4.f,8.f,8.f);
    }
    for (const int32 Chamber : Arena->GetDiscoveredChambers()) {
     if (Chamber < 0 || Chamber >= 420) continue;
     const int32 Region = Chamber / 70, Dungeon = (Chamber % 70) / 14;
     const float X = CenterX + (Region * 6000.f - 2200.f + Dungeon * 1100.f) * ScaleX;
     const float Y = CenterY - (3600.f + (Chamber % 14)*780.f) * ScaleY;
     DrawRect(FLinearColor(0.55f,0.69f,0.88f),X-2.f,Y-2.f,4.f,4.f);
    }
    const FVector Pos = Hero->GetActorLocation();
    if (FMath::IsFinite(Pos.X) && FMath::IsFinite(Pos.Y))
     DrawRect(FLinearColor(1.f,1.f,1.f),CenterX+Pos.X*ScaleX-4.f,
       CenterY-Pos.Y*ScaleY-4.f,8.f,8.f);
    break;
   }
   DrawText(TEXT("Unexplored regions and dungeons remain hidden."),
     FLinearColor(0.75f,0.81f,0.9f),Left+8.f,T+Height-69.f,nullptr,0.85f);
  }
  DrawText(TEXT("A / D or Left / Right: switch tabs   I / Esc: close"),
    FLinearColor(0.85f,0.85f,0.95f),Left,T+Height-33.f,nullptr,0.82f);
  return;
 }
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
 if (Hero->bInventoryOpen) {
  // Canvas-only paginated inventory: no extra actors, widgets, or hidden controls.
  const float PanelWidth = FMath::Min(680.f, FMath::Max(280.f, Canvas->SizeX - 24.f));
  const float PanelHeight = FMath::Min(580.f, FMath::Max(280.f, Canvas->SizeY - 24.f));
  const float Left = (Canvas->SizeX - PanelWidth) * 0.5f;
  const float Top = (Canvas->SizeY - PanelHeight) * 0.5f;
  DrawRect(FLinearColor(0.04f, 0.035f, 0.10f, 0.96f), Left, Top, PanelWidth, PanelHeight);
  DrawText(TEXT("GEAR  |  CHARACTER  |  QUESTS  |  WORLD"),
    FLinearColor(1.f,0.87f,0.48f), Left + 18.f, Top + 16.f, nullptr,
    Canvas->SizeX < 900.f ? 0.82f : 1.f);
  const int32 Count = Hero->InventoryItemCodes.Num();
  const int32 PerPage = 8;
  const int32 Selected = FMath::Clamp(Hero->InventorySelection, 0, FMath::Max(0, Count - 1));
  const int32 PageStart = Selected / PerPage * PerPage;
  const int32 PageEnd = FMath::Min(Count, PageStart + PerPage);
  DrawText(FString::Printf(TEXT("%d items  |  %d salvage  |  page %d / %d"),
    Count, Hero->Salvage, PageStart / PerPage + 1,
    FMath::Max(1, (Count + PerPage - 1) / PerPage)), FLinearColor(0.75f,0.72f,0.91f),
    Left + 18.f, Top + 49.f, nullptr, 0.9f);
  if (Count == 0)
   DrawText(TEXT("No equipment yet. Defeat enemies to find gear."), FLinearColor::White,
     Left + 18.f, Top + 100.f, nullptr, 0.92f);
  const float RowSpacing = FMath::Max(21.f, (PanelHeight - 142.f) / PerPage);
  for (int32 Index = PageStart; Index < PageEnd; ++Index) {
   const int32 Code = Hero->InventoryItemCodes[Index];
   const int32 Affix = Code / 100000;
   const int32 Slot = (Code / 10000) % 10;
   const int32 Rarity = (Code / 1000) % 10;
   const int32 Tier = Code % 1000;
   const TCHAR* Slots[] = {TEXT("Weapon"), TEXT("Armor"), TEXT("Accessory")};
   const TCHAR* Rarities[] = {TEXT("Common"), TEXT("Uncommon"), TEXT("Rare"), TEXT("Epic")};
   const TCHAR* Traits[] = {TEXT("None"), TEXT("Force"), TEXT("Ward"), TEXT("Focus")};
   const bool bValid = Code > 0 && Slot <= 2 && Affix <= 3 && Rarity <= 3 && Tier >= 1 && Tier <= 25;
   const bool bEquipped = Code == (Slot == 0 ? Hero->EquippedWeaponCode
     : Slot == 1 ? Hero->EquippedArmorCode : Hero->EquippedAccessoryCode);
   const float Y = Top + 83.f + (Index - PageStart) * RowSpacing;
   if (Index == Selected) DrawRect(FLinearColor(0.25f,0.18f,0.43f,0.90f),
     Left + 12.f, Y - 3.f, PanelWidth - 24.f, RowSpacing - 3.f);
   const FString Row = bValid ? FString::Printf(TEXT("%02d  %s  T%d  %s  %s%s"),
      Index + 1, Slots[Slot], Tier, Rarities[Rarity], Traits[Affix], bEquipped ? TEXT("  [EQUIPPED]") : TEXT(""))
    : FString::Printf(TEXT("%02d  Invalid item"), Index + 1);
   DrawText(Row, Index == Selected ? FLinearColor::Yellow : FLinearColor::White,
      Left + 20.f, Y, nullptr, Canvas->SizeX < 900.f ? 0.75f : 0.9f);
  }
  DrawText(TEXT("W / S: select  A / D: tabs  Enter / Y: equip  Delete / X: salvage"),
     FLinearColor::White, Left + 18.f, Top + PanelHeight - 51.f, nullptr, 0.75f);
  DrawText(TEXT("I / Select / Esc: close   K: character   J: quests   M: map"), FLinearColor::White,
     Left + 18.f, Top + PanelHeight - 28.f, nullptr, 0.8f);
  return;
 }
 // Compact north-up Raven Atlas prototype: only physically discovered
 // chambers appear. Nearby unvisited chambers and quest locations remain blank.
 // No additional HUD widget, radar actor or persistent tick allocation.
 for (TActorIterator<AMoonfallArena> ArenaIt(GetWorld()); ArenaIt; ++ArenaIt) {
  const AMoonfallArena* Arena = *ArenaIt;
  if (!IsValid(Arena) || !Arena->bContinuousWorld) continue;
  const float MapWidth = FMath::Clamp(Canvas->SizeX * 0.16f, 120.f, 184.f);
  const float MapLeft = Canvas->SizeX - MapWidth - 18.f;
  const float MapTop = 18.f;
  const float CenterX = MapLeft + MapWidth * 0.5f;
  const float CenterY = MapTop + MapWidth * 0.5f;
  const float MapRadius = MapWidth * 0.44f;
  const float Scale = MapRadius / 4800.f;
  const FVector PlayerPos = Hero->GetActorLocation();
  if (!FMath::IsFinite(PlayerPos.X) || !FMath::IsFinite(PlayerPos.Y)) break;
  DrawRect(FLinearColor(0.025f, 0.035f, 0.07f, 0.9f),
           MapLeft, MapTop, MapWidth, MapWidth);
  // Low-cost terrain silhouettes are revealed only after entering a region.
  // Unvisited regions remain black fog, even if nearby on the same map.
  for (const int32 Region : Arena->GetDiscoveredRegions()) {
   if (Region < 0 || Region >= 6) continue;
   const float DX = (Region * 6000.f - PlayerPos.X) * Scale;
   const float DY = PlayerPos.Y * Scale;
   if (DX * DX + DY * DY > MapRadius * MapRadius) continue;
   const FLinearColor Terrain[] = {
    FLinearColor(0.31f,0.15f,0.12f,0.75f), FLinearColor(0.13f,0.29f,0.19f,0.75f),
    FLinearColor(0.26f,0.38f,0.47f,0.75f), FLinearColor(0.22f,0.24f,0.45f,0.75f),
    FLinearColor(0.36f,0.27f,0.17f,0.75f), FLinearColor(0.27f,0.16f,0.37f,0.75f)
   };
   DrawRect(Terrain[Region], CenterX + DX - 16.f, CenterY + DY - 16.f, 32.f, 32.f);
  }
  // Known dungeon entrances are marked independently of interior discoveries.
  for (const int32 EntranceId : Arena->GetDiscoveredDungeonEntrances()) {
   if (EntranceId < 0 || EntranceId >= 30) continue;
   const int32 Region = EntranceId / 5;
   const int32 Dungeon = EntranceId % 5;
   const float WorldX = Region * 6000.f - 2200.f + Dungeon * 1100.f;
   const float WorldY = 3600.f;
   const float DX = (WorldX - PlayerPos.X) * Scale;
   const float DY = (PlayerPos.Y - WorldY) * Scale;
   if (DX * DX + DY * DY > MapRadius * MapRadius) continue;
   const bool bTracked = EntranceId == Arena->GetTrackedDungeonQuestId();
   const bool bComplete = Arena->IsDungeonQuestCompleted(EntranceId);
   DrawRect(bComplete ? FLinearColor(0.3f, 0.85f, 0.44f)
      : bTracked ? FLinearColor(1.f, 0.94f, 0.39f)
      : FLinearColor(0.96f, 0.67f, 0.27f),
    CenterX + DX - (bTracked ? 4.f : 3.f),
    CenterY + DY - (bTracked ? 4.f : 3.f),
    bTracked ? 8.f : 6.f, bTracked ? 8.f : 6.f);
  }
  for (const int32 RoomId : Arena->GetDiscoveredChambers()) {
   if (RoomId < 0 || RoomId >= 420) continue;
   const int32 Region = RoomId / 70;
   const int32 Dungeon = (RoomId % 70) / 14;
   const int32 Chamber = RoomId % 14;
   // Matches MoonfallArena::RoomCenter; unknown coordinates remain hidden.
   const float WorldX = Region * 6000.f - 2200.f + Dungeon * 1100.f;
   const float WorldY = 3600.f + Chamber * 780.f;
   const float DX = (WorldX - PlayerPos.X) * Scale;
   const float DY = (PlayerPos.Y - WorldY) * Scale;
   if (DX * DX + DY * DY > MapRadius * MapRadius) continue;
   DrawRect(FLinearColor(0.55f, 0.65f, 0.85f, 0.90f),
            CenterX + DX - 2.f, CenterY + DY - 2.f, 4.f, 4.f);
  }
  // A single high-contrast player marker; north remains screen-up.
  DrawRect(FLinearColor(0.96f, 0.81f, 0.33f), CenterX - 3.f, CenterY - 3.f, 6.f, 6.f);
  // Compact grounded objective, next to the map, not an extra combat overlay.
  const int32 QuestId = Arena->GetTrackedDungeonQuestId();
  if (QuestId >= 0 && QuestId < 30 &&
      Arena->GetDiscoveredDungeonEntrances().Contains(QuestId)) {
   const bool bComplete = Arena->IsDungeonQuestCompleted(QuestId);
   const float TextScale = Canvas->SizeX < 800.f ? 0.72f : 0.84f;
   DrawText(FString::Printf(TEXT("DUNGEON %d  |  %d / 30"),
      QuestId + 1, Arena->GetCompletedDungeonQuestCount()),
      FLinearColor(0.93f, 0.85f, 0.66f), MapLeft,
      MapTop + MapWidth + 6.f, nullptr, TextScale);
   const int32 ResidentsDefeated = Arena->GetDungeonResidentDefeatCount(QuestId);
   const FString Objective = bComplete ? TEXT("Guardian defeated")
     : ResidentsDefeated < 4
       ? FString::Printf(TEXT("Defeat residents: %d / 4"), ResidentsDefeated)
       : TEXT("Defeat the guardian");
   DrawText(Objective, bComplete ? FLinearColor(0.38f, 0.95f, 0.5f)
      : FLinearColor::White, MapLeft, MapTop + MapWidth + 23.f, nullptr, TextScale);
   // Persistent capacity reward displayed without a new HUD widget or pickup.
   DrawText(FString::Printf(TEXT("Guardian power: +%d max mana/stamina"),
      Arena->GetCompletedDungeonQuestCount() * 3),
      FLinearColor(0.72f, 0.83f, 1.f), MapLeft,
      MapTop + MapWidth + 40.f, nullptr, TextScale * 0.8f);
  }
  break;
 }
 // Proximity-only quest guide prompt. Never advertises undiscovered quest pins.
 for (TActorIterator<AMoonfallQuestGuide> GuideIt(GetWorld()); GuideIt; ++GuideIt) {
  const AMoonfallQuestGuide* Guide = *GuideIt;
  if (!IsValid(Guide)) continue;
  if (FMath::Abs(Guide->GetActorLocation().Z - Hero->GetActorLocation().Z) > 250.f) continue;
  if (FVector::DistSquared2D(Guide->GetActorLocation(), Hero->GetActorLocation()) >= FMath::Square(380.f))
   continue;
  DrawText(TEXT("R: Talk   T: Service   V: Craft   G/H: Social"),
    FLinearColor(0.95f, 0.84f, 0.43f), FMath::Max(20.f, Canvas->SizeX * 0.38f),
    Canvas->SizeY - 92.f, nullptr, 0.9f);
  break;
 }
 // Dialogue feedback remains near the guide prompt, no separate menu or actor.
 if (Hero->LastGuideRegionIndex >= 0 && Hero->LastGuideMessageUntil > GetWorld()->GetTimeSeconds()) {
  const FString Speaker = Hero->LastGuideTitle.IsEmpty()
    ? TEXT("Quest Guide") : Hero->LastGuideTitle;
  FString GuideLine;
  if (Hero->bLastCraftSucceeded)
   GuideLine = FString::Printf(TEXT("%s: Crafted regional equipment for %d salvage."), *Speaker, Hero->LastCraftPrice);
  else if (Hero->LastCraftPrice > 0 && Hero->LastNpcRole == 1 && Hero->LastServicePrice == 0)
   GuideLine = FString::Printf(TEXT("%s: Craft %d salvage; workshop stock %d."),
     *Speaker, Hero->LastCraftPrice, Hero->LastWorkshopStock);
  else if (Hero->bLastServicePurchased)
   GuideLine = FString::Printf(TEXT("%s: Service complete, %d salvage paid."), *Speaker, Hero->LastServicePrice);
  else if (Hero->LastServicePrice > 0 && Hero->LastGuideSocialChoice == -1 &&
           Hero->LastNpcRole > 0)
   GuideLine = FString::Printf(TEXT("%s: %s service costs %d salvage."),
      *Speaker, Hero->LastNpcRole == 1 ? TEXT("Mana") : TEXT("Stamina"), Hero->LastServicePrice);
  else if (Hero->LastGuideSocialChoice == 0)
   GuideLine = FString::Printf(TEXT("%s: I remember your kindness."), *Speaker);
  else if (Hero->LastGuideSocialChoice == 1)
   GuideLine = FString::Printf(TEXT("%s: Your threats will not be forgotten."), *Speaker);
  else if (Hero->LastFactionStanding <= -45)
   GuideLine = FString::Printf(TEXT("%s: Your reputation has unsettled this town."), *Speaker);
  else if (Hero->LastFactionStanding >= 55)
   GuideLine = FString::Printf(TEXT("%s: Our community speaks well of you."), *Speaker);
  else if (Hero->LastGuideFear >= 50)
   GuideLine = FString::Printf(TEXT("%s: Please, leave us in peace."), *Speaker);
  else if (Hero->LastGuideTrust >= 55)
   GuideLine = FString::Printf(TEXT("%s: Our region remembers what you have done."), *Speaker);
  else if (Hero->LastNpcRole == 1)
   GuideLine = FString::Printf(TEXT("%s: I make what this region needs."), *Speaker);
  else if (Hero->LastNpcRole == 2)
   GuideLine = FString::Printf(TEXT("%s: My watch keeps the roads safe."), *Speaker);
  else if (!Hero->bLastGuideFoundQuest || Hero->LastGuideQuestId < 0)
   GuideLine = FString::Printf(TEXT("%s: Discover an entrance first."), *Speaker);
  else if (Hero->bLastGuideGuardianDefeated)
   GuideLine = FString::Printf(TEXT("%s: Dungeon %d is liberated. Seek another."),
     *Speaker, Hero->LastGuideQuestId + 1);
  else if (Hero->LastGuideResidentDefeats >= 4)
   GuideLine = FString::Printf(TEXT("%s: Dungeon %d guardian awaits."),
     *Speaker, Hero->LastGuideQuestId + 1);
  else
   GuideLine = FString::Printf(TEXT("%s: Dungeon %d, residents %d of 4."),
     *Speaker, Hero->LastGuideQuestId + 1,
     FMath::Clamp(Hero->LastGuideResidentDefeats, 0, 4));
  DrawText(GuideLine, FLinearColor(0.93f, 0.85f, 0.66f),
    FMath::Max(18.f, Canvas->SizeX * 0.18f), Canvas->SizeY - 121.f,
    nullptr, Canvas->SizeX < 900.f ? 0.70f : 0.88f);
  DrawText(FString::Printf(TEXT("%s  |  %s"), *Hero->LastNpcPersonality, *Hero->LastNpcRoutine),
    FLinearColor(0.86f,0.78f,0.97f), FMath::Max(18.f, Canvas->SizeX * 0.18f),
    Canvas->SizeY - 85.f, nullptr, 0.72f);
  if (Hero->LastNpcRole > 0 && Hero->LastServicePrice > 0)
   DrawText(FString::Printf(TEXT("Supply %d / 100  |  Service %d salvage"),
     Hero->LastSupplyRating, Hero->LastServicePrice),
     FLinearColor(0.9f,0.8f,0.55f), FMath::Max(18.f, Canvas->SizeX * 0.18f),
     Canvas->SizeY - 49.f, nullptr, 0.72f);
  DrawText(FString::Printf(TEXT("Faction %d   Ally bond %d"),
      Hero->LastFactionStanding, Hero->LastNpcAllyBond),
    FLinearColor(0.80f,0.86f,0.97f), FMath::Max(18.f, Canvas->SizeX * 0.18f),
    Canvas->SizeY - 67.f, nullptr, 0.72f);
  DrawText(FString::Printf(TEXT("Trust %d   Fear %d"), Hero->LastGuideTrust, Hero->LastGuideFear),
    FLinearColor(0.75f,0.85f,0.98f), FMath::Max(18.f, Canvas->SizeX * 0.18f),
    Canvas->SizeY - 103.f, nullptr, 0.72f);
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
  const TCHAR* RarityNames[] = { TEXT("COMMON"), TEXT("UNCOMMON"), TEXT("RARE"), TEXT("EPIC") };
  const int32 Rarity = FMath::Clamp(Hero->LastLootRarity, 0, 3);
  DrawText(FString::Printf(TEXT("Gear W:%d A:%d C:%d | Salvage %d | Bag %d/30"),
   Hero->WeaponTier, Hero->ArmorTier, Hero->AccessoryTier, Hero->Salvage, Hero->InventoryItemCodes.Num()),
   FLinearColor(0.85f,0.84f,0.95f), 24.f, Canvas->SizeY - 75.f, nullptr, 0.8f);
  if (Hero->InventoryItemCodes.Num() > 0) {
   DrawText(FString::Printf(TEXT("Last drop: %s Tier %d (%+d vs equipped)"),
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

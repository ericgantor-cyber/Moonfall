#include "MoonfallCombatHUD.h"
#include "MoonfallHeroPawn.h"
#include "MoonfallEnemy.h"
#include "MoonfallWorldNPC.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/PlayerController.h"
void AMoonfallCombatHUD::DrawHUD()
{
 Super::DrawHUD();
 if (!Canvas) return;
 const AMoonfallHeroPawn* Hero = Cast<AMoonfallHeroPawn>(UGameplayStatics::GetPlayerPawn(this,0));
 if (!IsValid(Hero)) return;
 // Native Canvas resident menu. Matching click bounds are in HeroPawn.
 if (const AMoonfallWorldNPC* Resident = Hero->FocusedResident.Get()) {
  const float InteractionRange = IsValid(Resident) && FMath::IsFinite(Resident->InteractRange)
      ? FMath::Clamp(Resident->InteractRange, 50.f, 400.f) : 0.f;
  if (Hero->Health > 0.f && IsValid(Resident) && InteractionRange > 0.f &&
      !Hero->GetActorLocation().ContainsNaN() &&
      !Resident->GetActorLocation().ContainsNaN() &&
      FVector::DistSquared(Hero->GetActorLocation(), Resident->GetActorLocation()) <=
          FMath::Square(InteractionRange)) {
   const float W = FMath::Min(490.f, FMath::Max(260.f, Canvas->SizeX - 24.f));
   const float H = FMath::Min(345.f, FMath::Max(270.f, Canvas->SizeY - 24.f));
   const float L = (Canvas->SizeX - W) * 0.5f, T = (Canvas->SizeY - H) * 0.5f;
   DrawRect(FLinearColor(0.025f, 0.028f, 0.07f, 0.96f), L, T, W, H);
   DrawRect(FLinearColor(0.42f, 0.28f, 0.68f, 0.90f), L, T, W, 5.f);
   // Standard close X in every interactive Moonfall panel.
   const float NpcCloseX = L + W - 42.f;
   DrawRect(FLinearColor(0.37f,0.16f,0.22f,0.96f), NpcCloseX, T + 12.f, 28.f, 28.f);
   DrawText(TEXT("X"), FLinearColor::White, NpcCloseX + 9.f, T + 16.f, nullptr, 0.9f);
   DrawText(Resident->DisplayName.ToString(), FLinearColor::White,
     L + 18.f, T + 16.f, nullptr, Canvas->SizeX < 900.f ? 0.95f : 1.25f);
   DrawText(FString::Printf(TEXT("%s  |  %s  |  Trust %d"),
     *Resident->GetRaceName(), *Resident->GetSocialDisposition(Hero),
     Resident->GetBondWithHero(Hero)), FLinearColor(0.70f,0.83f,0.98f),
     L + 18.f, T + 53.f, nullptr, 0.90f);
   const int32 Quest = Hero->GetResidentQuestState(Resident->ResidentId);
   const FString QuestAction = Quest == 0 ? TEXT("Accept contract")
     : Quest == 1 ? TEXT("Turn in salvage") : TEXT("Completed");
   const FString Names[3] = { TEXT("Talk"),
     Quest == 1 && Hero->bResidentTurnInPending && Hero->GetWorld()
       && Hero->GetWorld()->GetTimeSeconds() <= Hero->ResidentTurnInConfirmUntil
       ? FString(TEXT("Confirm turn-in")) : QuestAction,
     TEXT("Close") };
   // Mouse hover and gamepad selection share row geometry.
   float CursorX = -1.f, CursorY = -1.f;
   bool bCursorInMenu = false;
   if (const APlayerController* PC = Cast<APlayerController>(Hero->GetController()))
    bCursorInMenu = PC->bShowMouseCursor && PC->GetMousePosition(CursorX, CursorY);
   for (int32 Row = 0; Row < 3; ++Row) {
    const float Y = T + 122.f + Row * 53.f;
    const bool bHover = bCursorInMenu && CursorX >= L + 14.f &&
      CursorX <= L + W - 14.f && CursorY >= Y && CursorY <= Y + 43.f;
    DrawRect((bHover || Row == Hero->ResidentContextSelection)
        ? FLinearColor(0.24f,0.22f,0.43f,0.98f)
        : FLinearColor(0.10f,0.10f,0.18f,0.94f), L + 14.f, Y, W - 28.f, 43.f);
    DrawText(Names[Row], Row == 1 && Quest == 2
        ? FLinearColor(0.64f,0.66f,0.72f) : FLinearColor::White,
        L + 25.f, Y + 10.f, nullptr, 1.f);
   }
   const FString ContractStatus = Quest == 0 ? TEXT("Available")
     : Quest == 1 ? TEXT("Active") : TEXT("Completed");
   DrawText(FString::Printf(TEXT("Contract: %s  |  Faction %s"),
     *ContractStatus, *Resident->FactionId.ToString()),
     FLinearColor(0.83f,0.81f,0.94f), L + 18.f, T + 77.f, nullptr, 0.76f);
   DrawText(FString::Printf(TEXT("Salvage: %d / %d"),
       Hero->Salvage, Resident->GetContractSalvageCost()),
       FLinearColor(0.76f,0.87f,0.68f), L + 18.f, T + H - 58.f, nullptr, 0.86f);
   if (Hero->GetWorld() && Hero->GetWorld()->GetTimeSeconds() < Hero->ResidentFeedbackUntil)
    DrawText(Hero->ResidentFeedbackMessage, FLinearColor(0.96f,0.78f,0.41f),
        L + 18.f, T + 99.f, nullptr, 0.72f);
   DrawText(TEXT("Click choice  |  D-pad up/down + A  |  Esc / RMB: close"),
       FLinearColor(0.80f,0.79f,0.92f), L + 18.f, T + H - 31.f, nullptr,
       Canvas->SizeX < 850.f ? 0.65f : 0.78f);
   return;
  }
 }
 if (Hero->bOptionsOpen) {
  const float MenuWidth = FMath::Min(560.f, FMath::Max(240.f, Canvas->SizeX - 24.f));
  const float MenuHeight = FMath::Min(550.f, FMath::Max(280.f, Canvas->SizeY - 24.f));
  const float MenuLeft = Canvas->SizeX * 0.5f - MenuWidth * 0.5f;
  const float MenuTop = Canvas->SizeY * 0.5f - MenuHeight * 0.5f;
  const float MenuRowGap = (MenuHeight - 130.f) / 8.f;
  const float RowHeight = FMath::Min(38.f, MenuRowGap - 2.f);
  const float FirstRowY = MenuTop + 63.f;
  const bool bCompactOptions = MenuWidth < 420.f;
  DrawRect(FLinearColor(0.04f,0.03f,0.11f,0.94f), MenuLeft, MenuTop, MenuWidth, MenuHeight);
  const float OptionsCloseX = MenuLeft + MenuWidth - 42.f;
  DrawRect(FLinearColor(0.37f,0.16f,0.22f,0.96f), OptionsCloseX, MenuTop + 12.f, 28.f, 28.f);
  DrawText(TEXT("X"), FLinearColor::White, OptionsCloseX + 9.f, MenuTop + 16.f, nullptr, 0.9f);
  DrawText(bCompactOptions ? TEXT("MOONFALL | OPTIONS") : TEXT("MOONFALL | GRAPHICS & ACCESSIBILITY"),
      FLinearColor::White, MenuLeft + 18.f, MenuTop + 17.f, nullptr, bCompactOptions ? 0.83f : 1.05f);
  UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
  if (Settings) {
   float ScalePct = 100.f, ScaleNormalized = 0.f, ScaleMin = 50.f, ScaleMax = 100.f;
   Settings->GetResolutionScaleInformationEx(ScalePct, ScaleNormalized, ScaleMin, ScaleMax);
   const FIntPoint Res = Settings->GetScreenResolution();
   // Short, readable values leave room for real mouse +/- targets on every row.
   const FString ResLabel = Res == FIntPoint(1280,720) ? TEXT("720p")
     : Res == FIntPoint(1920,1080) ? TEXT("1080p")
     : Res == FIntPoint(2560,1440) ? TEXT("1440p")
     : Res == FIntPoint(3840,2160) ? TEXT("4K")
     : FString::Printf(TEXT("%dx%d"), Res.X, Res.Y);
   const int32 Quality = FMath::Clamp(Settings->GetOverallScalabilityLevel(), 0, 3);
   const FString QualityName[] = {TEXT("Low"), TEXT("Med"), TEXT("High"), TEXT("Epic")};
   const FString FrameStatus = Hero->FrameGenerationPreference == 0 ? TEXT("Off")
     : Hero->FrameGenerationPreference == 1 ? TEXT("Auto") : TEXT("On");
   const FString Names[] = {TEXT("Resolution"), TEXT("Render scale"), TEXT("Quality"),
     TEXT("Frame gen"), TEXT("Reduced motion"), TEXT("Combat text"), TEXT("Boss alerts"),
     TEXT("Display mode")};
   const FString CompactNames[] = {TEXT("Res"), TEXT("Scale"), TEXT("Quality"),
     TEXT("Frame"), TEXT("Motion"), TEXT("Text"), TEXT("Alerts"), TEXT("Mode")};
   const FString Values[] = {ResLabel, FString::Printf(TEXT("%.0f%%"), ScalePct),
     QualityName[Quality], FrameStatus, Hero->bReduceMotion ? TEXT("On") : TEXT("Off"),
     FString::Printf(TEXT("%.0f%%"), Hero->CombatTextScale * 100.f),
     FString::Printf(TEXT("%.0f%%"), Hero->BossWarningScale * 100.f),
     Settings->GetFullscreenMode() == EWindowMode::Windowed ? TEXT("Windowed") : Settings->GetFullscreenMode() == EWindowMode::WindowedFullscreen ? TEXT("Borderless") : TEXT("Fullscreen")};
   const float MinusX = MenuLeft + MenuWidth - 145.f;
   const float PlusX = MenuLeft + MenuWidth - 37.f;
   // Options mouse hover is visual only; clicks still choose and adjust explicitly.
   float OptionsCursorX = -1.f, OptionsCursorY = -1.f;
   bool bOptionsCursorVisible = false;
   if (const APlayerController* PC = Cast<APlayerController>(Hero->GetController()))
    bOptionsCursorVisible = PC->bShowMouseCursor
      && PC->GetMousePosition(OptionsCursorX, OptionsCursorY);
   for (int32 i = 0; i < 8; ++i) {
    const float Y = FirstRowY + i * MenuRowGap;
    const bool bSelected = Hero->OptionsSelection == i;
    const bool bHoveredRow = bOptionsCursorVisible
      && OptionsCursorX >= MenuLeft + 10.f && OptionsCursorX <= MenuLeft + MenuWidth - 10.f
      && OptionsCursorY >= Y && OptionsCursorY <= Y + RowHeight;
    DrawRect(bSelected ? FLinearColor(0.22f,0.18f,0.36f,0.98f)
                       : bHoveredRow ? FLinearColor(0.15f,0.18f,0.29f,0.96f)
                       : FLinearColor(0.09f,0.08f,0.16f,0.94f),
       MenuLeft + 10.f, Y, MenuWidth - 20.f, RowHeight);
    DrawText(bCompactOptions ? CompactNames[i] : Names[i], bSelected ? FLinearColor(0.96f,0.86f,0.56f) : FLinearColor::White,
       MenuLeft + 17.f, Y + 3.f, nullptr, bCompactOptions ? 0.65f : 0.89f);
    const float ButtonHeight = FMath::Min(26.f, RowHeight);
    const bool bMinusHover = bOptionsCursorVisible && OptionsCursorX >= MinusX
      && OptionsCursorX <= MinusX + 25.f && OptionsCursorY >= Y
      && OptionsCursorY <= Y + ButtonHeight;
    const bool bPlusHover = bOptionsCursorVisible && OptionsCursorX >= PlusX
      && OptionsCursorX <= PlusX + 25.f && OptionsCursorY >= Y
      && OptionsCursorY <= Y + ButtonHeight;
    const FLinearColor IdleButton(0.20f,0.25f,0.37f,0.98f);
    const FLinearColor HoverButton(0.35f,0.45f,0.60f,0.98f);
    DrawRect(bMinusHover ? HoverButton : IdleButton, MinusX, Y, 25.f, ButtonHeight);
    DrawRect(bPlusHover ? HoverButton : IdleButton, PlusX, Y, 25.f, ButtonHeight);
    DrawText(TEXT("-"), FLinearColor::White, MinusX + 9.f, Y + 2.f, nullptr, 0.95f);
    DrawText(TEXT("+"), FLinearColor::White, PlusX + 7.f, Y + 2.f, nullptr, 0.95f);
    DrawText(Values[i], FLinearColor(0.75f,0.90f,1.f), MinusX + 31.f, Y + 3.f,
      nullptr, bCompactOptions ? 0.57f : 0.72f);
   }
  }
  // Show concise, setting-specific help without covering the rows on small screens.
  const FString SettingHelp[] = {
    TEXT("Choose an available screen resolution"),
    TEXT("Adjust image sharpness and performance"),
    TEXT("Higher quality may reduce frame rate"),
    TEXT("Frame generation is a preference only; no runtime integration."),
    TEXT("Reduce impact shake and camera motion"),
    TEXT("Scale damage numbers in combat"),
    TEXT("Scale the visibility of boss warnings"),
    TEXT("Choose Windowed, Borderless or Fullscreen")
  };
  const FString CompactHelp[] = {
    TEXT("Screen resolution"), TEXT("Render scaling"), TEXT("Graphics preset"),
    TEXT("FG preference only (inactive)"), TEXT("Less camera motion"),
    TEXT("Damage text size"), TEXT("Boss alert size"), TEXT("Window mode")
  };
  const int32 HelpIndex = FMath::Clamp(Hero->OptionsSelection, 0, 7);
  DrawText(bCompactOptions ? CompactHelp[HelpIndex] : SettingHelp[HelpIndex],
     FLinearColor(0.74f,0.81f,0.91f), MenuLeft + 15.f,
     MenuTop + MenuHeight - 28.f, nullptr, bCompactOptions ? 0.60f : 0.72f);
  return;
 }
 if (Hero->bInventoryOpen) {
  // Canvas-only continuous inventory: no extra actors, widgets, or hidden controls.
  const float PanelWidth = FMath::Min(720.f, FMath::Max(280.f, FMath::Min(Canvas->SizeX - 24.f, Canvas->SizeX * 0.48f)));
  const float PanelHeight = FMath::Min(860.f, FMath::Max(280.f, Canvas->SizeY - 24.f));
  // Dock the inventory and character pane at the right, keeping the world visible to the left.
  const float Left = Canvas->SizeX - PanelWidth - 12.f;
  const float Top = (Canvas->SizeY - PanelHeight) * 0.5f;
  DrawRect(FLinearColor(0.04f, 0.035f, 0.10f, 0.96f), Left, Top, PanelWidth, PanelHeight);
  DrawText(PanelWidth < 550.f ? TEXT("INVENTORY") : TEXT("MOONFALL  |  INVENTORY"),
    FLinearColor::White, Left + 18.f, Top + 16.f, nullptr, PanelWidth < 550.f ? 0.93f : 1.3f);
  // Shared close button is visible on inventory and character views.
  const float CloseSortWidth = PanelWidth < 550.f ? 105.f : 172.f;
  const float CloseSortX = Left + PanelWidth - CloseSortWidth - 14.f;
  const float CloseButtonX = CloseSortX - 36.f;
  DrawRect(FLinearColor(0.37f,0.16f,0.22f,0.96f), CloseButtonX, Top + 14.f, 28.f, 28.f);
  DrawText(TEXT("X"), FLinearColor::White, CloseButtonX + 9.f, Top + 18.f, nullptr, 0.9f);
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
   DrawText(TEXT("P / A: back to inventory    I / Select / Esc: close"),
     FLinearColor(0.81f,0.83f,0.97f), Left + 18.f, Top + PanelHeight - 29.f,
     nullptr, 0.8f);
   return;
  }
  // Identical button rectangle to HitTestInventorySortButton in HeroPawn.
  const float SortWidth = PanelWidth < 550.f ? 105.f : 172.f;
  const float SortX = Left + PanelWidth - SortWidth - 14.f;
  const float SortY = Top + 14.f;
  DrawRect(FLinearColor(0.18f,0.28f,0.42f,0.96f), SortX, SortY, SortWidth, 28.f);
  DrawText(PanelWidth < 550.f
       ? (Hero->InventorySortMode == 0 ? TEXT("SORT: NEW") :
          Hero->InventorySortMode == 1 ? TEXT("SORT: QLTY") : TEXT("SORT: SLOT"))
       : (Hero->InventorySortMode == 0 ? TEXT("SORT: ORIGINAL") :
          Hero->InventorySortMode == 1 ? TEXT("SORT: QUALITY") : TEXT("SORT: SLOT")),
     FLinearColor::White, SortX + 7.f, SortY + 5.f, nullptr, PanelWidth < 550.f ? 0.68f : 0.78f);
  const int32 Count = Hero->InventoryItemCodes.Num();
  // Reserve height for a real equipment-stat line and three controls rows.
  const int32 PerPage = Hero->GetInventoryVisibleRows();
  const int32 Selected = FMath::Clamp(Hero->InventorySelection, 0, FMath::Max(0, Count - 1));
  const TArray<int32> DisplayOrder = Hero->GetInventoryDisplayOrder();
  const int32 HoveredIndex = Hero->HitTestInventoryRow();
  // Wheel moves only the visible window; keyboard/controller selection remains independent.
  const int32 PageStart = FMath::Clamp(Hero->InventoryViewStart, 0, FMath::Max(0, Count - PerPage));
  const int32 PageEnd = FMath::Min(Count, PageStart + PerPage);
  DrawText(FString::Printf(TEXT("%d items  |  %d salvage  |  visible %d-%d / %d"),
    Count, Hero->Salvage, Count == 0 ? 0 : PageStart + 1,
    PageEnd, Count), FLinearColor(0.75f,0.72f,0.91f),
    Left + 18.f, Top + 49.f, nullptr, 0.9f);
  if (Count == 0)
   DrawText(TEXT("No equipment yet. Defeat enemies to find gear."), FLinearColor::White,
     Left + 18.f, Top + 100.f, nullptr, 0.92f);
  const float RowSpacing = FMath::Max(21.f, (PanelHeight - 390.f) / PerPage);
  for (int32 DisplayPosition = PageStart; DisplayPosition < PageEnd; ++DisplayPosition) {
   const int32 Index = DisplayOrder[DisplayPosition];
   const int32 Code = Hero->InventoryItemCodes[Index];
   const int32 Affix = Code / 100000;
   const int32 Slot = (Code / 10000) % 10;
   const int32 Rarity = (Code / 1000) % 10;
   const int32 Tier = Code % 1000;
   const TCHAR* Slots[] = {TEXT("Weapon"), TEXT("Armor"), TEXT("Accessory")};
   const TCHAR* Rarities[] = {TEXT("Common"), TEXT("Uncommon"), TEXT("Rare"), TEXT("Epic"), TEXT("Emerald"), TEXT("Legendary")};
   const TCHAR* Traits[] = {TEXT("None"), TEXT("Force"), TEXT("Ward"), TEXT("Focus")};
   const bool bValid = Code > 0 && Slot <= 2 && Affix <= 3 && Rarity <= 5 && Tier >= 1 && Tier <= 25
     && (Rarity != 0 || Affix == 0)
     && Code == Affix * 100000 + Slot * 10000 + Rarity * 1000 + Tier;
   const bool bEquipped = Code == (Slot == 0 ? Hero->EquippedWeaponCode
     : Slot == 1 ? Hero->EquippedArmorCode : Hero->EquippedAccessoryCode);
   const float Y = Top + 145.f + (DisplayPosition - PageStart) * RowSpacing;
   if (Index == Selected) DrawRect(FLinearColor(0.25f,0.18f,0.43f,0.90f),
     Left + 12.f, Y - 3.f, PanelWidth - 24.f, RowSpacing - 3.f);
   else if (Index == HoveredIndex) DrawRect(FLinearColor(0.17f,0.32f,0.43f,0.80f),
     Left + 12.f, Y - 3.f, PanelWidth - 24.f, RowSpacing - 3.f);
   const FString Row = bValid ? FString::Printf(TEXT("%02d  %s  T%d  %s  %s%s"),
      DisplayPosition + 1, Slots[Slot], Tier, Rarities[Rarity], Traits[Affix], bEquipped ? TEXT("  [EQUIPPED]") : TEXT(""))
    : FString::Printf(TEXT("%02d  Invalid item"), DisplayPosition + 1);
   // Item names retain a clear text label plus a distinct quality color.
   const FLinearColor RarityColors[] = {FLinearColor(0.90f,0.90f,0.90f),
     FLinearColor(0.25f,0.69f,1.f), FLinearColor(0.96f,0.79f,0.25f),
     FLinearColor(0.72f,0.35f,0.96f), FLinearColor(0.23f,0.84f,0.42f),
     FLinearColor(1.f,0.52f,0.12f)};
   DrawText(Row, Index == Selected ? FLinearColor::Yellow
     : (bValid ? RarityColors[Rarity] : FLinearColor::White),
     Left + 20.f, Y, nullptr, Canvas->SizeX < 900.f ? 0.75f : 0.9f);
  }
  // Proportional scroll indicator. Clicking its track jumps the visible
  // bag window without selecting, equipping or modifying saved gear.
  if (Count > PerPage) {
   const float TrackY = Top + 145.f;
   const float TrackHeight = RowSpacing * PerPage;
   const float TrackX = Left + PanelWidth - 11.f;
   const float ThumbHeight = FMath::Max(14.f, TrackHeight * PerPage / Count);
   const int32 MaxStart = FMath::Max(1, Count - PerPage);
   const float ThumbY = TrackY + (TrackHeight - ThumbHeight) * PageStart / MaxStart;
   DrawRect(FLinearColor(0.17f,0.16f,0.25f,0.95f), TrackX, TrackY, 4.f, TrackHeight);
   DrawRect(FLinearColor(0.57f,0.81f,0.93f,1.f), TrackX, ThumbY, 6.f, ThumbHeight);
  }
  // Match the combat decoder: a code only contributes while owned, correctly
  // slotted and canonically encoded. HUD previews must never imply phantom stats.
  const auto ReadOwnedGear = [Hero](int32 Code, int32 ExpectedSlot,
                                   int32& Trait, int32& Rarity, int32& Tier) {
   Trait = 0; Rarity = 0; Tier = 0;
   if (Code <= 0 || !Hero->InventoryItemCodes.Contains(Code)) return;
   const int32 EncodedTrait = Code / 100000;
   const int32 Slot = (Code / 10000) % 10;
   const int32 EncodedRarity = (Code / 1000) % 10;
   const int32 EncodedTier = Code % 1000;
   if (EncodedTrait < 0 || EncodedTrait > 3 || Slot != ExpectedSlot
       || EncodedRarity < 0 || EncodedRarity > 5
       || EncodedTier < 1 || EncodedTier > 25
       || (EncodedRarity == 0 && EncodedTrait != 0)
       || Code != EncodedTrait * 100000 + Slot * 10000 + EncodedRarity * 1000 + EncodedTier)
    return;
   Trait = EncodedTrait; Rarity = EncodedRarity; Tier = EncodedTier;
  };
  int32 WeaponTrait = 0, WeaponRarity = 0, WeaponTier = 0;
  int32 ArmorTrait = 0, ArmorRarity = 0, ArmorTier = 0;
  int32 AccessoryTrait = 0, AccessoryRarity = 0, AccessoryTier = 0;
  ReadOwnedGear(Hero->EquippedWeaponCode, 0, WeaponTrait, WeaponRarity, WeaponTier);
  ReadOwnedGear(Hero->EquippedArmorCode, 1, ArmorTrait, ArmorRarity, ArmorTier);
  ReadOwnedGear(Hero->EquippedAccessoryCode, 2, AccessoryTrait, AccessoryRarity, AccessoryTier);
  const float WeaponBonusPct = 100.f * (FMath::Clamp(0.018f * WeaponTier, 0.f, 0.35f)
    + (WeaponTrait == 1 && WeaponRarity > 0 ? 0.06f : 0.f) + WeaponRarity * 0.015f);
  const float ArmorMitigationPct = 100.f * FMath::Min(0.34f, FMath::Min(0.28f, ArmorTier * 0.01f)
    + (ArmorTrait == 2 && ArmorRarity > 0 ? 0.06f : 0.f) + ArmorRarity * 0.005f
    + Hero->GetVersatilityBonus() * 0.5f
    + (Hero->GetSetPieceCount(2) >= 3 ? 0.025f : 0.f));
  const float ManaRegenBonusPct = 100.f * (FMath::Clamp(AccessoryTier * 0.012f, 0.f, 0.25f)
    + (AccessoryTrait == 3 && AccessoryRarity > 0 ? 0.08f : 0.f) + AccessoryRarity * 0.02f
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
  DrawText(TEXT("Mouse wheel: scroll   Left click: select   Right click: equip"),
     FLinearColor::White, Left + 18.f, Top + PanelHeight - 51.f, nullptr, 0.75f);
  DrawText(TEXT("Drag scrollbar: browse   Delete / X: salvage"),
     FLinearColor(0.80f,0.85f,0.97f), Left + 18.f, Top + PanelHeight - 68.f,
     nullptr, PanelWidth < 420.f ? 0.60f : 0.72f);
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
   // A dedicated, read-only selected-item summary above crafting and comparison.
   // Keep this area hidden on very short viewports to avoid covering bag rows.
   if (PanelHeight >= 500.f) {
    const TCHAR* DetailSlots[] = {TEXT("Weapon"), TEXT("Armor"), TEXT("Accessory")};
    const TCHAR* DetailRarities[] = {TEXT("Common"), TEXT("Uncommon"), TEXT("Rare"),
      TEXT("Epic"), TEXT("Emerald"), TEXT("Legendary")};
    const TCHAR* DetailTraits[] = {TEXT("None"), TEXT("Force"), TEXT("Ward"), TEXT("Focus")};
    const bool bValidDetail = SelectedCode > 0 && SelectedSlot >= 0 && SelectedSlot <= 2
      && SelectedRarity >= 0 && SelectedRarity <= 5
      && SelectedAffix >= 0 && SelectedAffix <= 3 && SelectedTier >= 1 && SelectedTier <= 25
      && (SelectedRarity != 0 || SelectedAffix == 0)
      && SelectedCode == SelectedAffix * 100000 + SelectedSlot * 10000
        + SelectedRarity * 1000 + SelectedTier;
    const FLinearColor DetailColors[] = {FLinearColor(0.90f,0.90f,0.90f),
      FLinearColor(0.25f,0.69f,1.f), FLinearColor(0.96f,0.79f,0.25f),
      FLinearColor(0.72f,0.35f,0.96f), FLinearColor(0.23f,0.84f,0.42f),
      FLinearColor(1.f,0.52f,0.12f)};
    const FString ItemName = bValidDetail
      ? FString::Printf(TEXT("SELECTED  |  %s  |  %s  |  Tier %d  |  %s"),
          DetailSlots[SelectedSlot], DetailRarities[SelectedRarity], SelectedTier,
          DetailTraits[SelectedAffix])
      : TEXT("SELECTED  |  Invalid equipment data");
    DrawText(ItemName,
      bValidDetail ? DetailColors[SelectedRarity] : FLinearColor(1.f,0.48f,0.46f),
      Left + 18.f, Top + PanelHeight - 230.f, nullptr, PanelWidth < 550.f ? 0.61f : 0.78f);
    const int32 SetPieces = bValidDetail && SelectedAffix > 0
      ? Hero->GetSetPieceCount(SelectedAffix) : 0;
    const TCHAR* SetEffects[] = {TEXT("No set trait"), TEXT("2pc: +3% crit  |  3pc: +4% mastery"),
      TEXT("2pc: +3% versatility  |  3pc: +2.5% protection"),
      TEXT("2pc: +4% haste  |  3pc: +6% mana regen")};
    const FString SetLine = bValidDetail && SelectedAffix > 0
      ? FString::Printf(TEXT("EQUIPPED SET: %s %d/3  |  %s"),
          DetailTraits[SelectedAffix], SetPieces, SetEffects[SelectedAffix])
      : TEXT("EQUIPPED SET: no set bonus from this item");
    DrawText(SetLine, FLinearColor(0.59f,0.95f,0.77f),
      Left + 18.f, Top + PanelHeight - 207.f, nullptr, PanelWidth < 550.f ? 0.56f : 0.68f);
   }
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
   // Compare the highlighted item against the currently equipped item in
   // the same slot. This is informational only and never changes gear.
   const int32 EquippedCode = SelectedSlot == 0 ? Hero->EquippedWeaponCode
     : SelectedSlot == 1 ? Hero->EquippedArmorCode
     : SelectedSlot == 2 ? Hero->EquippedAccessoryCode : 0;
   const int32 EquippedTier = EquippedCode > 0 ? EquippedCode % 1000 : 0;
   const int32 EquippedRarity = EquippedCode > 0 ? (EquippedCode / 1000) % 10 : 0;
   const int32 EquippedAffix = EquippedCode > 0 ? EquippedCode / 100000 : 0;
   const int32 TierDelta = SelectedTier - EquippedTier;
   const int32 RarityDelta = SelectedRarity - EquippedRarity;
   const bool bIsUpgrade = EquippedCode <= 0 || TierDelta > 0
     || (TierDelta == 0 && RarityDelta > 0);
   const bool bIsLower = EquippedCode > 0 && (TierDelta < 0
     || (TierDelta == 0 && RarityDelta < 0));
   const TCHAR* CompareWord = EquippedCode <= 0 ? TEXT("EMPTY SLOT")
     : bIsUpgrade ? TEXT("UPGRADE") : bIsLower ? TEXT("LOWER") : TEXT("SIDEGRADE");
   const TCHAR* CompareRarities[] = {TEXT("Common"), TEXT("Uncommon"), TEXT("Rare"),
     TEXT("Epic"), TEXT("Emerald"), TEXT("Legendary")};
   const TCHAR* CompareTraits[] = {TEXT("None"), TEXT("Force"), TEXT("Ward"), TEXT("Focus")};
   const FString CompareLine = EquippedCode <= 0
     ? FString::Printf(TEXT("Compare: %s  |  equipped slot is empty"), CompareWord)
     : FString::Printf(TEXT("Compare: %s  |  T%d / %s / %s  vs  T%d / %s / %s"),
       CompareWord, SelectedTier, CompareRarities[FMath::Clamp(SelectedRarity, 0, 5)],
       CompareTraits[FMath::Clamp(SelectedAffix, 0, 3)],
       EquippedTier, CompareRarities[FMath::Clamp(EquippedRarity, 0, 5)],
       CompareTraits[FMath::Clamp(EquippedAffix, 0, 3)]);
   DrawText(CompareLine,
     bIsUpgrade ? FLinearColor(0.42f,0.96f,0.62f)
       : bIsLower ? FLinearColor(0.96f,0.62f,0.42f)
       : FLinearColor(0.78f,0.86f,1.f),
     Left + 18.f, Top + PanelHeight - 110.f, nullptr, PanelWidth < 550.f ? 0.57f : 0.68f);
   const int32 NextCost = 4 + SelectedTier * 2 + SelectedRarity * 3;
   const FString ForgeStatus = !bForgeable ? TEXT("Crafting: MAX TIER or invalid")
     : FString::Printf(TEXT("Next tier: %d salvage  [%s]"),
       NextCost, Hero->Salvage >= NextCost ? TEXT("READY") : TEXT("NEED MORE"));
   DrawText(ForgeStatus, bForgeable && Hero->Salvage >= NextCost
     ? FLinearColor(0.50f,0.95f,0.62f) : FLinearColor(0.98f,0.67f,0.45f),
     Left + 18.f, Top + PanelHeight - 89.f, nullptr, 0.82f);
  }
  DrawText(TEXT("B / LT: equip best   U / RT: +1 tier   H / RB: forge 5   R / B: ascend   G / LB: reforge"),
     FLinearColor(0.78f,0.90f,0.95f), Left + 18.f, Top + PanelHeight - 70.f, nullptr, 0.74f);
  DrawText(TEXT("P / A: character stats    I / Select / Esc: close"), FLinearColor::White,
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
  const TCHAR* RarityNames[] = { TEXT("COMMON"), TEXT("UNCOMMON"), TEXT("RARE"),
    TEXT("EPIC"), TEXT("EMERALD"), TEXT("LEGENDARY") };
  const int32 Rarity = FMath::Clamp(Hero->LastLootRarity, 0, 5);
  DrawText(FString::Printf(TEXT("Gear W:%d A:%d C:%d | Salvage %d | Bag %d"),
   Hero->GetValidatedEquippedTier(0), Hero->GetValidatedEquippedTier(1),
    Hero->GetValidatedEquippedTier(2), Hero->Salvage, Hero->InventoryItemCodes.Num()),
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

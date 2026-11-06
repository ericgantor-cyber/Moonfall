#include "MoonfallCombatHUD.h"
#include "MoonfallHeroPawn.h"
#include "MoonfallEnemy.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/DefaultValueHelper.h"
void AMoonfallCombatHUD::BeginPlay()
{
 Super::BeginPlay();
 if (!GConfig) return;
 TArray<FString> SavedCells;
 GConfig->GetArray(TEXT("Moonfall.Exploration"), TEXT("VisitedCells"), SavedCells, GGameUserSettingsIni);
 for (const FString& Encoded : SavedCells) {
  if (ExploredMapCells.Num() >= 384) break;
  FString XText, YText;
  int32 X = 0, Y = 0;
  if (!Encoded.Split(TEXT(","), &XText, &YText) ||
      !FDefaultValueHelper::ParseInt(XText, X) ||
      !FDefaultValueHelper::ParseInt(YText, Y) ||
      FMath::Abs(static_cast<int64>(X)) > 50000 ||
      FMath::Abs(static_cast<int64>(Y)) > 50000) continue;
  const FIntPoint Cell(X,Y);
  if (!ExploredMapCells.Contains(Cell)) ExploredMapCells.Add(Cell);
 }
}
void AMoonfallCombatHUD::SaveExploration()
{
 if (!bExplorationDirty || !GConfig) return;
 TArray<FString> EncodedCells;
 EncodedCells.Reserve(ExploredMapCells.Num());
 for (const FIntPoint& Cell : ExploredMapCells)
  EncodedCells.Add(FString::Printf(TEXT("%d,%d"), Cell.X, Cell.Y));
 GConfig->SetArray(TEXT("Moonfall.Exploration"), TEXT("VisitedCells"), EncodedCells, GGameUserSettingsIni);
 GConfig->Flush(false, GGameUserSettingsIni);
 bExplorationDirty = false;
}
void AMoonfallCombatHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
 SaveExploration();
 Super::EndPlay(EndPlayReason);
}
void AMoonfallCombatHUD::DrawHUD()
{
 Super::DrawHUD();
 if (!Canvas) return;
 const AMoonfallHeroPawn* Hero = Cast<AMoonfallHeroPawn>(UGameplayStatics::GetPlayerPawn(this,0));
 if (!IsValid(Hero)) return;
 // Track exploration independently of HUD visibility, including compact or Options screens.
 const float CellSize = 200.f; // Unreal centimetres
 const FVector HeroPosition = Hero->GetActorLocation();
 const FIntPoint CurrentCell(FMath::FloorToInt(HeroPosition.X / CellSize), FMath::FloorToInt(HeroPosition.Y / CellSize));
 if (!ExploredMapCells.Contains(CurrentCell)) {
  if (ExploredMapCells.Num() >= 384) ExploredMapCells.RemoveAt(0, 1, EAllowShrinking::No);
  ExploredMapCells.Add(CurrentCell);
  bExplorationDirty = true;
 }
 const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
 if (bExplorationDirty && Now >= NextExplorationSaveAt) {
  SaveExploration();
  NextExplorationSaveAt = Now + 15.f;
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
 // Nearest living boss, displayed without creating additional widget actors.
 const AMoonfallEnemy* Boss = nullptr;
 float BossDistanceSq = TNumericLimits<float>::Max();
 for (TActorIterator<AMoonfallEnemy> It(GetWorld()); It; ++It) {
  const AMoonfallEnemy* E = *It;
  if (!IsValid(E) || !E->bBoss || E->Health <= 0.f) continue;
  const float DistSq = FVector::DistSquared(E->GetActorLocation(), Hero->GetActorLocation());
  if (DistSq < BossDistanceSq) { Boss = E; BossDistanceSq = DistSq; }
 }
 // Compact quest tracker uses the live boss and explicit waypoint; no extra actor queries.
 if (Hero->bArenaBossQuestActive && Hero->bQuestTargetActive) {
  const float DistanceMetres = FVector::Dist2D(Hero->GetActorLocation(), Hero->QuestTarget) / 100.f;
  const float Progress = IsValid(Boss)
   ? 1.f - FMath::Clamp(Boss->Health / FMath::Max(1.f, Boss->BossMaxHealth), 0.f, 1.f) : 0.f;
  const float TrackerWidth = FMath::Clamp(Canvas->SizeX * 0.22f, 180.f, 260.f);
  const float TrackerX = 22.f;
  const float TrackerY = 26.f;
  DrawText(TEXT("QUEST: Defeat the Arena Boss"),FLinearColor(1.f,0.78f,0.21f),TrackerX,TrackerY,nullptr,0.9f);
  DrawText(FString::Printf(TEXT("Boss %d%%  |  %.0f m"),FMath::RoundToInt(Progress * 100.f),DistanceMetres),
           FLinearColor(0.85f,0.89f,1.f),TrackerX,TrackerY+22.f,nullptr,0.8f);
  DrawRect(FLinearColor(0.12f,0.13f,0.2f),TrackerX,TrackerY+44.f,TrackerWidth,7.f);
  DrawRect(FLinearColor(1.f,0.68f,0.18f),TrackerX,TrackerY+44.f,TrackerWidth*Progress,7.f);
 } else if (Hero->bArenaBossQuestComplete && Hero->ArenaBossQuestCompletedAt >= 0.f &&
            GetWorld()->GetTimeSeconds() - Hero->ArenaBossQuestCompletedAt < 5.f) {
  DrawText(TEXT("QUEST COMPLETE: Arena Boss"),FLinearColor(0.42f,1.f,0.57f),22.f,26.f,nullptr,0.95f);
 }
 if (Boss && BossDistanceSq < FMath::Square(3000.f)) {
  const float Width = FMath::Min(480.f, Canvas->SizeX * 0.68f);
  const float X = (Canvas->SizeX - Width) * 0.5f;
  const float Y = 72.f;
  const float MaxHP = FMath::Max(Boss->Health, Boss->BossMaxHealth);
  const float Percent = FMath::Clamp(Boss->Health / FMath::Max(1.f, MaxHP), 0.f, 1.f);
  DrawRect(FLinearColor(0.07f,0.04f,0.10f,0.94f),X-6.f,Y-30.f,Width+12.f,67.f);
  DrawRect(FLinearColor(0.19f,0.10f,0.16f),X,Y,Width,16.f);
  DrawRect(Boss->BossPhase >= 3 ? FLinearColor(0.86f,0.18f,0.67f) : FLinearColor(0.84f,0.15f,0.22f),X,Y,Width*Percent,16.f);
  DrawText(FString::Printf(TEXT("BOSS  |  PHASE %d  |  %.0f / %.0f HP"),Boss->BossPhase,Boss->Health,MaxHP),FLinearColor::White,X,Y-25.f,nullptr,0.95f);
  if (Boss->BossWarningEndTime > GetWorld()->GetTimeSeconds())
   DrawText(TEXT("INCOMING ATTACK - DODGE"),FLinearColor(1.f,0.83f,0.24f),X,Y+18.f,nullptr,0.9f);
 }
 if (Hero->bWorldMapOpen) { DrawExplorationAtlas(Hero); return; }
 // North-up local minimap: bounded nearby enemy markers, without duplicating the HUD.
 // World +X points east on the chart, +Y points north; no undiscovered terrain is revealed.
 if (Canvas->SizeX >= 700.f && Canvas->SizeY >= 460.f) {
  const float MapRadius = FMath::Clamp(FMath::Min(Canvas->SizeX, Canvas->SizeY) * 0.09f, 52.f, 84.f);
  const FVector2D MapCenter(Canvas->SizeX - MapRadius - 24.f, MapRadius + 28.f);
  const float MapRange = FMath::Clamp(Hero->MinimapRange, 1200.f, 4800.f); // Unreal centimetres
  for (const FIntPoint& Cell : ExploredMapCells) {
   const FVector2D CellOffset((Cell.X + 0.5f) * CellSize - HeroPosition.X, (Cell.Y + 0.5f) * CellSize - HeroPosition.Y);
   if (CellOffset.SizeSquared() > FMath::Square(MapRange - CellSize)) continue;
   const FVector2D DiscoveredScreen = MapCenter + FVector2D(CellOffset.X, -CellOffset.Y) * (MapRadius * 0.86f / MapRange);
   DrawRect(FLinearColor(0.2f,0.32f,0.42f,0.55f),DiscoveredScreen.X-2.f,DiscoveredScreen.Y-2.f,4.f,4.f);
  }
  const int32 RingSegments = 48;
  for (int32 Segment = 0; Segment < RingSegments; ++Segment) {
   const float A0 = 2.f * PI * Segment / RingSegments;
   const float A1 = 2.f * PI * (Segment + 1) / RingSegments;
   Canvas->K2_DrawLine(MapCenter + FVector2D(FMath::Cos(A0),FMath::Sin(A0)) * MapRadius,
                       MapCenter + FVector2D(FMath::Cos(A1),FMath::Sin(A1)) * MapRadius,
                       2.f,FLinearColor(0.53f,0.67f,0.85f,0.85f));
  }
  Canvas->K2_DrawLine(MapCenter + FVector2D(-MapRadius,0.f),MapCenter + FVector2D(MapRadius,0.f),1.f,FLinearColor(0.3f,0.4f,0.55f,0.5f));
  Canvas->K2_DrawLine(MapCenter + FVector2D(0.f,-MapRadius),MapCenter + FVector2D(0.f,MapRadius),1.f,FLinearColor(0.3f,0.4f,0.55f,0.5f));
  int32 DrawnEnemies = 0;
  for (TActorIterator<AMoonfallEnemy> MapIt(GetWorld()); MapIt && DrawnEnemies < 32; ++MapIt) {
   const AMoonfallEnemy* MapEnemy = *MapIt;
   if (!IsValid(MapEnemy) || MapEnemy->Health <= 0.f) continue;
   const FVector Offset = MapEnemy->GetActorLocation() - Hero->GetActorLocation();
   const FVector2D Delta(Offset.X, Offset.Y);
   if (Delta.SizeSquared() > FMath::Square(MapRange)) continue;
   const FVector2D Dot = MapCenter + FVector2D(Delta.X,-Delta.Y) * (MapRadius * 0.86f / MapRange);
   const float DotSize = MapEnemy->bBoss ? 6.f : MapEnemy->bElite ? 4.5f : 3.f;
   const FLinearColor DotColor = MapEnemy->bBoss ? FLinearColor(1.f,0.16f,0.28f) : MapEnemy->bElite ? FLinearColor(1.f,0.64f,0.15f) : FLinearColor(0.75f,0.37f,0.85f);
   DrawRect(DotColor,Dot.X - DotSize*0.5f,Dot.Y - DotSize*0.5f,DotSize,DotSize);
   ++DrawnEnemies;
  }
  // Only explicit gameplay quest targets are displayed, including an edge direction for distant objectives.
  if (Hero->bQuestTargetActive) {
   const FVector TargetOffset = Hero->QuestTarget - HeroPosition;
   const FVector2D QuestDelta(TargetOffset.X, -TargetOffset.Y);
   const float QuestDistance = QuestDelta.Size();
   if (FMath::IsFinite(QuestDistance)) {
    const FVector2D QuestScreen = MapCenter + QuestDelta.GetSafeNormal() *
     (FMath::Min(QuestDistance, MapRange) * MapRadius * 0.86f / MapRange);
    DrawRect(FLinearColor(1.f,0.78f,0.21f), QuestScreen.X-4.f, QuestScreen.Y-4.f,8.f,8.f);
    if (QuestDistance > MapRange)
     Canvas->K2_DrawLine(QuestScreen - QuestDelta.GetSafeNormal()*8.f,QuestScreen,2.f,FLinearColor(1.f,0.78f,0.21f));
   }
  }
  DrawRect(FLinearColor(0.27f,0.88f,1.f),MapCenter.X-3.f,MapCenter.Y-3.f,6.f,6.f);
  DrawText(TEXT("N"),FLinearColor(0.75f,0.88f,1.f),MapCenter.X-4.f,MapCenter.Y-MapRadius-17.f,nullptr,0.82f);
  DrawText(FString::Printf(TEXT("+ / -  %.0f m"),MapRange/100.f),FLinearColor(0.75f,0.88f,1.f),MapCenter.X-MapRadius,MapCenter.Y+MapRadius+5.f,nullptr,0.72f);
 }
 // Compact circular resource HUD at bottom left, with actual gameplay values.
 const float Radius = FMath::Clamp(FMath::Min(Canvas->SizeX, Canvas->SizeY) * 0.092f, 50.f, 80.f);
 const float CX = Radius + 25.f;
 const float CY = Canvas->SizeY - Radius - 25.f;
 const float LabelScale = FMath::Clamp(Radius / 65.f, 0.78f, 1.16f);
 const bool bCompactScreen = Canvas->SizeX < 760.f || Canvas->SizeY < 480.f;
 const float ActualHealth = FMath::Clamp(Hero->Health / FMath::Max(1.f, Hero->MaxHealth), 0.f, 1.f);
 const float ActualMana = FMath::Clamp(Hero->Mana / FMath::Max(1.f, Hero->MaxMana), 0.f, 1.f);
 const float ActualXP = FMath::Clamp(static_cast<float>(Hero->Experience) / FMath::Max(1, Hero->ExperienceToNextLevel), 0.f, 1.f);
 const float Delta = GetWorld() ? FMath::Clamp(GetWorld()->GetDeltaSeconds(), 0.f, 0.1f) : 0.f;
 if (DisplayHealth < 0.f) { DisplayHealth = ActualHealth; DisplayMana = ActualMana; DisplayXP = ActualXP; }
 if (LastShownLevel < 0) LastShownLevel = Hero->Level;
 if (Hero->Level != LastShownLevel) {
  LastShownLevel = Hero->Level;
  DisplayXP = 0.f;
  LevelGlowUntil = GetWorld() ? GetWorld()->GetTimeSeconds() + 1.25f : 0.f;
 }
 DisplayHealth = FMath::FInterpTo(DisplayHealth, ActualHealth, Delta, 9.f);
 DisplayMana = FMath::FInterpTo(DisplayMana, ActualMana, Delta, 9.f);
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
  const float Age = GetWorld()->GetTimeSeconds() - Enemy->LastHitTime;
  if (Age < 0.f || Age > 0.8f || Enemy->LastHitAmount <= 0.f) continue;
  FVector2D Screen;
  if (GetOwningPlayerController() && GetOwningPlayerController()->ProjectWorldLocationToScreen(Enemy->GetActorLocation() + FVector(0,0,175.f + Age*65.f), Screen) &&
      Screen.X >= 0.f && Screen.X < Canvas->SizeX && Screen.Y >= 0.f && Screen.Y < Canvas->SizeY) {
   ++VisibleDamageNumbers;
   DrawText(FString::Printf(TEXT("-%.0f"), Enemy->LastHitAmount), FLinearColor(1.f,0.84f,0.2f,1.f-Age/0.8f), Screen.X, Screen.Y, nullptr, 1.3f * Hero->CombatTextScale);
  }
 }
}

void AMoonfallCombatHUD::DrawExplorationAtlas(const AMoonfallHeroPawn* Hero)
{
 if (!Canvas || !IsValid(Hero)) return;
 const float Width = FMath::Max(200.f, Canvas->SizeX - 60.f);
 const float Height = FMath::Max(180.f, Canvas->SizeY - 90.f);
 const FVector2D TopLeft((Canvas->SizeX - Width)*0.5f,(Canvas->SizeY - Height)*0.5f);
 const FVector2D Centre(Canvas->SizeX*0.5f, Canvas->SizeY*0.5f);
 const FVector Player = Hero->GetActorLocation();
 const float HalfWorldRange = 15000.f; // 150 metres from player
 const float Scale = FMath::Min((Width - 40.f),(Height - 75.f)) * 0.5f / HalfWorldRange;
 DrawRect(FLinearColor(0.025f,0.03f,0.07f,0.96f),TopLeft.X,TopLeft.Y,Width,Height);
 DrawText(TEXT("MOONFALL  |  EXPLORED WORLD"),FLinearColor(0.85f,0.87f,1.f),TopLeft.X+20.f,TopLeft.Y+12.f,nullptr,1.f);
 DrawText(TEXT("M / VIEW: CLOSE  |  UNEXPLORED SPACE IS BLANK"),FLinearColor(0.67f,0.73f,0.85f),
          TopLeft.X+20.f,TopLeft.Y+35.f,nullptr,0.8f);
 // Only cells the player has actually visited are charted, never speculative terrain.
 for (const FIntPoint& Cell : ExploredMapCells) {
  const FVector2D Offset((Cell.X+0.5f)*200.f-Player.X,(Cell.Y+0.5f)*200.f-Player.Y);
  if (FMath::Abs(Offset.X)>HalfWorldRange || FMath::Abs(Offset.Y)>HalfWorldRange) continue;
  const FVector2D Spot = Centre+FVector2D(Offset.X,-Offset.Y)*Scale;
  if (Spot.X<TopLeft.X+12.f || Spot.X>TopLeft.X+Width-12.f ||
      Spot.Y<TopLeft.Y+64.f || Spot.Y>TopLeft.Y+Height-12.f) continue;
  DrawRect(FLinearColor(0.22f,0.36f,0.49f,0.85f),Spot.X-2.f,Spot.Y-2.f,4.f,4.f);
 }
 if (Hero->bQuestTargetActive) {
  const FVector Delta = Hero->QuestTarget - Player;
  const FVector2D Raw(Delta.X,-Delta.Y);
  const FVector2D Bound(FMath::Clamp(Raw.X,-HalfWorldRange,HalfWorldRange),
                        FMath::Clamp(Raw.Y,-HalfWorldRange,HalfWorldRange));
  const FVector2D Target = Centre + Bound*Scale;
  DrawRect(FLinearColor(1.f,0.78f,0.22f),Target.X-5.f,Target.Y-5.f,10.f,10.f);
 }
 DrawRect(FLinearColor(0.26f,0.91f,1.f),Centre.X-5.f,Centre.Y-5.f,10.f,10.f);
 DrawText(TEXT("N"),FLinearColor(0.85f,0.9f,1.f),Centre.X-5.f,TopLeft.Y+65.f,nullptr,0.86f);
 DrawText(FString::Printf(TEXT("DISCOVERED: %d CELLS  |  VIEW RADIUS: 150 m"),ExploredMapCells.Num()),
          FLinearColor(0.78f,0.86f,0.95f),TopLeft.X+20.f,TopLeft.Y+Height-29.f,nullptr,0.85f);
}

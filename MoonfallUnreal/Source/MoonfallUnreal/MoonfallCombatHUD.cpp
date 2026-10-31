#include "MoonfallCombatHUD.h"
#include "MoonfallHeroPawn.h"
#include "MoonfallEnemy.h"
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
 if (Hero->bOptionsOpen) {
  DrawRect(FLinearColor(0.04f,0.03f,0.11f,0.94f),Canvas->SizeX*0.5f-280.f,Canvas->SizeY*0.5f-190.f,560.f,380.f);
  DrawText(TEXT("MOONFALL  |  GRAPHICS OPTIONS"),FLinearColor::White,Canvas->SizeX*0.5f-230.f,Canvas->SizeY*0.5f-150.f,nullptr,1.25f);
  UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
  if (Settings) {
   float ScalePct = 100.f, ScaleNormalized = 0.f, ScaleMin = 50.f, ScaleMax = 100.f;
   Settings->GetResolutionScaleInformationEx(ScalePct, ScaleNormalized, ScaleMin, ScaleMax);
   const FIntPoint Res = Settings->GetScreenResolution();
   const FString Rows[] = {FString::Printf(TEXT("Resolution: %d x %d"),Res.X,Res.Y),FString::Printf(TEXT("Render scale: %.0f%%"),ScalePct),FString::Printf(TEXT("Quality preset: %d (0 Low - 3 Epic)"),Settings->GetOverallScalabilityLevel()),FString::Printf(TEXT("Frame Generation: %s (%s)"),Hero->FrameGenerationPreference==0?TEXT("Off"):Hero->FrameGenerationPreference==1?TEXT("Auto"):TEXT("On"),Hero->bFrameGenerationPluginDetected?TEXT("plugin detected; inactive"):TEXT("plugin unavailable"))};
   for (int32 i=0;i<4;++i) DrawText(Rows[i],i==Hero->OptionsSelection?FLinearColor::Yellow:FLinearColor::White,Canvas->SizeX*0.5f-210.f,Canvas->SizeY*0.5f-95.f+i*53.f,nullptr,1.1f);
  }
  DrawText(TEXT("Arrow keys / D-pad: select and adjust    Esc / Start: close"),FLinearColor::White,Canvas->SizeX*0.5f-240.f,Canvas->SizeY*0.5f+155.f,nullptr,0.85f);
  return;
 }
 // Minimal, resolution-aware HUD. Keep the center of the screen clear for combat.
 const float Margin = 24.f;
 const float Width = FMath::Clamp(Canvas->SizeX * 0.21f, 190.f, 300.f);
 const float Height = 104.f;
 const float X = Margin;
 const float Y = Canvas->SizeY - Height - Margin;
 const float HealthFraction = FMath::Clamp(Hero->Health / 100.f, 0.f, 1.f);
 const float ManaFraction = FMath::Clamp(Hero->Mana / FMath::Max(1.f, Hero->MaxMana), 0.f, 1.f);
 const FLinearColor TextColor(0.91f, 0.90f, 0.96f);
 DrawRect(FLinearColor(0.025f, 0.026f, 0.055f, 0.80f), X-10.f, Y-9.f, Width+20.f, Height+12.f);
 DrawText(TEXT("HEALTH"), TextColor, X, Y, nullptr, 0.83f);
 DrawText(FString::Printf(TEXT("%.0f / 100"), FMath::Max(0.f, Hero->Health)), TextColor,
          X + Width - 102.f, Y, nullptr, 0.83f);
 DrawRect(FLinearColor(0.20f, 0.08f, 0.13f), X, Y+25.f, Width, 15.f);
 DrawRect(FLinearColor(0.86f, 0.13f, 0.24f), X, Y+25.f, Width*HealthFraction, 15.f);
 DrawText(TEXT("MANA"), TextColor, X, Y+51.f, nullptr, 0.83f);
 DrawText(FString::Printf(TEXT("%.0f / %.0f"), FMath::Max(0.f, Hero->Mana), Hero->MaxMana), TextColor,
          X + Width - 102.f, Y+51.f, nullptr, 0.83f);
 DrawRect(FLinearColor(0.06f, 0.12f, 0.24f), X, Y+75.f, Width, 15.f);
 DrawRect(FLinearColor(0.15f, 0.53f, 0.98f), X, Y+75.f, Width*ManaFraction, 15.f);
 // Only one unobtrusive shortcut hint; detailed graphics controls live in Options.
 DrawText(TEXT("ESC  OPTIONS"), FLinearColor(0.73f, 0.70f, 0.83f, 0.85f),
          FMath::Max(16.f, Canvas->SizeX - 142.f), Canvas->SizeY - 37.f, nullptr, 0.82f);
 for (TActorIterator<AMoonfallEnemy> It(GetWorld()); It; ++It) {
  const AMoonfallEnemy* Enemy = *It;
  const float Age = GetWorld()->GetTimeSeconds() - Enemy->LastHitTime;
  if (Age < 0.f || Age > 0.8f) continue;
  FVector2D Screen;
  if (GetOwningPlayerController() && GetOwningPlayerController()->ProjectWorldLocationToScreen(Enemy->GetActorLocation() + FVector(0,0,175.f + Age*65.f), Screen)) {
   DrawText(FString::Printf(TEXT("-%.0f"), Enemy->LastHitAmount), FLinearColor(1.f,0.84f,0.2f,1.f-Age/0.8f), Screen.X, Screen.Y, nullptr, 1.3f);
  }
 }
}

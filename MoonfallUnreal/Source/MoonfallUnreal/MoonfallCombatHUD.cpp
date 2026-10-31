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
 DrawText(TEXT("F6: Graphics quality    F7: Window / Fullscreen"), FLinearColor(0.85f,0.78f,1.f), 32.f, 25.f, nullptr, 1.f);
 if (Hero->bOptionsOpen) {
  DrawRect(FLinearColor(0.04f,0.03f,0.11f,0.94f),Canvas->SizeX*0.5f-280.f,Canvas->SizeY*0.5f-155.f,560.f,310.f);
  DrawText(TEXT("MOONFALL  |  GRAPHICS OPTIONS"),FLinearColor::White,Canvas->SizeX*0.5f-230.f,Canvas->SizeY*0.5f-120.f,nullptr,1.25f);
  UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
  if (Settings) {
   const FIntPoint Res = Settings->GetScreenResolution();
   const FString Rows[] = {FString::Printf(TEXT("Resolution: %d x %d"),Res.X,Res.Y),FString::Printf(TEXT("Render scale: %.0f%%"),Settings->GetResolutionScaleNormalized()*100.f),FString::Printf(TEXT("Quality preset: %d (0 Low - 3 Epic)"),Settings->GetOverallScalabilityLevel())};
   for (int32 i=0;i<3;++i) DrawText(Rows[i],i==Hero->OptionsSelection?FLinearColor::Yellow:FLinearColor::White,Canvas->SizeX*0.5f-210.f,Canvas->SizeY*0.5f-60.f+i*53.f,nullptr,1.1f);
  }
  DrawText(TEXT("Arrow keys / D-pad: select and adjust    Esc / Start: close"),FLinearColor::White,Canvas->SizeX*0.5f-240.f,Canvas->SizeY*0.5f+120.f,nullptr,0.85f);
  return;
 }
 const float X = 32.f, Y = Canvas->SizeY - 90.f, Width = 260.f;
 DrawRect(FLinearColor(0.07f,0.05f,0.11f,0.85f),X-6.f,Y-6.f,Width+12.f,68.f);
 DrawRect(FLinearColor(0.25f,0.06f,0.1f),X,Y,Width,22.f);
 DrawRect(FLinearColor(0.88f,0.12f,0.2f),X,Y,Width * FMath::Clamp(Hero->Health/100.f,0.f,1.f),22.f);
 DrawRect(FLinearColor(0.04f,0.13f,0.24f),X,Y+32.f,Width,18.f);
 DrawRect(FLinearColor(0.15f,0.55f,0.97f),X,Y+32.f,Width * FMath::Clamp(Hero->Mana/FMath::Max(1.f,Hero->MaxMana),0.f,1.f),18.f);
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

#include "MoonfallCombatHUD.h"
#include "MoonfallHeroPawn.h"
#include "MoonfallEnemy.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Canvas.h"
void AMoonfallCombatHUD::DrawHUD()
{
 Super::DrawHUD();
 if (!Canvas) return;
 const AMoonfallHeroPawn* Hero = Cast<AMoonfallHeroPawn>(UGameplayStatics::GetPlayerPawn(this,0));
 if (!IsValid(Hero)) return;
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

#include "MoonfallNorthPreviewGameMode.h"
#include "Engine/World.h"
#include "MoonfallHeroPawn.h"
#include "MoonfallCombatHUD.h"

AMoonfallNorthPreviewGameMode::AMoonfallNorthPreviewGameMode() {
 DefaultPawnClass=AMoonfallHeroPawn::StaticClass();
 HUDClass=AMoonfallCombatHUD::StaticClass();
 // No spawning in BeginPlay: preview geography only.
}

int32 AMoonfallNorthPreviewGameMode::GetOceanActivity() const {
 const float Now=GetWorld()?GetWorld()->GetTimeSeconds():0.f;
 if (!FMath::IsFinite(Now) || Now<0.f) return 0;
 return MoonfallOceanEvents::Activity(FMath::FloorToInt(FMath::Fmod(Now,180.f)));
}

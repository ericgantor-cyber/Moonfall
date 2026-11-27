#include "MoonfallNorthPreviewGameMode.h"
#include "MoonfallHeroPawn.h"
#include "MoonfallCombatHUD.h"

AMoonfallNorthPreviewGameMode::AMoonfallNorthPreviewGameMode() {
 DefaultPawnClass=AMoonfallHeroPawn::StaticClass();
 HUDClass=AMoonfallCombatHUD::StaticClass();
 // No spawning in BeginPlay: preview geography only.
}

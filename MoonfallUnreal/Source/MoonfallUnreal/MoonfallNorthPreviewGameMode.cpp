#include "MoonfallNorthPreviewGameMode.h"
#include "MoonfallHeroPawn.h"

AMoonfallNorthPreviewGameMode::AMoonfallNorthPreviewGameMode() {
 DefaultPawnClass=AMoonfallHeroPawn::StaticClass();
 // No spawning in BeginPlay: preview geography only.
}

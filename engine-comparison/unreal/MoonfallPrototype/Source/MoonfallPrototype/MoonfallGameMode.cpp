#include "MoonfallGameMode.h"
#include "MoonfallPawn.h"
#include "MoonfallHud.h"

AMoonfallGameMode::AMoonfallGameMode()
{
    DefaultPawnClass = AMoonfallPawn::StaticClass();
    HUDClass = AMoonfallHud::StaticClass();
}

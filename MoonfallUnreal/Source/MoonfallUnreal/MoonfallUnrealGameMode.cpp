#include "MoonfallUnrealGameMode.h"
#include "MoonfallHeroPawn.h"
AMoonfallUnrealGameMode::AMoonfallUnrealGameMode()
{
 DefaultPawnClass = AMoonfallHeroPawn::StaticClass();
}

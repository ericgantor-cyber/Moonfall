#include "MoonfallUnrealGameMode.h"
#include "MoonfallHeroPawn.h"
#include "MoonfallArena.h"
#include "MoonfallCombatHUD.h"
#include "Engine/World.h"
AMoonfallUnrealGameMode::AMoonfallUnrealGameMode()
{
 DefaultPawnClass = AMoonfallHeroPawn::StaticClass();
 HUDClass = AMoonfallCombatHUD::StaticClass();
}

void AMoonfallUnrealGameMode::BeginPlay() {
 Super::BeginPlay();
 if (!GetWorld()) return;
 UClass* SpawnClass = ArenaClass ? ArenaClass.Get() : AMoonfallArena::StaticClass();
 GetWorld()->SpawnActor<AMoonfallArena>(SpawnClass,FVector::ZeroVector,FRotator::ZeroRotator);
}

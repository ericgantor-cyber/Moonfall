#include "MoonfallUnrealGameMode.h"
#include "MoonfallHeroPawn.h"
#include "MoonfallArena.h"
#include "MoonfallDungeonLayout.h"
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
 // The authored starting arena is still available. Its southern central
 // gateway flows directly into a 14-chamber dungeon, without loading a level.
 GetWorld()->SpawnActor<AMoonfallArena>(FVector::ZeroVector, FRotator::ZeroRotator);
 const FVector DungeonOrigin(0.f, -2100.f, 0.f);
 const FRotator DungeonFacing(0.f, -90.f, 0.f);
 // Set before BeginPlay/OnConstruction via deferred spawning so chamber RNG
 // is generated from the intended zone and dungeon IDs.
 const FTransform DungeonTransform(DungeonFacing, DungeonOrigin);
 AMoonfallDungeonLayout* Dungeon = GetWorld()->SpawnActorDeferred<AMoonfallDungeonLayout>(
     AMoonfallDungeonLayout::StaticClass(), DungeonTransform);
 if (Dungeon) {
  Dungeon->ZoneIndex = 0;
  Dungeon->DungeonIndex = 0;
  Dungeon->LayoutSeed = 0;
  Dungeon->bRandomizeChambers = true;
  Dungeon->FinishSpawning(DungeonTransform);
 }
}

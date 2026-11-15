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
 GetWorld()->SpawnActor<AMoonfallArena>(FVector::ZeroVector, FRotator::ZeroRotator);
 // First playable dungeon starts just south of the arena gate. No loading
 // transition, spawn wave or overwriting authored startup-map actors.
 // Rotating local +X to world -Y keeps the dungeon connected to the road.
 const FTransform DungeonTransform(FRotator(0.f, -90.f, 0.f),
   FVector(0.f, -3100.f, 0.f));
 AMoonfallDungeonLayout* Dungeon = GetWorld()->SpawnActorDeferred<AMoonfallDungeonLayout>(
   AMoonfallDungeonLayout::StaticClass(), DungeonTransform, nullptr, nullptr,
   ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
 if (IsValid(Dungeon)) {
  Dungeon->ZoneIndex = 0;
  Dungeon->DungeonIndex = 0;
  Dungeon->bRandomizeChambers = true;
  Dungeon->FinishSpawning(DungeonTransform);
 }
}

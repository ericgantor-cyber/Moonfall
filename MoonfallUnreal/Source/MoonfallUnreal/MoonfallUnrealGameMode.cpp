#include "MoonfallUnrealGameMode.h"
#include "MoonfallHeroPawn.h"
#include "MoonfallArena.h"
#include "MoonfallCombatHUD.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
AMoonfallUnrealGameMode::AMoonfallUnrealGameMode()
{
 DefaultPawnClass = AMoonfallHeroPawn::StaticClass();
 HUDClass = AMoonfallCombatHUD::StaticClass();
}

void AMoonfallUnrealGameMode::BeginPlay() {
 Super::BeginPlay();
 UWorld* World = GetWorld();
 if (!World) return;
 // Level-authored arenas take precedence; do not duplicate their enemies.
 for (TActorIterator<AMoonfallArena> Existing(World); Existing; ++Existing) {
  return;
 }
 // Configure dungeon/zone before BeginPlay, so the 25% dungeon roll
 // cannot run prematurely or leak into open-world encounters.
 const FTransform ArenaTransform(FRotator::ZeroRotator, FVector::ZeroVector);
 AMoonfallArena* Arena = Cast<AMoonfallArena>(
  UGameplayStatics::BeginDeferredActorSpawnFromClass(
   this, AMoonfallArena::StaticClass(), ArenaTransform));
 if (!Arena) return;
 Arena->bIsDungeonEncounter = bStartInDungeonEncounter;
 Arena->SpawnBiome = StartingSpawnBiome;
 UGameplayStatics::FinishSpawningActor(Arena, ArenaTransform);
}

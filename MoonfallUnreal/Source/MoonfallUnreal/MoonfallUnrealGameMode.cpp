#include "MoonfallUnrealGameMode.h"
#include "MoonfallHeroPawn.h"
#include "MoonfallArena.h"
#include "MoonfallDungeonLayout.h"
#include "MoonfallDungeonRoad.h"
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
 // The world road provides a continuous ground-level route to all 30
 // physically generated dungeon layouts; no extra game mode or teleport.
 GetWorld()->SpawnActor<AMoonfallDungeonRoad>(FVector::ZeroVector, FRotator::ZeroRotator);
 const FRotator DungeonFacing(0.f, -90.f, 0.f);
 for (int32 ZoneIndex = 0; ZoneIndex < AMoonfallHeroPawn::DungeonZoneCount; ++ZoneIndex) {
  for (int32 DungeonIndex = 0; DungeonIndex < AMoonfallHeroPawn::DungeonsPerZone; ++DungeonIndex) {
   const FVector DungeonOrigin = AMoonfallDungeonRoad::GetDungeonOrigin(ZoneIndex, DungeonIndex);
   const FTransform DungeonTransform(DungeonFacing, DungeonOrigin);
   // Deferred spawn assigns the IDs before randomized rooms are constructed.
   AMoonfallDungeonLayout* Dungeon = GetWorld()->SpawnActorDeferred<AMoonfallDungeonLayout>(
       AMoonfallDungeonLayout::StaticClass(), DungeonTransform);
   if (!Dungeon) continue;
   Dungeon->ZoneIndex = ZoneIndex;
   Dungeon->DungeonIndex = DungeonIndex;
   Dungeon->LayoutSeed = 0; // Deterministic unique seed per zone+dungeon.
   Dungeon->bRandomizeChambers = true;
   Dungeon->FinishSpawning(DungeonTransform);
  }
 }
}

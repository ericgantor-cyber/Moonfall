#include "MoonfallUnrealGameMode.h"
#include "MoonfallHeroPawn.h"
#include "MoonfallArena.h"
#include "MoonfallCombatHUD.h"
#include "Engine/World.h"
#include "EngineUtils.h"
AMoonfallUnrealGameMode::AMoonfallUnrealGameMode()
{
 DefaultPawnClass = AMoonfallHeroPawn::StaticClass();
 HUDClass = AMoonfallCombatHUD::StaticClass();
}

void AMoonfallUnrealGameMode::BeginPlay() {
 Super::BeginPlay();
 if (!GetWorld()) return;
 // Respect an Arena placed intentionally in a level. Duplicate world coordinators
 // would spawn duplicate civilians/enemies and race to write the same save slot.
 for (TActorIterator<AMoonfallArena> Existing(GetWorld()); Existing; ++Existing)
  if (IsValid(*Existing)) return;
 GetWorld()->SpawnActor<AMoonfallArena>(FVector::ZeroVector, FRotator::ZeroRotator);
}

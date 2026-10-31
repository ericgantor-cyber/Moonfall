#include "MoonfallUnrealGameMode.h"
#include "MoonfallHeroPawn.h"
#include "MoonfallArena.h"
#include "Engine/World.h"
AMoonfallUnrealGameMode::AMoonfallUnrealGameMode()
{
 DefaultPawnClass = AMoonfallHeroPawn::StaticClass();
}

void AMoonfallUnrealGameMode::BeginPlay() {
 Super::BeginPlay();
 if (GetWorld()) GetWorld()->SpawnActor<AMoonfallArena>(FVector::ZeroVector, FRotator::ZeroRotator);
}

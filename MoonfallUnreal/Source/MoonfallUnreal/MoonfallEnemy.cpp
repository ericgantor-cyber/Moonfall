#include "MoonfallEnemy.h"
#include "Components/StaticMeshComponent.h"
AMoonfallEnemy::AMoonfallEnemy() {
  Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
  SetRootComponent(Body);
  PrimaryActorTick.bCanEverTick = false;
}
void AMoonfallEnemy::TakeSpellDamage(float Amount) {
  Health = FMath::Max(0.0f, Health - FMath::Max(0.0f, Amount));
  if (Health <= 0.0f) { SetActorHiddenInGame(true); SetActorEnableCollision(false); }
}

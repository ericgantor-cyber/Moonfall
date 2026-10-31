#include "MoonfallEnemy.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
AMoonfallEnemy::AMoonfallEnemy() {
  Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
  SetRootComponent(Body);
  static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
  if (Sphere.Succeeded()) Body->SetStaticMesh(Sphere.Object);
  Body->SetCollisionProfileName(TEXT("BlockAll"));
  Body->SetRelativeScale3D(FVector(0.85f,0.85f,1.4f));
  PrimaryActorTick.bCanEverTick = false;
}
void AMoonfallEnemy::TakeSpellDamage(float Amount) {
  Health = FMath::Max(0.0f, Health - FMath::Max(0.0f, Amount));
  if (Health <= 0.0f) { SetActorHiddenInGame(true); SetActorEnableCollision(false); }
}

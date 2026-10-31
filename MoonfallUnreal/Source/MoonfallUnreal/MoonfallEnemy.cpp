#include "MoonfallEnemy.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "MoonfallHeroPawn.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
AMoonfallEnemy::AMoonfallEnemy() {
  Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
  SetRootComponent(Body);
  static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
  if (Sphere.Succeeded()) Body->SetStaticMesh(Sphere.Object);
  Body->SetCollisionProfileName(TEXT("BlockAll"));
  Body->SetRelativeScale3D(FVector(0.85f,0.85f,1.4f));
  PrimaryActorTick.bCanEverTick = true;
  PrimaryActorTick.TickInterval = 0.1f;
}
void AMoonfallEnemy::TakeSpellDamage(float Amount) {
  if (Health <= 0.f) return;
  Health = FMath::Max(0.0f, Health - FMath::Max(0.0f, Amount));
  OnHitFeedback(Amount, Health);
  if (Health <= 0.0f) { SetActorHiddenInGame(true); SetActorEnableCollision(false); }
}

void AMoonfallEnemy::Tick(float DeltaSeconds) {
 Super::Tick(DeltaSeconds);
 if (Health <= 0.f || !GetWorld()) return;
 AMoonfallHeroPawn* Hero = Cast<AMoonfallHeroPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
 if (!IsValid(Hero) || Hero->Health <= 0.f) return;
 FVector Difference = Hero->GetActorLocation() - GetActorLocation();
 Difference.Z = 0.f;
 const float Distance = Difference.Size();
 if (Distance > AttackRange && Distance < 2200.f) {
  FHitResult Hit;
  AddActorWorldOffset(Difference.GetSafeNormal() * ChaseSpeed * DeltaSeconds, true, &Hit);
 } else if (Distance <= AttackRange && GetWorld()->GetTimeSeconds() >= NextAttackAt) {
  NextAttackAt = GetWorld()->GetTimeSeconds() + AttackCooldown;
  Hero->Health = FMath::Max(0.f, Hero->Health - AttackDamage);
  OnEnemyAttack();
 }
}

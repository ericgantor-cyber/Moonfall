#include "MoonfallEnemy.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "MoonfallHeroPawn.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "DrawDebugHelpers.h"
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
  // Hit feedback is now tracked by the HUD rather than persistent debug strings.
  LastHitAmount = Amount;
  LastHitTime = GetWorld() ? GetWorld()->GetTimeSeconds() : -10.f;
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
  WindupEndsAt = -1.f;
  FHitResult Hit;
  FVector Target = Hero->GetActorLocation();
  if (UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld())) {
   FNavLocation Point;
   if (Nav->ProjectPointToNavigation(Target, Point)) {
    UNavigationPath* Path = Nav->FindPathToLocationSynchronously(GetWorld(), GetActorLocation(), Point.Location, this);
    if (IsValid(Path) && !Path->IsPartial() && Path->PathPoints.Num() > 1) Target = Path->PathPoints[1];
   }
  }
  FVector Direction = Target - GetActorLocation();
  Direction.Z = 0.f;
  AddActorWorldOffset(Direction.GetSafeNormal() * ChaseSpeed * DeltaSeconds, true, &Hit);
 } else if (Distance <= AttackRange) {
  const float Now = GetWorld()->GetTimeSeconds();
  if (WindupEndsAt > 0.f && Now >= WindupEndsAt) {
   WindupEndsAt = -1.f;
   NextAttackAt = Now + AttackCooldown;
   if (Distance <= AttackRange) { Hero->ApplyMoonfallDamage(AttackDamage); OnEnemyAttack(); }
  } else if (WindupEndsAt < 0.f && Now >= NextAttackAt) {
   WindupEndsAt = Now + 0.5f;
  }
  if (WindupEndsAt > 0.f) {
   DrawDebugSphere(GetWorld(), GetActorLocation(), AttackRange, 16, FColor::Red, false, 0.12f);
   DrawDebugString(GetWorld(), GetActorLocation() + FVector(0.f,0.f,160.f), TEXT("ATTACK"), nullptr, FColor::Red, 0.12f, false);
  }
 } else {
  WindupEndsAt = -1.f;
 }
}

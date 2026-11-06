#include "MoonfallBirdFlock.h"
#include "Components/SceneComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "MoonfallHeroPawn.h"
#include "Engine/World.h"

AMoonfallBirdFlock::AMoonfallBirdFlock()
{
 PrimaryActorTick.bCanEverTick = true;
 PrimaryActorTick.TickInterval = 0.05f; // At most 20 flock updates per second.
 SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("FlockRoot"));
 SetRootComponent(SceneRoot);
 Bodies = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("BirdBodies"));
 LeftWings = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("BirdLeftWings"));
 RightWings = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("BirdRightWings"));
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
 for (UInstancedStaticMeshComponent* Part : {Bodies, LeftWings, RightWings}) {
  Part->SetupAttachment(SceneRoot);
  Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  Part->SetCastShadow(false);
  Part->SetMobility(EComponentMobility::Movable);
  if (Cube.Succeeded()) Part->SetStaticMesh(Cube.Object);
 }
}
void AMoonfallBirdFlock::BeginPlay()
{
 Super::BeginPlay();
 BirdCount = FMath::Clamp(BirdCount, 0, 16);
 FlockRadius = FMath::Clamp(FlockRadius, 200.f, 2000.f);
 FlyingHeight = FMath::Clamp(FlyingHeight, 300.f, 2200.f);
 for (int32 i=0; i<BirdCount; ++i) {
  const FTransform Initial(FRotator::ZeroRotator, FVector::ZeroVector, FVector(0.01f));
  Bodies->AddInstance(Initial);
  LeftWings->AddInstance(Initial);
  RightWings->AddInstance(Initial);
 }
 UpdateFlock(0.f);
}
void AMoonfallBirdFlock::Tick(float DeltaSeconds)
{
 Super::Tick(DeltaSeconds);
 if (!GetWorld() || BirdCount == 0) return;
 const AMoonfallHeroPawn* Hero = Cast<AMoonfallHeroPawn>(UGameplayStatics::GetPlayerPawn(this,0));
 if (!IsValid(Hero) || Hero->bOptionsOpen || Hero->bWorldMapOpen ||
     FVector::DistSquared2D(Hero->GetActorLocation(), GetActorLocation()) > FMath::Square(3500.f)) return;
 UpdateFlock(GetWorld()->GetTimeSeconds());
}
void AMoonfallBirdFlock::UpdateFlock(float TimeSeconds)
{
 // A single flock performs all motions using three instanced mesh draw groups.
 // Stable phases provide individually moving birds with no actor spawning during Tick.
 for (int32 i=0; i<BirdCount; ++i) {
  const float Seed = static_cast<float>(i)*2.399963f;
  const float Angle = TimeSeconds*(0.32f + (i%3)*0.065f) + Seed;
  const float Orbit = FlockRadius*(0.72f + 0.25f*FMath::Sin(Seed*1.7f));
  const FVector Location(Orbit*FMath::Cos(Angle),
                         Orbit*FMath::Sin(Angle),
                         FlyingHeight + 110.f*FMath::Sin(TimeSeconds*0.7f+Seed));
  const float Heading = FMath::RadiansToDegrees(Angle + PI*0.5f);
  const float Flap = FMath::Sin(TimeSeconds*5.8f+Seed*2.f)*26.f;
  const bool bDirty = (i == BirdCount-1);
  Bodies->SetInstanceTransform(i,FTransform(FRotator(0.f,Heading,0.f),Location,FVector(0.44f,0.17f,0.095f)),
                               false,bDirty,true);
  LeftWings->SetInstanceTransform(i,FTransform(FRotator(Flap,Heading+14.f,0.f),
                                Location+FVector(0.f,0.f,6.f),FVector(0.28f,0.52f,0.035f)),false,bDirty,true);
  RightWings->SetInstanceTransform(i,FTransform(FRotator(-Flap,Heading-14.f,0.f),
                                 Location+FVector(0.f,0.f,-6.f),FVector(0.28f,0.52f,0.035f)),false,bDirty,true);
 }
}

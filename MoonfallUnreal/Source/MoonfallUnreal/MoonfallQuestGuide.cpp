#include "MoonfallQuestGuide.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

AMoonfallQuestGuide::AMoonfallQuestGuide()
{
 // Half-second scheduler; no Behavior Tree, controller or expensive path search.
 PrimaryActorTick.bCanEverTick = true;
 PrimaryActorTick.TickInterval = 0.5f;
 Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GuideBody"));
 SetRootComponent(Body);
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
 if (Cylinder.Succeeded()) Body->SetStaticMesh(Cylinder.Object);
 Body->SetRelativeScale3D(FVector(0.45f, 0.45f, 1.55f));
 // NPCs do not block players, but navigation sweeps must stop at world walls.
 Body->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
 Body->SetCollisionResponseToAllChannels(ECR_Ignore);
 Body->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);

 Head = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GuideHead"));
 Head->SetupAttachment(Body);
 if (Sphere.Succeeded()) Head->SetStaticMesh(Sphere.Object);
 // Root scales affect attached pieces, so offsets are intentionally local.
 Head->SetRelativeLocation(FVector(0.f, 0.f, 57.f));
 Head->SetRelativeScale3D(FVector(1.3f, 1.3f, 0.42f));
 Head->SetCollisionEnabled(ECollisionEnabled::NoCollision);

 LeftArm = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GuideLeftArm"));
 LeftArm->SetupAttachment(Body);
 if (Cube.Succeeded()) LeftArm->SetStaticMesh(Cube.Object);
 LeftArm->SetRelativeLocation(FVector(0.f, -88.f, 7.f));
 LeftArm->SetRelativeScale3D(FVector(0.40f, 0.32f, 0.64f));
 LeftArm->SetRelativeRotation(FRotator(-15.f, 0.f, -14.f));
 LeftArm->SetCollisionEnabled(ECollisionEnabled::NoCollision);

 RightArm = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GuideRightArm"));
 RightArm->SetupAttachment(Body);
 if (Cube.Succeeded()) RightArm->SetStaticMesh(Cube.Object);
 RightArm->SetRelativeLocation(FVector(0.f, 88.f, 7.f));
 RightArm->SetRelativeScale3D(FVector(0.40f, 0.32f, 0.64f));
 RightArm->SetRelativeRotation(FRotator(-15.f, 0.f, 14.f));
 RightArm->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

FText AMoonfallQuestGuide::GetGuideTitle() const
{
 static const TCHAR* Titles[] = {
  TEXT("Ash Warden"), TEXT("Mire Keeper"), TEXT("Frost Herald"),
  TEXT("Storm Oracle"), TEXT("Foundry Chronicler"), TEXT("Eclipse Seer")
 };
 return FText::FromString(Titles[FMath::Clamp(RegionIndex, 0, 5)]);
}

void AMoonfallQuestGuide::BeginPlay()
{
 Super::BeginPlay();
 HomeLocation = GetActorLocation();
}
void AMoonfallQuestGuide::Tick(float DeltaSeconds)
{
 Super::Tick(DeltaSeconds);
 if (!GetWorld()) return;
 const APawn* Hero = UGameplayStatics::GetPlayerPawn(this, 0);
 if (!IsValid(Hero) || FVector::DistSquared2D(Hero->GetActorLocation(), HomeLocation)
     > FMath::Square(2600.f)) return;
 const float Now = GetWorld()->GetTimeSeconds();
 if (!FMath::IsFinite(Now)) return;
 // Different region guides offset their watch, market and rest cycles.
 // The world clock never forces a quest deadline or skips player exploration.
 CurrentRoutine = (FMath::FloorToInt(Now / 90.f) + FMath::Clamp(RegionIndex, 0, 5)) % 3;
 const FVector Waypoints[] = {HomeLocation,
   HomeLocation + FVector(180.f, 65.f, 0.f),
   HomeLocation + FVector(-130.f, -100.f, 0.f)};
 const FVector Goal = Waypoints[CurrentRoutine];
 const FVector Here = GetActorLocation();
 const FVector ToGoal = FVector(Goal.X - Here.X, Goal.Y - Here.Y, 0.f);
 const float Distance = ToGoal.Size();
 if (Distance < 10.f) return;
 // Small, collision-aware steps. Never teleport, go vertical or clip a wall.
 const float Step = FMath::Min(Distance, 90.f * FMath::Clamp(DeltaSeconds, 0.f, 0.5f));
 FHitResult Hit;
 AddActorWorldOffset(ToGoal.GetSafeNormal() * Step, true, &Hit);
}
FText AMoonfallQuestGuide::GetRoutineLabel() const
{
 static const TCHAR* Routines[] = {TEXT("Keeping watch"), TEXT("Visiting the market"),
   TEXT("Resting at camp")};
 return FText::FromString(Routines[FMath::Clamp(CurrentRoutine, 0, 2)]);
}

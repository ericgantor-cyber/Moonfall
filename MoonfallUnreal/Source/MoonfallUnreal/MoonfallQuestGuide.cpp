#include "MoonfallQuestGuide.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

AMoonfallQuestGuide::AMoonfallQuestGuide()
{
 PrimaryActorTick.bCanEverTick = false;
 Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GuideBody"));
 SetRootComponent(Body);
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
 if (Cylinder.Succeeded()) Body->SetStaticMesh(Cylinder.Object);
 Body->SetRelativeScale3D(FVector(0.45f, 0.45f, 1.55f));
 Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);

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

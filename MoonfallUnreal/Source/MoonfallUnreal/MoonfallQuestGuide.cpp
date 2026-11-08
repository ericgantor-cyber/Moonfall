#include "MoonfallQuestGuide.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

AMoonfallQuestGuide::AMoonfallQuestGuide()
{
 PrimaryActorTick.bCanEverTick = false;
 Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GuideBody"));
 SetRootComponent(Body);
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
 if (Mesh.Succeeded()) Body->SetStaticMesh(Mesh.Object);
 Body->SetRelativeScale3D(FVector(0.6f, 0.6f, 2.f));
 Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

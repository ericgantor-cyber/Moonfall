#include "MoonfallGrottoTreasure.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AMoonfallGrottoTreasure::AMoonfallGrottoTreasure()
{
    PrimaryActorTick.bCanEverTick = false;
    PickupZone = CreateDefaultSubobject<USphereComponent>(TEXT("PickupZone"));
    SetRootComponent(PickupZone);
    PickupZone->InitSphereRadius(180.f);
    PickupZone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PickupZone->SetCollisionObjectType(ECC_WorldDynamic);
    PickupZone->SetCollisionResponseToAllChannels(ECR_Ignore);
    PickupZone->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    PickupZone->SetGenerateOverlapEvents(false);
    ChestMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ChestMesh"));
    ChestMesh->SetupAttachment(PickupZone);
    ChestMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ChestMesh->SetRelativeScale3D(FVector(0.9f, 0.6f, 0.5f));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (Cube.Succeeded()) ChestMesh->SetStaticMesh(Cube.Object);
}

// Keep the class loadable for older preview maps, but retire its gameplay.
void AMoonfallGrottoTreasure::BeginPlay()
{
    Super::BeginPlay();
    SetActorEnableCollision(false);
    SetActorHiddenInGame(true);
}

bool AMoonfallGrottoTreasure::TryClaim(AMoonfallHeroPawn*, bool)
{
    return false;
}

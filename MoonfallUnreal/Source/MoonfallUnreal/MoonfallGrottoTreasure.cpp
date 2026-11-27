#include "MoonfallGrottoTreasure.h"
#include "MoonfallHeroPawn.h"
#include "MoonfallNorthPreviewGameMode.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AMoonfallGrottoTreasure::AMoonfallGrottoTreasure()
{
    PrimaryActorTick.bCanEverTick = false;
    PickupZone = CreateDefaultSubobject<USphereComponent>(TEXT("PickupZone"));
    SetRootComponent(PickupZone);
    PickupZone->InitSphereRadius(180.f);
    PickupZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    PickupZone->SetCollisionObjectType(ECC_WorldDynamic);
    PickupZone->SetCollisionResponseToAllChannels(ECR_Ignore);
    PickupZone->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    PickupZone->SetGenerateOverlapEvents(true);
    ChestMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ChestMesh"));
    ChestMesh->SetupAttachment(PickupZone);
    ChestMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ChestMesh->SetRelativeScale3D(FVector(0.9f, 0.6f, 0.5f));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (Cube.Succeeded()) ChestMesh->SetStaticMesh(Cube.Object);
}

bool AMoonfallGrottoTreasure::TryClaim(AMoonfallHeroPawn* Hero)
{
    if (!HasAuthority() || IsActorBeingDestroyed() || !GetWorld() || !IsValid(Hero)
        || Hero->GetWorld() != GetWorld() || !FMath::IsFinite(Hero->Health)
        || Hero->Health <= 0.f || !Hero->bWaterSwimming
        || Hero->bOptionsOpen || Hero->bInventoryOpen || Hero->bAtlasOpen
        || Hero->bCharacterStatsOpen || Hero->bResidentContextOpen || Hero->bTalentWebOpen)
        return false;
    auto* Mode = GetWorld()->GetAuthGameMode<AMoonfallNorthPreviewGameMode>();
    if (!IsValid(Mode)) return false;
    const FVector Start = Hero->GetActorLocation();
    const FVector End = GetActorLocation();
    if (Start.ContainsNaN() || End.ContainsNaN()
        || FVector::DistSquared(Start, End) > FMath::Square(250.f)) return false;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(MoonfallGrottoTreasureSight), false);
    Params.AddIgnoredActor(Hero);
    Params.AddIgnoredActor(this);
    FHitResult Hit;
    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params)) return false;
    if (!Mode->ClaimGrotto(GrottoIndex)) return false;
    Destroy();
    return true;
}

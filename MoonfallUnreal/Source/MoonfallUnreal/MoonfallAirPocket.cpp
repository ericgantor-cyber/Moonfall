#include "MoonfallAirPocket.h"
#include "Components/SphereComponent.h"

AMoonfallAirPocket::AMoonfallAirPocket()
{
    PrimaryActorTick.bCanEverTick = false;
    BreathingZone = CreateDefaultSubobject<USphereComponent>(TEXT("BreathingZone"));
    SetRootComponent(BreathingZone);
    BreathingZone->InitSphereRadius(160.f);
    BreathingZone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BreathingZone->SetCollisionObjectType(ECC_WorldDynamic);
    BreathingZone->SetCollisionResponseToAllChannels(ECR_Ignore);
    BreathingZone->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    BreathingZone->SetGenerateOverlapEvents(false);
}

void AMoonfallAirPocket::BeginPlay()
{
    Super::BeginPlay();
    SetActorEnableCollision(false);
    SetActorHiddenInGame(true);
}

bool AMoonfallAirPocket::ContainsEye(const FVector&) const
{
    return false;
}

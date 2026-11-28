#include "MoonfallAirPocket.h"
#include "Components/SphereComponent.h"

AMoonfallAirPocket::AMoonfallAirPocket()
{
    PrimaryActorTick.bCanEverTick = false;
    BreathingZone = CreateDefaultSubobject<USphereComponent>(TEXT("BreathingZone"));
    SetRootComponent(BreathingZone);
    BreathingZone->InitSphereRadius(160.f);
    BreathingZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    BreathingZone->SetCollisionObjectType(ECC_WorldDynamic);
    BreathingZone->SetCollisionResponseToAllChannels(ECR_Ignore);
    BreathingZone->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    BreathingZone->SetGenerateOverlapEvents(true);
}

bool AMoonfallAirPocket::ContainsEye(const FVector& EyeLocation) const
{
    if (IsActorBeingDestroyed() || !GetActorEnableCollision() || !IsValid(BreathingZone)
        || !BreathingZone->IsQueryCollisionEnabled() || EyeLocation.ContainsNaN()) return false;
    const FVector Center = BreathingZone->GetComponentLocation();
    const float Radius = BreathingZone->GetScaledSphereRadius();
    const double DistanceSquared=FVector::DistSquared(EyeLocation, Center);
    return !Center.ContainsNaN() && FMath::IsFinite(Radius) && Radius > 0.f
        && FMath::IsFinite(DistanceSquared) && DistanceSquared <= FMath::Square(double(Radius));
}

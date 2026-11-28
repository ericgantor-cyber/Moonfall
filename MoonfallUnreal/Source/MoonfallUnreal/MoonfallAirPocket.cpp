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

bool AMoonfallAirPocket::GetBreathingSphere(FVector& Center,float& Radius) const
{
    Center=FVector::ZeroVector; Radius=0.f;
    if (IsActorBeingDestroyed() || !GetActorEnableCollision() || !IsValid(BreathingZone)
        || !BreathingZone->IsQueryCollisionEnabled()) return false;
    Center=BreathingZone->GetComponentLocation();
    Radius=BreathingZone->GetScaledSphereRadius();
    return !Center.ContainsNaN() && FMath::IsFinite(Radius) && Radius>0.f;
}

bool AMoonfallAirPocket::ContainsEye(const FVector& EyeLocation) const
{
    FVector Center; float Radius=0.f;
    if (EyeLocation.ContainsNaN() || !GetBreathingSphere(Center,Radius)) return false;
    const double DistanceSquared=FVector::DistSquared(EyeLocation,Center);
    return FMath::IsFinite(DistanceSquared) && DistanceSquared<=FMath::Square(double(Radius));
}

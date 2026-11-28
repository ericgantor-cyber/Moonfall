#include "MoonfallAirPocket.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

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
    // Three static greybox bubble markers inside the breathing radius.
    // The level artist can assign a translucent bubble material later.
    // Never add collision, ticking actors or a second breathing mechanic.
    static ConstructorHelpers::FObjectFinder<UStaticMesh> BubbleMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    const FName Names[] = {TEXT("AirBubbleSmall"),TEXT("AirBubbleMedium"),TEXT("AirBubbleLarge")};
    const FVector Offsets[] = {FVector(-24,0,-55),FVector(18,0,-5),FVector(-8,0,50)};
    const float Scales[] = {0.16f,0.24f,0.32f};
    for (int32 I=0; I<3; ++I)
    {
        auto* Bubble = CreateDefaultSubobject<UStaticMeshComponent>(Names[I]);
        Bubble->SetupAttachment(BreathingZone);
        Bubble->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Bubble->SetGenerateOverlapEvents(false);
        Bubble->SetCastShadow(false);
        Bubble->SetRelativeLocation(Offsets[I]);
        Bubble->SetRelativeScale3D(FVector(Scales[I]));
        if (BubbleMesh.Succeeded()) Bubble->SetStaticMesh(BubbleMesh.Object);
    }
}

bool AMoonfallAirPocket::ContainsEye(const FVector& EyeLocation) const
{
    if (!IsValid(BreathingZone) || EyeLocation.ContainsNaN()) return false;
    const FVector Center = BreathingZone->GetComponentLocation();
    const float Radius = BreathingZone->GetScaledSphereRadius();
    return !Center.ContainsNaN() && FMath::IsFinite(Radius) && Radius > 0.f
        && FVector::DistSquared(EyeLocation, Center) <= FMath::Square(Radius);
}

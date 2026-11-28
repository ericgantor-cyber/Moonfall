#include "MoonfallAirPocket.h"
#include "MoonfallHeroPawn.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AMoonfallAirPocket::AMoonfallAirPocket()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bAllowTickOnDedicatedServer = false;
    PrimaryActorTick.TickInterval = 0.1f;
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
    // Presentation only: no collision or second breathing mechanic.
    static ConstructorHelpers::FObjectFinder<UStaticMesh> BubbleMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    const FName Names[] = {TEXT("AirBubbleSmall"),TEXT("AirBubbleMedium"),TEXT("AirBubbleLarge")};
    const FVector Offsets[] = {FVector(-24,0,-55),FVector(18,0,-5),FVector(-8,0,50)};
    const float Scales[] = {0.16f,0.24f,0.32f};
    for (int32 I=0; I<3; ++I)
    {
        auto* Bubble = CreateDefaultSubobject<UStaticMeshComponent>(Names[I]);
        Bubble->SetupAttachment(BreathingZone);
        Bubble->SetMobility(EComponentMobility::Movable);
        BubbleMarkers.Add(Bubble);
        Bubble->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Bubble->SetGenerateOverlapEvents(false);
        Bubble->SetCastShadow(false);
        Bubble->SetRelativeLocation(Offsets[I]);
        Bubble->SetRelativeScale3D(FVector(Scales[I]));
        if (BubbleMesh.Succeeded()) Bubble->SetStaticMesh(BubbleMesh.Object);
    }
}

void AMoonfallAirPocket::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const auto* World = GetWorld();
    const auto* PC = World ? World->GetFirstPlayerController() : nullptr;
    const auto* Hero = IsValid(PC) ? Cast<AMoonfallHeroPawn>(PC->GetPawn()) : nullptr;
    const double Now = World ? World->GetTimeSeconds() : 0.0;
    const bool bAnimate = bAnimateBubbles && IsValid(Hero) && Hero->IsLocallyControlled()
        && Hero->bWaterSwimming && Hero->bWaterSubmerged && !Hero->bReduceMotion
        && FMath::IsFinite(Hero->Health) && Hero->Health > 0.f
        && FMath::IsFinite(Now) && Now >= 0.0
        && !Hero->GetActorLocation().ContainsNaN() && !GetActorLocation().ContainsNaN()
        && FVector::DistSquared(Hero->GetActorLocation(), GetActorLocation()) <= FMath::Square(1500.f);
    if (!bAnimate && !bBubblesMoving) return;
    const float X[] = {-24.f,18.f,-8.f};
    const float RestZ[] = {-55.f,-5.f,50.f};
    for (int32 I=0; I<BubbleMarkers.Num() && I<3; ++I)
    {
        auto* Bubble=BubbleMarkers[I].Get();
        if (!IsValid(Bubble)) continue;
        const float Z=bAnimate ? -80.f+160.f*float(FMath::Fmod(Now/3.0+double(I)/3.0,1.0)) : RestZ[I];
        Bubble->SetRelativeLocation(FVector(X[I],0.f,Z));
    }
    bBubblesMoving=bAnimate;
}

bool AMoonfallAirPocket::ContainsEye(const FVector& EyeLocation) const
{
    if (!IsValid(BreathingZone) || EyeLocation.ContainsNaN()) return false;
    const FVector Center = BreathingZone->GetComponentLocation();
    const float Radius = BreathingZone->GetScaledSphereRadius();
    return !Center.ContainsNaN() && FMath::IsFinite(Radius) && Radius > 0.f
        && FVector::DistSquared(EyeLocation, Center) <= FMath::Square(Radius);
}

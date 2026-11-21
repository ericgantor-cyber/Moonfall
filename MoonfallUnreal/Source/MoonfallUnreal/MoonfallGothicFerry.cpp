#include "MoonfallGothicFerry.h"
#include "MoonfallHeroPawn.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/World.h"

AMoonfallGothicFerry::AMoonfallGothicFerry()
{
 PrimaryActorTick.bCanEverTick = false;
 SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("FerryRoot")));
 Hull = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GothicHull"));
 Hull->SetupAttachment(GetRootComponent());
 // Allow mouse/controller targeted interaction, but do not create a
 // physical barrier that knocks a character off a moving ferry.
 Hull->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
 Hull->SetCollisionResponseToAllChannels(ECR_Ignore);
 Hull->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
 Hull->SetGenerateOverlapEvents(false);

 Mast = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BlackMast"));
 Mast->SetupAttachment(GetRootComponent());
 Mast->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
 Mast->SetCollisionResponseToAllChannels(ECR_Ignore);
 Mast->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
 Mast->SetGenerateOverlapEvents(false);
 Sail = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NightSail"));
 Sail->SetupAttachment(GetRootComponent());
 Sail->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
 Sail->SetCollisionResponseToAllChannels(ECR_Ignore);
 Sail->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
 Sail->SetGenerateOverlapEvents(false);

 VesselName = CreateDefaultSubobject<UTextRenderComponent>(TEXT("VesselName"));
 VesselName->SetupAttachment(GetRootComponent());
 VesselName->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 VesselName->SetGenerateOverlapEvents(false);
 VesselName->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
 VesselName->SetWorldSize(95.f);
 VesselName->SetText(FText::FromString(TEXT("THE NIGHTFERRY")));
 VesselName->SetRelativeLocation(FVector(0.f, 0.f, 330.f));

 static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
 if (Cube.Succeeded()) {
  Hull->SetStaticMesh(Cube.Object);
  Mast->SetStaticMesh(Cube.Object);
  Sail->SetStaticMesh(Cube.Object);
 }
 // Placeholder gothic longboat silhouette: 7m beam, 12m keel,
 // 4m mast and a triangular-looking tall black sail proxy.
 Hull->SetRelativeLocation(FVector(0.f, 0.f, 6.f));
 Hull->SetRelativeScale3D(FVector(7.f, 12.f, 0.5f));
 Mast->SetRelativeLocation(FVector(0.f, 0.f, 205.f));
 Mast->SetRelativeScale3D(FVector(0.55f, 0.55f, 4.f));
 Sail->SetRelativeLocation(FVector(0.f, -100.f, 210.f));
 Sail->SetRelativeScale3D(FVector(5.1f, 0.32f, 2.8f));
}

FVector AMoonfallGothicFerry::DockLocation(bool bIsland)
{
 // Main pier extends north to Y=238000; island pier starts at Y=246000.
 // Both piers have collision at Z=0; the vessel crosses the 80m sea gap.
 return FVector(0.f, bIsland ? 246400.f : 237600.f, 0.f);
}

void AMoonfallGothicFerry::BeginPlay()
{
 Super::BeginPlay();
 // Authored placement may override this location in a future native map.
 // All current crossings use the two fixed, verified dock coordinates.
 SetActorLocation(DockLocation(bAtIsland));
}

float AMoonfallGothicFerry::GetCrossingProgress() const
{
 if (!bUnderway || !GetWorld()) return 0.f;
 const float Elapsed = GetWorld()->GetTimeSeconds() - CrossingStartedAt;
 return FMath::Clamp(Elapsed / CrossingDuration, 0.f, 1.f);
}

bool AMoonfallGothicFerry::TryBoard(AMoonfallHeroPawn* Hero)
{
 if (!IsValid(Hero) || !GetWorld() || bUnderway || Passenger.IsValid()
     || !FMath::IsFinite(Hero->Health) || Hero->Health <= 0.f
     || Hero->bOptionsOpen || Hero->bInventoryOpen || Hero->bAtlasOpen
     || Hero->bResidentContextOpen || Hero->GetWorld() != GetWorld())
  return false;
 const FVector Dock = DockLocation(bAtIsland);
 if (!FMath::IsFinite(FVector::DistSquared2D(Hero->GetActorLocation(), Dock))
     || FVector::DistSquared2D(Hero->GetActorLocation(), Dock) > FMath::Square(DockReach)
     || FMath::Abs(Hero->GetActorLocation().Z - Dock.Z) > 350.f)
  return false;
 UCharacterMovementComponent* Movement = Hero->GetCharacterMovement();
 if (!IsValid(Movement) || !Hero->GetCapsuleComponent()) return false;

 // Short boarding reposition puts the rider on deck, then actual crossing
 // is continuous movement of their attached root with the ship.
 const FVector BoardingLocation = GetActorLocation() + FVector(0.f, 0.f, 125.f);
 if (!Hero->SetActorLocation(BoardingLocation, false, nullptr, ETeleportType::TeleportPhysics))
  return false;
 if (!Hero->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform))
  return false;
 Movement->StopMovementImmediately();
 Movement->DisableMovement();
 Passenger = Hero;
 CrossingStartedAt = GetWorld()->GetTimeSeconds();
 bUnderway = true;
 GetWorldTimerManager().SetTimer(CrossingTimer, this,
   &AMoonfallGothicFerry::AdvanceCrossing, 0.05f, true);
 return true;
}

void AMoonfallGothicFerry::AdvanceCrossing()
{
 if (!bUnderway || !GetWorld()) return;
 AMoonfallHeroPawn* Rider = Passenger.Get();
 if (!IsValid(Rider) || !FMath::IsFinite(Rider->Health) || Rider->Health <= 0.f) {
  FinishCrossing(false);
  return;
 }
 const float Alpha = GetCrossingProgress();
 // A genuine moving boat for fourteen seconds, not a world swap or a
 // teleporter. Ease at both shores to avoid abrupt camera motion.
 const float Eased = FMath::InterpEaseInOut(0.f, 1.f, Alpha, 2.f);
 const FVector From = DockLocation(bAtIsland);
 const FVector To = DockLocation(!bAtIsland);
 SetActorLocation(FMath::Lerp(From, To, Eased));
 if (Alpha >= 1.f) FinishCrossing(true);
}

void AMoonfallGothicFerry::FinishCrossing(bool bArrived)
{
 if (!bUnderway) return;
 GetWorldTimerManager().ClearTimer(CrossingTimer);
 if (bArrived) bAtIsland = !bAtIsland;
 SetActorLocation(DockLocation(bAtIsland));
 bUnderway = false;
 if (AMoonfallHeroPawn* Rider = Passenger.Get()) {
  Rider->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
  // Deposit safely *on land* rather than leaving the character above
  // seawater, beneath the hull, or on the shoreless part of the pier.
  const float IntoLand = bAtIsland ? 800.f : -800.f;
  const FVector Landing = DockLocation(bAtIsland)
    + FVector(0.f, IntoLand, 125.f);
  Rider->SetActorLocation(Landing, false, nullptr, ETeleportType::TeleportPhysics);
  if (UCharacterMovementComponent* Movement = Rider->GetCharacterMovement())
   Movement->SetMovementMode(MOVE_Walking);
 }
 Passenger.Reset();
}

void AMoonfallGothicFerry::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
 if (bUnderway) FinishCrossing(false);
 if (GetWorld()) GetWorldTimerManager().ClearTimer(CrossingTimer);
 Super::EndPlay(EndPlayReason);
}

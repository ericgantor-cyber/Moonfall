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
 VoyageStatus = CreateDefaultSubobject<UTextRenderComponent>(TEXT("VoyageStatus"));
 VoyageStatus->SetupAttachment(GetRootComponent());
 VoyageStatus->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 VoyageStatus->SetGenerateOverlapEvents(false);
 VoyageStatus->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
 VoyageStatus->SetWorldSize(75.f);
 VoyageStatus->SetRelativeLocation(FVector(0.f, 0.f, 245.f));
 VoyageStatus->SetText(FText::GetEmpty());

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
 UpdateVoyageStatus();
}

float AMoonfallGothicFerry::GetCrossingProgress() const
{
 if (!bUnderway || !GetWorld()) return 0.f;
 const float Elapsed = GetWorld()->GetTimeSeconds() - CrossingStartedAt;
 return FMath::Clamp(Elapsed / CrossingDuration, 0.f, 1.f);
}

FText AMoonfallGothicFerry::GetDestinationName() const
{
 return bAtIsland ? FText::FromString(TEXT("Mainland Harbor"))
                  : FText::FromString(TEXT("Gothic Isle"));
}

float AMoonfallGothicFerry::GetSecondsUntilArrival() const
{
 if (!bUnderway || !GetWorld()) return 0.f;
 const float Elapsed = GetWorld()->GetTimeSeconds() - CrossingStartedAt;
 return FMath::Clamp(CrossingDuration - Elapsed, 0.f, CrossingDuration);
}

void AMoonfallGothicFerry::UpdateVoyageStatus()
{
 if (!VoyageStatus) return;
 if (!bUnderway) {
  // Stationary label invites players to board without a separate menu.
  // Mouse and controller both use existing bindings.
  LastDisplayedSeconds = -1;
  VoyageStatus->SetText(FText::FromString(FString::Printf(
   TEXT("TO %s | RIGHT-CLICK / D-PAD UP"),
   *GetDestinationName().ToString())));
  return;
 }
 // Only redraw when the displayed whole second changes, not 20 times/sec.
 const int32 Seconds = FMath::CeilToInt(GetSecondsUntilArrival());
 if (Seconds == LastDisplayedSeconds) return;
 LastDisplayedSeconds = Seconds;
 if (Seconds == 0) {
  // Arrival retries can continue while the dock is obstructed. Do not
  // mislead the passenger with a permanent zero-second countdown.
  VoyageStatus->SetText(FText::FromString(TEXT("WAITING FOR CLEAR DOCK")));
  return;
 }
 VoyageStatus->SetText(FText::FromString(FString::Printf(
  TEXT("TO %s  |  %ds"), *GetDestinationName().ToString(), Seconds)));
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
 const FVector OriginalLocation = Hero->GetActorLocation();
 FVector BoardingLocation = GetActorLocation() + FVector(0.f, 0.f, 125.f);
 // TeleportPhysics alone does not test clearance: reject an obstructed
 // boarding deck before moving the hero or disabling their controls.
 if (!GetWorld()->FindTeleportSpot(Hero, BoardingLocation, Hero->GetActorRotation()))
  return false;
 if (!Hero->SetActorLocation(BoardingLocation, false, nullptr, ETeleportType::TeleportPhysics))
  return false;
 if (!Hero->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform)) {
  // Failed attachment must never strand the pawn above water.
  Hero->SetActorLocation(OriginalLocation, false, nullptr, ETeleportType::TeleportPhysics);
  return false;
 }
 Movement->StopMovementImmediately();
 Movement->DisableMovement();
 Passenger = Hero;
 CrossingStartedAt = GetWorld()->GetTimeSeconds();
 bUnderway = true;
 UpdateVoyageStatus();
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
 UpdateVoyageStatus();
 const float Alpha = GetCrossingProgress();
 // A genuine moving boat for eleven seconds, not a world swap or a
 // teleporter. Ease at both shores to avoid abrupt camera motion.
 const float Eased = FMath::InterpEaseInOut(0.f, 1.f, Alpha, 2.f);
 const FVector From = DockLocation(bAtIsland);
 const FVector To = DockLocation(!bAtIsland);
 SetActorLocation(FMath::Lerp(From, To, Eased));
 if (Alpha >= 1.f) FinishCrossing(true);
}

void AMoonfallGothicFerry::FinishCrossing(bool bArrived)
{
 if (!bUnderway || !GetWorld()) return;
 AMoonfallHeroPawn* Rider = Passenger.Get();
 // Keep the ferry and passenger at the dock until a *clear* shore landing
 // exists. A blocked landing must not put a rider inside scenery or enable
 // walking while they are still attached to the vessel.
 const bool bDestinationIsland = bArrived ? !bAtIsland : bAtIsland;
 if (IsValid(Rider) && FMath::IsFinite(Rider->Health) && Rider->Health > 0.f) {
  const float IntoLand = bDestinationIsland ? 800.f : -800.f;
  FVector Landing = DockLocation(bDestinationIsland) + FVector(0.f, IntoLand, 125.f);
  if (!GetWorld()->FindTeleportSpot(Rider, Landing, Rider->GetActorRotation())) {
   // Do not complete or detach. The existing timer retries this landing
   // until the blocking obstruction is cleared.
   SetActorLocation(DockLocation(bDestinationIsland));
   return;
  }
  Rider->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
  if (!Rider->SetActorLocation(Landing, false, nullptr, ETeleportType::TeleportPhysics)) {
   // A failed placement should not leave the rider drifting independently.
   Rider->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
   SetActorLocation(DockLocation(bDestinationIsland));
   return;
  }
  if (UCharacterMovementComponent* Movement = Rider->GetCharacterMovement())
   Movement->SetMovementMode(MOVE_Walking);
 } else if (IsValid(Rider)) {
  // Death/cancellation must release the attachment. The respawn/death
  // pipeline controls movement and placement for an incapacitated hero.
  Rider->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
 }
 GetWorldTimerManager().ClearTimer(CrossingTimer);
 if (bArrived) bAtIsland = !bAtIsland;
 SetActorLocation(DockLocation(bAtIsland));
 bUnderway = false;
 Passenger.Reset();
 UpdateVoyageStatus();
}

void AMoonfallGothicFerry::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
 // Shutdown must never depend on a clear landing. FinishCrossing(false)
 // deliberately retries blocked shores during normal gameplay, which would
 // leave a passenger attached when this actor is actually being destroyed.
 if (GetWorld()) GetWorldTimerManager().ClearTimer(CrossingTimer);
 if (bUnderway) {
  if (AMoonfallHeroPawn* Rider = Passenger.Get(); IsValid(Rider)) {
   Rider->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
   // Recover a living passenger at the original dock if possible. If level
   // teardown has removed collision, still release movement/attachment.
   if (FMath::IsFinite(Rider->Health) && Rider->Health > 0.f) {
    FVector SafeShore = DockLocation(bAtIsland)
      + FVector(0.f, bAtIsland ? 800.f : -800.f, 125.f);
    if (GetWorld() && GetWorld()->FindTeleportSpot(Rider, SafeShore, Rider->GetActorRotation()))
     Rider->SetActorLocation(SafeShore, false, nullptr, ETeleportType::TeleportPhysics);
    if (UCharacterMovementComponent* Movement = Rider->GetCharacterMovement())
     Movement->SetMovementMode(MOVE_Walking);
   }
  }
  Passenger.Reset();
  bUnderway = false;
  UpdateVoyageStatus();
 }
 Super::EndPlay(EndPlayReason);
}

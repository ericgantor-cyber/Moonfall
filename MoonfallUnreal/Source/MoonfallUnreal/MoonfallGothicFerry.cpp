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
 // A separate solid walking deck gives CharacterMovement a moving base.
 // The hull, mast and sail remain visual-only interaction geometry.
 Deck = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WalkableDeck"));
 Deck->SetupAttachment(GetRootComponent());
 Deck->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
 Deck->SetCollisionResponseToAllChannels(ECR_Ignore);
 Deck->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
 Deck->SetGenerateOverlapEvents(false);
 Deck->SetCanEverAffectNavigation(false);
 Gangplank = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WalkOnGangplank"));
 Gangplank->SetupAttachment(GetRootComponent());
 Gangplank->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
 Gangplank->SetCollisionResponseToAllChannels(ECR_Ignore);
 Gangplank->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
 Gangplank->SetCanEverAffectNavigation(false);
 Gangplank->SetGenerateOverlapEvents(false);

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
  Deck->SetStaticMesh(Cube.Object);
  Gangplank->SetStaticMesh(Cube.Object);
  Mast->SetStaticMesh(Cube.Object);
  Sail->SetStaticMesh(Cube.Object);
 }
 // Placeholder gothic longboat silhouette: 7m beam, 12m keel,
 // 4m mast and a triangular-looking tall black sail proxy.
 Hull->SetRelativeLocation(FVector(0.f, 0.f, 6.f));
 Hull->SetRelativeScale3D(FVector(7.f, 12.f, 0.5f));
 // A 7m x 12m moving platform with its top at approximately Z=95cm.
 Deck->SetRelativeLocation(FVector(0.f, 0.f, 82.5f));
 Deck->SetRelativeScale3D(FVector(7.f, 12.f, 0.25f));
 // Dock-to-deck bridge: 5m long, 2.5m wide, shallow slope to Z=95cm.
 // It is only deployed while moored and faces the current shore.
 Gangplank->SetRelativeScale3D(FVector(2.5f, 5.f, 0.2f));
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
 SetGangplankDeployed(true);
}

void AMoonfallGothicFerry::SetGangplankDeployed(bool bDeployed)
{
 if (!Gangplank) return;
 if (bDeployed) {
  const float Direction = bAtIsland ? -1.f : 1.f;
  Gangplank->SetRelativeLocation(FVector(0.f, Direction * 650.f, 42.f));
  Gangplank->SetRelativeRotation(FRotator(0.f, 0.f, -Direction * 11.f));
 }
 Gangplank->SetHiddenInGame(!bDeployed);
 Gangplank->SetCollisionEnabled(bDeployed ? ECollisionEnabled::QueryAndPhysics
                                        : ECollisionEnabled::NoCollision);
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

 if (!IsValid(Deck)) return false;
 // Characters who walked onto the deck can depart from their current spot.
 // Players still standing on the pier use the existing clear-space placement.
 const bool bAlreadyOnDeck = Hero->GetMovementBase() == Deck;
 if (!bAlreadyOnDeck) {
  FVector BoardingLocation = GetActorLocation() + FVector(0.f, 0.f, 190.f);
  if (!GetWorld()->FindTeleportSpot(Hero, BoardingLocation, Hero->GetActorRotation()))
   return false;
  if (!Hero->SetActorLocation(BoardingLocation, false, nullptr, ETeleportType::TeleportPhysics))
   return false;
  Movement->StopMovementImmediately();
  Movement->SetMovementMode(MOVE_Walking);
  Hero->SetBase(Deck);
 }
 // Walking characters retain their location and movement on the moving deck.
 Passenger = Hero;
 CrossingStartedAt = GetWorld()->GetTimeSeconds();
 bUnderway = true;
 SetGangplankDeployed(false);
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
  // A walking rider uses the ferry deck as the moving base until landing.
  // Preserve that base when placement fails and retry next timer update.
  if (!Rider->SetActorLocation(Landing, false, nullptr, ETeleportType::TeleportPhysics)) {
   SetActorLocation(DockLocation(bDestinationIsland));
   return;
  }
  Rider->SetBase(nullptr);
  if (UCharacterMovementComponent* Movement = Rider->GetCharacterMovement())
   Movement->SetMovementMode(MOVE_Walking);
 } else if (IsValid(Rider)) {
  // The death pipeline owns movement for an incapacitated hero.
  Rider->SetBase(nullptr);
 }
 GetWorldTimerManager().ClearTimer(CrossingTimer);
 if (bArrived) bAtIsland = !bAtIsland;
 SetActorLocation(DockLocation(bAtIsland));
 bUnderway = false;
 Passenger.Reset();
 SetGangplankDeployed(true);
}

void AMoonfallGothicFerry::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
 // Shutdown must never depend on a clear landing. FinishCrossing(false)
 // deliberately retries blocked shores during normal gameplay, which would
 // leave a passenger attached when this actor is actually being destroyed.
 if (GetWorld()) GetWorldTimerManager().ClearTimer(CrossingTimer);
 if (bUnderway) {
  if (AMoonfallHeroPawn* Rider = Passenger.Get(); IsValid(Rider)) {
   Rider->SetBase(nullptr);
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
 }
 SetGangplankDeployed(false);
 Super::EndPlay(EndPlayReason);
}

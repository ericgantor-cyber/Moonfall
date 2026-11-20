#include "MoonfallNorthFerryGate.h"
#include "MoonfallNorthSeaPassage.h"
#include "MoonfallNorthAutoSail.h"
#include "MoonfallNorthGothicFerryActor.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "MoonfallHeroPawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"

AMoonfallNorthFerryGate::AMoonfallNorthFerryGate() {
 PrimaryActorTick.bCanEverTick = false;
 BoardingVolume = CreateDefaultSubobject<USphereComponent>(TEXT("BoardingVolume"));
 SetRootComponent(BoardingVolume);
 BoardingVolume->InitSphereRadius(190.f);
 BoardingVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
 BoardingVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
 BoardingVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
 BoardingVolume->SetGenerateOverlapEvents(true);
 BoardingVolume->OnComponentBeginOverlap.AddDynamic(
     this, &AMoonfallNorthFerryGate::OnBoardingOverlap);

 DeckMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FerryDeckMarker"));
 DeckMarker->SetupAttachment(BoardingVolume);
 DeckMarker->SetRelativeScale3D(FVector(3.f, 2.f, 0.2f));
 DeckMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 DeckMarker->SetGenerateOverlapEvents(false);
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(
     TEXT("/Engine/BasicShapes/Cube.Cube"));
 if (Cube.Succeeded()) DeckMarker->SetStaticMesh(Cube.Object);

 ArrivalMarker = CreateDefaultSubobject<USceneComponent>(TEXT("SafeDisembarkAnchor"));
 ArrivalMarker->SetupAttachment(BoardingVolume);
 // Keep landing outside the boarding sphere. Designers must put it on dry
 // collision-tested ground, not out over open ocean.
 ArrivalMarker->SetRelativeLocation(FVector(650.f, 0.f, 130.f));

 FerrySign = CreateDefaultSubobject<UTextRenderComponent>(TEXT("FerryBoardingSign"));
 FerrySign->SetupAttachment(BoardingVolume);
 FerrySign->SetRelativeLocation(FVector(0.f, 0.f, 180.f));
 FerrySign->SetWorldSize(45.f);
 FerrySign->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
 FerrySign->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 FerrySign->SetGenerateOverlapEvents(false);
 FerrySign->SetTextRenderColor(FColor(235, 240, 255));
}

void AMoonfallNorthFerryGate::OnConstruction(const FTransform& Transform) {
 Super::OnConstruction(Transform);
 if (FerrySign) {
  const FString Direction = bIslandHarbor
      ? TEXT("RETURN FERRY TO MAINLAND")
      : TEXT("FERRY TO SHIVERWAKE COAST");
  FerrySign->SetText(FText::FromString(Direction
      + (bEnableSeaPassage ? TEXT("\nAUTOMATIC FERRY BOARDING")
                           : TEXT("\nNOT YET AVAILABLE"))));
 }
}

void AMoonfallNorthFerryGate::EndPlay(const EEndPlayReason::Type EndPlayReason) {
 StopAutomaticVoyage();
 Super::EndPlay(EndPlayReason);
}

void AMoonfallNorthFerryGate::StopAutomaticVoyage(bool bArrived) {
 if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(AutomaticVoyageTimer);
 AMoonfallHeroPawn* Hero = PendingVoyager.Get();
 AMoonfallNorthGothicFerryActor* Ship = PendingShip.Get();

 // Never leave a character based on a ship that is about to be reset.
 if (bVoyagerOnDeck && IsValid(Hero) && IsValid(Ship)
     && Hero->GetMovementBase() == Ship->PassengerDeck) {
  Hero->SetBase(nullptr);
 }
 bool bRescued = bArrived;
 if (!bArrived && bVoyagerOnDeck && IsValid(Hero)
     && !OriginalBoardingLocation.ContainsNaN()
     && OriginalBoardingLocation.SizeSquared() < 1.0e14f) {
  // Failed landing or canceled voyage sends the traveler to the original
  // collision-validated harbor spot. No save slot, death or reward changes.
  bRescued = Hero->TeleportTo(OriginalBoardingLocation,
      OriginalBoardingRotation, false, false);
  if (!bRescued && IsValid(ArrivalMarker)) {
   const FVector Fallback = ArrivalMarker->GetComponentLocation();
   if (!Fallback.ContainsNaN() && Fallback.SizeSquared() < 1.0e14f)
    bRescued = Hero->TeleportTo(Fallback, GetActorRotation(), false, false);
  }
 }

 // Restore only a camera this voyage still owns. Another camera takeover
 // must never be overridden, including on canceled/invalid crossings.
 if (bCinematicViewActive) {
  if (APlayerController* PC = VoyagePlayerController.Get()) {
   if (PC->GetViewTarget() == Ship) {
    AActor* ReturnTarget = PreviousPlayerViewTarget.Get();
    if (!IsValid(ReturnTarget) || ReturnTarget == Ship) ReturnTarget = Hero;
    if (IsValid(ReturnTarget)) {
     PC->SetViewTargetWithBlend(ReturnTarget,
         Hero && Hero->bReduceMotion ? 0.f : 0.3f);
    }
   }
  }
 }
 bCinematicViewActive = false;
 VoyagePlayerController.Reset();
 PreviousPlayerViewTarget.Reset();

 if (IsValid(Ship)) {
  Ship->ResetCrimsonSails();
  Ship->ResetPassengerTravelStatus();
  Ship->SetActorTransform(OriginalShipTransform, false, nullptr,
      ETeleportType::TeleportPhysics);
  // If both dry dock teleports failed, keep the player on the restored ship
  // rather than leaving them floating where the vessel just disappeared.
  if (!bRescued && bVoyagerOnDeck && IsValid(Hero)
      && IsValid(Ship->PassengerDeck)
      && IsValid(Ship->PassengerBoardingMarker)) {
   if (Hero->TeleportTo(Ship->PassengerBoardingMarker->GetComponentLocation(),
           Ship->GetActorRotation(), false, false)) {
    Hero->SetBase(Ship->PassengerDeck);
   } else {
    UE_LOG(LogTemp, Error, TEXT("Northern ferry could not restore passenger to any safe position."));
   }
  }
 }
 PendingShip.Reset();
 PendingVoyager.Reset();
 PendingArrivalGate.Reset();
 bVoyagerOnDeck = false;
 bAutoVoyageInProgress = false;
 VoyageSecondsRemaining = 0;
 VoyagePhaseIndex = 0;
 if (FerrySign) {
  FerrySign->SetText(FText::FromString(
   bIslandHarbor ? TEXT("RETURN FERRY TO MAINLAND\nAUTOMATIC BOARDING")
                 : TEXT("FERRY TO SHIVERWAKE COAST\nAUTOMATIC BOARDING")));
 }
}

bool AMoonfallNorthFerryGate::IsVoyagerAboard(
    const AMoonfallHeroPawn* Hero,
    const AMoonfallNorthGothicFerryActor* Ship) const {
 if (!bVoyagerOnDeck || !IsValid(Hero) || !IsValid(Ship)
     || !IsValid(Ship->PassengerDeck)) return false;
 // Detect a silently lost movement base while the hero reports grounded.
 // A jumping/falling hero is allowed briefly inside the protected rail space,
 // but a grounded hero on anything other than this deck fails closed.
 const UCharacterMovementComponent* Movement = Hero->GetCharacterMovement();
 if (!IsValid(Movement)
     || (Movement->IsMovingOnGround()
         && Hero->GetMovementBase() != Ship->PassengerDeck)) return false;
 const FVector Local = Ship->GetActorTransform().InverseTransformPosition(
     Hero->GetActorLocation());
 // An actual walkable deck: the hero can explore while the boat sails.
 // Straying outside the guarded rail footprint triggers safe cancellation.
 return !Local.ContainsNaN() && FMath::Abs(Local.X) <= 770.f
     && FMath::Abs(Local.Y) <= 270.f
     && Local.Z >= 260.f && Local.Z <= 800.f;
}

void AMoonfallNorthFerryGate::UpdateVoyagePresentation(
    AMoonfallHeroPawn* Hero, float ElapsedSeconds) {
 if (!bAutoVoyageInProgress || !IsValid(Hero)) return;
 const MoonfallNorth::EAutoSailPhase Phase = MoonfallNorth::AutoSailPhaseAt(
     ElapsedSeconds, ActiveVoyageDurationSeconds);
 // Arrival is not a presentation cue until the hero actually lands.
 if (Phase == MoonfallNorth::EAutoSailPhase::Invalid
     || Phase == MoonfallNorth::EAutoSailPhase::Arrived) return;
 const int32 NextPhase = static_cast<int32>(Phase);
 const int32 NextRemaining = MoonfallNorth::AutoSailSecondsRemaining(
     ElapsedSeconds, ActiveVoyageDurationSeconds);
 if (NextPhase == VoyagePhaseIndex && NextRemaining == VoyageSecondsRemaining)
  return; // Do not rebuild text 10 times a second.
 VoyagePhaseIndex = NextPhase;
 VoyageSecondsRemaining = NextRemaining;
 const TCHAR* Direction = bIslandHarbor
     ? TEXT("SAILING TO MAINLAND") : TEXT("SAILING TO SHIVERWAKE COAST");
 const TCHAR* PhaseName = Phase == MoonfallNorth::EAutoSailPhase::Departing
     ? TEXT("DEPARTING HARBOR")
     : Phase == MoonfallNorth::EAutoSailPhase::OpenSea
     ? TEXT("CROSSING THE DARK SEA") : TEXT("APPROACHING HARBOR");
 const FString VoyageMessage = FString::Printf(
     TEXT("%s\n%s\nARRIVAL IN %dS"), Direction, PhaseName, NextRemaining);
 if (FerrySign) FerrySign->SetText(FText::FromString(VoyageMessage));
 // Dock signs are no longer visible once the character boards: keep the
 // exact same stage and countdown on the moving Gothic Raven passenger deck.
 if (AMoonfallNorthGothicFerryActor* Ship = PendingShip.Get())
  Ship->SetPassengerTravelStatus(VoyageMessage);
 OnAutomaticVoyageProgress(Hero, NextRemaining, NextPhase);
}

bool AMoonfallNorthFerryGate::BoardFerry(AMoonfallHeroPawn* Hero) {
 // The boat sails itself. There is no helm, player acceleration, steering
 // or enemy encounter code in this finite-duration boarding gate.
 if (bAutoVoyageInProgress || !IsValid(Hero) || !IsValid(PairedGate)
     || PairedGate == this || !IsValid(SailingVessel)
     || !IsValid(PairedGate->SailingVessel) || SailingVessel == PairedGate->SailingVessel
     || !GetWorld() || Hero->GetWorld() != GetWorld()
     || GetWorld() != PairedGate->GetWorld()
     || SailingVessel->GetWorld() != GetWorld()
     || PairedGate->SailingVessel->GetWorld() != GetWorld()
     || !Hero->IsPlayerControlled() || PairedGate->bAutoVoyageInProgress)
  return false;
 const bool bMarkerReady = IsValid(PairedGate->ArrivalMarker);
 const FVector Destination = bMarkerReady
     ? PairedGate->ArrivalMarker->GetComponentLocation() : FVector::ZeroVector;
 const bool bLandingUsable = bMarkerReady && !Destination.ContainsNaN()
     && Destination.SizeSquared() < 1.0e14f;
 const MoonfallNorth::ESeaPort Origin = bIslandHarbor
     ? MoonfallNorth::ESeaPort::Shiverwake : MoonfallNorth::ESeaPort::Mainland;
 const MoonfallNorth::ESeaPort Finish = PairedGate->bIslandHarbor
     ? MoonfallNorth::ESeaPort::Shiverwake : MoonfallNorth::ESeaPort::Mainland;
 const bool bMenuOpen = Hero->bOptionsOpen || Hero->bInventoryOpen
     || Hero->bAtlasOpen || Hero->bCharacterStatsOpen || Hero->bResidentContextOpen;
 if (!MoonfallNorth::MayBoard(Origin, Finish,
         bEnableSeaPassage && PairedGate->bEnableSeaPassage,
         PairedGate->PairedGate == this,
         FMath::IsFinite(Hero->Health) && Hero->Health > 0.f,
         bMenuOpen, bLandingUsable))
  return false;
 if (Hero->GetActorLocation().ContainsNaN()
     || FVector::DistSquared(Hero->GetActorLocation(), GetActorLocation()) > FMath::Square(400.f)
     || !IsValid(SailingVessel->PassengerDeck)
     || !IsValid(SailingVessel->PassengerBoardingMarker)
     || !IsValid(Hero->GetCapsuleComponent()))
  return false;
 const FVector From = SailingVessel->GetActorLocation();
 const FVector To = PairedGate->SailingVessel->GetActorLocation();
 if (From.ContainsNaN() || To.ContainsNaN()
     || From.SizeSquared() >= 1.0e14f || To.SizeSquared() >= 1.0e14f
     || FVector::DistSquared(From, To) < FMath::Square(1000.f))
  return false;
 // Start the eleven-second trip by *actually boarding* the vessel.
 // CharacterMovement's moving-platform base carries the hero as the boat
 // travels, while their normal movement controls remain available on deck.
 const FVector DockLocation = Hero->GetActorLocation();
 const FRotator DockRotation = Hero->GetActorRotation();
 FVector DeckSpawn = SailingVessel->PassengerBoardingMarker->GetComponentLocation();
 // The static preview marker is intentionally approximate. Place the actual
 // capsule 2 cm above the walkable floor, honoring the hero's real capsule
 // scale instead of relying on the same height for every character.
 const float DeckSurfaceZ = SailingVessel->PassengerDeck->GetComponentLocation().Z
     + SailingVessel->PassengerDeck->GetScaledBoxExtent().Z;
 DeckSpawn.Z = DeckSurfaceZ
     + Hero->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 2.f;
 if (DeckSpawn.ContainsNaN() || DeckSpawn.SizeSquared() >= 1.0e14f
     || !Hero->TeleportTo(DeckSpawn, SailingVessel->GetActorRotation(), false, false))
  return false;
 Hero->SetBase(SailingVessel->PassengerDeck);
 if (Hero->GetMovementBase() != SailingVessel->PassengerDeck) {
  // Failed to create a real UE walking base; abort rather than sending an
  // unparented player over an empty ocean. Prefer the original safe dock.
  if (!Hero->TeleportTo(DockLocation, DockRotation, false, false))
   UE_LOG(LogTemp, Error, TEXT("Ferry boarding rejected: cannot return hero to original dock."));
  return false;
 }
 OriginalBoardingLocation = DockLocation;
 OriginalBoardingRotation = DockRotation;
 bVoyagerOnDeck = true;
 const float Duration = MoonfallNorth::ValidAutoSailDuration(VoyageDurationSeconds)
     ? VoyageDurationSeconds : MoonfallNorth::DefaultAutoSailSeconds;
 ActiveVoyageDurationSeconds = Duration;
 SailingFrom = From;
 SailingTo = To;
 OriginalShipTransform = SailingVessel->GetActorTransform();
 PendingShip = SailingVessel;
 PendingArrivalGate = PairedGate;
 PendingVoyager = Hero;
 VoyageStartWorldSeconds = GetWorld()->GetTimeSeconds();
 bAutoVoyageInProgress = true;
 VoyageSecondsRemaining = -1; // Force a readable 11S departure cue.
 VoyagePhaseIndex = 0;
 if (FerrySign) FerrySign->SetText(FText::FromString(
  bIslandHarbor ? TEXT("SAILING TO MAINLAND") : TEXT("SAILING TO SHIVERWAKE COAST")));
 GetWorld()->GetTimerManager().SetTimer(AutomaticVoyageTimer, this,
     &AMoonfallNorthFerryGate::UpdateAutomaticVoyage,
     MoonfallNorth::AutoSailUpdateSeconds, true);
 // Normal third-person hero camera is the default so players can walk on
 // deck. Designers may opt into the ship-wide cinematic camera instead.
 if (bUseVoyageCamera && IsValid(SailingVessel->VoyageCamera)) {
  if (APlayerController* PC = Cast<APlayerController>(Hero->GetController())) {
   if (PC->IsLocalController()) {
    AActor* Before = PC->GetViewTarget();
    if (IsValid(Before) && Before != SailingVessel) {
     VoyagePlayerController = PC;
     PreviousPlayerViewTarget = Before;
     bCinematicViewActive = true;
     PC->SetViewTargetWithBlend(SailingVessel,
         Hero->bReduceMotion ? 0.f : 0.35f);
    }
   }
  }
 }
 OnAutomaticVoyageStarted(Hero);
 UpdateVoyagePresentation(Hero, 0.f);
 return true; // Boarding has begun, NOT proof that arrival is complete.
}

void AMoonfallNorthFerryGate::UpdateAutomaticVoyage() {
 AMoonfallHeroPawn* Hero = PendingVoyager.Get();
 AMoonfallNorthFerryGate* Arrival = PendingArrivalGate.Get();
 AMoonfallNorthGothicFerryActor* Ship = PendingShip.Get();
 if (!bAutoVoyageInProgress || !GetWorld() || !IsValid(Hero)
     || !IsValid(Arrival) || !IsValid(Ship)
     || Arrival->PairedGate != this || !bEnableSeaPassage
     || !Arrival->bEnableSeaPassage || !Hero->IsPlayerControlled()
     || !FMath::IsFinite(Hero->Health) || Hero->Health <= 0.f
     || Hero->bOptionsOpen || Hero->bInventoryOpen || Hero->bAtlasOpen
     || Hero->bCharacterStatsOpen || Hero->bResidentContextOpen
     || Hero->GetWorld() != GetWorld() || Ship->GetWorld() != GetWorld()
     || !IsVoyagerAboard(Hero, Ship)) {
  StopAutomaticVoyage();
  return;
 }
 const float ElapsedSeconds = GetWorld()->GetTimeSeconds() - VoyageStartWorldSeconds;
 if (!FMath::IsFinite(ElapsedSeconds) || ElapsedSeconds < 0.f) {
  StopAutomaticVoyage();
  return;
 }
 const float Progress = MoonfallNorth::AutoSailEasedProgress(
     ElapsedSeconds, ActiveVoyageDurationSeconds);
 FVector Position = FMath::Lerp(SailingFrom, SailingTo, Progress);
 if (!Hero->bReduceMotion) {
  Position.Z += 16.f * FMath::Sin(ElapsedSeconds * 2.3f)
      * FMath::Sin(Progress * PI); // cosmetic only; disabled for reduced motion
 }
 Ship->SetActorLocation(Position, false, nullptr, ETeleportType::TeleportPhysics);
 Ship->AnimateCrimsonSails(ElapsedSeconds, Hero->bReduceMotion);
 UpdateVoyagePresentation(Hero, ElapsedSeconds);

 if (!MoonfallNorth::AutoSailShouldArrive(ElapsedSeconds, ActiveVoyageDurationSeconds))
  return;
 const FVector Destination = IsValid(Arrival->ArrivalMarker)
     ? Arrival->ArrivalMarker->GetComponentLocation() : FVector::ZeroVector;
 const bool bDestinationSafe = IsValid(Arrival->ArrivalMarker)
     && !Destination.ContainsNaN() && Destination.SizeSquared() < 1.0e14f;
 // The arriving hero only teleports after the voyage timer. Unreal checks
 // collision without forcing placement; a failed landing grants no progress.
 // Remove the moving-platform base before the dry-land teleport. On failure,
 // immediately rebase to the boat so cancellation can restore the traveler.
 if (Hero->GetMovementBase() == Ship->PassengerDeck) Hero->SetBase(nullptr);
 const bool bArrived = bDestinationSafe
     && Hero->TeleportTo(Destination, Arrival->GetActorRotation(), false, false);
 if (!bArrived && IsValid(Ship->PassengerDeck))
  Hero->SetBase(Ship->PassengerDeck);
 const MoonfallNorth::ESeaPort Origin = bIslandHarbor
     ? MoonfallNorth::ESeaPort::Shiverwake : MoonfallNorth::ESeaPort::Mainland;
 const MoonfallNorth::ESeaPort Finish = Arrival->bIslandHarbor
     ? MoonfallNorth::ESeaPort::Shiverwake : MoonfallNorth::ESeaPort::Mainland;
 const bool bFirst = bArrived && MoonfallNorth::IsFirstIslandLanding(
     Origin, Finish, bReachedIslandThisSession);
 if (bFirst) bReachedIslandThisSession = true;
 StopAutomaticVoyage(bArrived);
 if (bArrived) OnFerryArrival(Hero, bFirst);
}

void AMoonfallNorthFerryGate::OnBoardingOverlap(
 UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
 UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
 bool bFromSweep, const FHitResult& SweepResult) {
 BoardFerry(Cast<AMoonfallHeroPawn>(OtherActor));
}

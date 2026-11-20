#include "MoonfallNorthFerryGate.h"
#include "MoonfallNorthSeaPassage.h"
#include "MoonfallNorthAutoSail.h"
#include "MoonfallNorthGothicFerryActor.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "MoonfallHeroPawn.h"
#include "Components/SphereComponent.h"
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

void AMoonfallNorthFerryGate::StopAutomaticVoyage() {
 if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(AutomaticVoyageTimer);
 if (AMoonfallNorthGothicFerryActor* Ship = PendingShip.Get()) {
  Ship->SetActorTransform(OriginalShipTransform, false, nullptr, ETeleportType::TeleportPhysics);
 }
 PendingShip.Reset();
 PendingVoyager.Reset();
 PendingArrivalGate.Reset();
 bAutoVoyageInProgress = false;
 VoyageSecondsRemaining = 0;
 VoyagePhaseIndex = 0;
 if (FerrySign) {
  FerrySign->SetText(FText::FromString(
   bIslandHarbor ? TEXT("RETURN FERRY TO MAINLAND\nAUTOMATIC BOARDING")
                 : TEXT("FERRY TO SHIVERWAKE COAST\nAUTOMATIC BOARDING")));
 }
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
 if (FerrySign) FerrySign->SetText(FText::FromString(FString::Printf(
     TEXT("%s\n%s\nARRIVAL IN %dS"), Direction, PhaseName, NextRemaining)));
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
     || FVector::DistSquared(Hero->GetActorLocation(), GetActorLocation()) > FMath::Square(400.f))
  return false;
 const FVector From = SailingVessel->GetActorLocation();
 const FVector To = PairedGate->SailingVessel->GetActorLocation();
 if (From.ContainsNaN() || To.ContainsNaN()
     || From.SizeSquared() >= 1.0e14f || To.SizeSquared() >= 1.0e14f
     || FVector::DistSquared(From, To) < FMath::Square(1000.f))
  return false;
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
     || Hero->GetActorLocation().ContainsNaN()
     || FVector::DistSquared(Hero->GetActorLocation(), GetActorLocation()) > FMath::Square(400.f)) {
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
 Position.Z += 16.f * FMath::Sin(ElapsedSeconds * 2.3f)
     * FMath::Sin(Progress * PI); // cosmetic wave bob, no gameplay movement
 Ship->SetActorLocation(Position, false, nullptr, ETeleportType::TeleportPhysics);
 UpdateVoyagePresentation(Hero, ElapsedSeconds);

 if (!MoonfallNorth::AutoSailShouldArrive(ElapsedSeconds, ActiveVoyageDurationSeconds))
  return;
 const FVector Destination = IsValid(Arrival->ArrivalMarker)
     ? Arrival->ArrivalMarker->GetComponentLocation() : FVector::ZeroVector;
 const bool bDestinationSafe = IsValid(Arrival->ArrivalMarker)
     && !Destination.ContainsNaN() && Destination.SizeSquared() < 1.0e14f;
 // The arriving hero only teleports after the voyage timer. Unreal checks
 // collision without forcing placement; a failed landing grants no progress.
 const bool bArrived = bDestinationSafe
     && Hero->TeleportTo(Destination, Arrival->GetActorRotation(), false, false);
 const MoonfallNorth::ESeaPort Origin = bIslandHarbor
     ? MoonfallNorth::ESeaPort::Shiverwake : MoonfallNorth::ESeaPort::Mainland;
 const MoonfallNorth::ESeaPort Finish = Arrival->bIslandHarbor
     ? MoonfallNorth::ESeaPort::Shiverwake : MoonfallNorth::ESeaPort::Mainland;
 const bool bFirst = bArrived && MoonfallNorth::IsFirstIslandLanding(
     Origin, Finish, bReachedIslandThisSession);
 if (bFirst) bReachedIslandThisSession = true;
 StopAutomaticVoyage();
 if (bArrived) OnFerryArrival(Hero, bFirst);
}

void AMoonfallNorthFerryGate::OnBoardingOverlap(
 UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
 UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
 bool bFromSweep, const FHitResult& SweepResult) {
 BoardFerry(Cast<AMoonfallHeroPawn>(OtherActor));
}

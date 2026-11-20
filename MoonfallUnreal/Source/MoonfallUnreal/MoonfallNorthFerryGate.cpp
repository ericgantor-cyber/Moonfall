#include "MoonfallNorthFerryGate.h"
#include "MoonfallNorthSeaPassage.h"
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
      + (bEnableSeaPassage ? TEXT("\nWALK ONTO DECK TO BOARD")
                           : TEXT("\nNOT YET AVAILABLE"))));
 }
}

bool AMoonfallNorthFerryGate::BoardFerry(AMoonfallHeroPawn* Hero) {
 if (!IsValid(Hero) || !IsValid(PairedGate) || PairedGate == this
     || GetWorld() != PairedGate->GetWorld()
     || !Hero->IsPlayerControlled()) return false;
 const FVector Destination = PairedGate->ArrivalMarker
     ? PairedGate->ArrivalMarker->GetComponentLocation() : FVector(NAN);
 const bool bLandingUsable = !Destination.ContainsNaN()
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
         bMenuOpen, bLandingUsable)) return false;
 // Unreal's non-test teleport checks for valid destination placement.
 // If blocked, do not unlock or emit a successful arrival event.
 if (!Hero->TeleportTo(Destination, PairedGate->GetActorRotation(), false, false))
  return false;
 const bool bFirst = MoonfallNorth::IsFirstIslandLanding(
     Origin, Finish, bReachedIslandThisSession);
 if (bFirst) bReachedIslandThisSession = true;
 OnFerryArrival(Hero, bFirst);
 return true;
}

void AMoonfallNorthFerryGate::OnBoardingOverlap(
 UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
 UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
 bool bFromSweep, const FHitResult& SweepResult) {
 BoardFerry(Cast<AMoonfallHeroPawn>(OtherActor));
}

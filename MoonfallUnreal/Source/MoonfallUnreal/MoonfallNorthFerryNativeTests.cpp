// Native Unreal Editor-only component and timing smoke tests.
// Run explicitly inside a compiled UE5 Editor. The portable GitHub CI
// checks source but does NOT execute this native automation test.
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "MoonfallNorthGothicFerryActor.h"
#include "MoonfallNorthFerryGate.h"
#include "MoonfallNorthAutoSail.h"
#include "Components/BoxComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "UObject/UObjectGlobals.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMoonfallNorthFerryComponentContract,
    "Moonfall.NorthernExpanse.Ferry.ComponentContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMoonfallNorthFerryComponentContract::RunTest(const FString& Parameters) {
 const AMoonfallNorthGothicFerryActor* Ship =
     GetDefault<AMoonfallNorthGothicFerryActor>();
 const AMoonfallNorthFerryGate* Gate = GetDefault<AMoonfallNorthFerryGate>();
 if (!TestNotNull(TEXT("Gothic Raven class default"), Ship)
     || !TestNotNull(TEXT("Ferry gate class default"), Gate))
  return false;

 // All render geometry stays decorative, but the movable walking deck and
 // four safety rails must block the player capsule. No ship steering.
 const UBoxComponent* Deck = Ship->PassengerDeck.Get();
 const USceneComponent* Boarding = Ship->PassengerBoardingMarker.Get();
 if (!TestNotNull(TEXT("Passenger walking deck"), Deck)
     || !TestNotNull(TEXT("Passenger boarding marker"), Boarding))
  return false;
 TestTrue(TEXT("Movable ship root"), Ship->GetRootComponent()->Mobility == EComponentMobility::Movable);
 TestTrue(TEXT("Movable walking base"), Deck->Mobility == EComponentMobility::Movable);
 TestTrue(TEXT("Deck permits step-up"), Deck->CanCharacterStepUpOn == ECB_Yes);
 TestTrue(TEXT("Deck blocks Pawn"), Deck->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block);
 TestTrue(TEXT("Deck has query and physics collision"),
          Deck->GetCollisionEnabled() == ECollisionEnabled::QueryAndPhysics);
 TestTrue(TEXT("Deck floor fits authored hull"),
          Deck->GetUnscaledBoxExtent().Equals(FVector(720.f,235.f,23.f), 0.1f));
 TestTrue(TEXT("Boarding point above walking floor"),
          Boarding->GetRelativeLocation().Z > Deck->GetRelativeLocation().Z
              + Deck->GetUnscaledBoxExtent().Z + 90.f);
 TestTrue(TEXT("Boarding point remains within safe rail footprint"),
          FMath::Abs(Boarding->GetRelativeLocation().X) < 720.f
              && FMath::Abs(Boarding->GetRelativeLocation().Y) < 235.f);
 const UTextRenderComponent* Sign = Ship->PassengerTravelSign.Get();
 if (!TestNotNull(TEXT("Moving ship passenger progress sign"), Sign)) return false;
 TestTrue(TEXT("Onboard sign cannot block passenger"),
          Sign->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
 TestTrue(TEXT("Onboard sign follows ferry root"), Sign->GetAttachParent() == Ship->GetRootComponent());

 const UBoxComponent* Rails[] = {
     Ship->PortPassengerRail.Get(), Ship->StarboardPassengerRail.Get(),
     Ship->BowPassengerRail.Get(), Ship->SternPassengerRail.Get()
 };
 for (int32 Index=0; Index<4; ++Index) {
  if (!TestNotNull(TEXT("Passenger rail"), Rails[Index])) return false;
  TestTrue(TEXT("Rail blocks Pawn"), Rails[Index]->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block);
  TestTrue(TEXT("Rail moves with ship"), Rails[Index]->Mobility == EComponentMobility::Movable);
 }
 const UInstancedStaticMeshComponent* RenderComponents[] = {
     Ship->HullPieces.Get(), Ship->GothicIronwork.Get(),
     Ship->CrimsonSails.Get(), Ship->RavenFigurehead.Get(),
     Ship->CrimsonLanterns.Get()
 };
 for (int32 Index=0; Index<5; ++Index) {
  if (!TestNotNull(TEXT("Decorative mesh"), RenderComponents[Index])) return false;
  TestTrue(TEXT("Render mesh does not block moving passenger"),
           RenderComponents[Index]->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
 }
 TestFalse(TEXT("Ship has no expensive per-frame tick"), Ship->PrimaryActorTick.bCanEverTick);
 TestFalse(TEXT("Gate has no expensive per-frame tick"), Gate->PrimaryActorTick.bCanEverTick);
 TestFalse(TEXT("Ferry remains opt-in by default"), Gate->bEnableSeaPassage);
 TestFalse(TEXT("Third-person camera is default for deck passengers"), Gate->bUseVoyageCamera);
 TestTrue(TEXT("Route is exactly eleven seconds"), FMath::IsNearlyEqual(Gate->VoyageDurationSeconds, 11.f));
 TestTrue(TEXT("Eleven-second voyage starts at eleven"), MoonfallNorth::AutoSailSecondsRemaining(0.f, 11.f) == 11);
 TestTrue(TEXT("Voyage cannot finish early"), !MoonfallNorth::AutoSailShouldArrive(10.99f, 11.f));
 TestTrue(TEXT("Voyage finishes at eleven seconds"), MoonfallNorth::AutoSailShouldArrive(11.f, 11.f));
 return true;
}
#endif // WITH_DEV_AUTOMATION_TESTS

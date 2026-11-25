"""Verify the original Gothic Raven ferry remains an isolated visual prototype.

Portable source policy only; not a UE5 compile, material render or navigation test.
"""
import ast
from pathlib import Path

root = Path(__file__).resolve().parents[1]
ue = root / "MoonfallUnreal"
module = ue / "Source" / "MoonfallUnreal"
header = (module / "MoonfallNorthGothicFerryActor.h").read_text(encoding="utf-8")
cpp = (module / "MoonfallNorthGothicFerryActor.cpp").read_text(encoding="utf-8")
builder = (ue / "Tools" / "create_northern_expanse_preview.py").read_text(encoding="utf-8")
validator = (ue / "Tools" / "validate_northern_expanse_preview.py").read_text(encoding="utf-8")
ast.parse(builder)
ast.parse(validator)
for required in (
    'PrimaryActorTick.bCanEverTick = false',
    'PassengerDeck = CreateDefaultSubobject<UBoxComponent>',
    'GetRootComponent()->SetMobility(EComponentMobility::Movable)',
    'PassengerDeck->SetMobility(EComponentMobility::Movable)',
    'PassengerDeck->CanCharacterStepUpOn = ECB_Yes',
    'Rail->SetMobility(EComponentMobility::Movable)',
    'PassengerTravelSign = CreateDefaultSubobject<UTextRenderComponent>',
    'PassengerTravelSign->SetCollisionEnabled(ECollisionEnabled::NoCollision)',
    'PassengerTravelSign->SetText(',
    'AMoonfallNorthGothicFerryActor::SetPassengerTravelStatus',
    'AMoonfallNorthGothicFerryActor::ResetPassengerTravelStatus',
    'AMoonfallNorthGothicFerryActor::AnimateCrimsonLanterns',
    'AMoonfallNorthGothicFerryActor::ResetCrimsonLanterns',
    'PortLanternLight->SetIntensity(210.f + 30.f * FMath::Sin(',
    'StarboardLanternLight->SetIntensity(210.f + 30.f * FMath::Sin(',
    'bReduceMotion || !FMath::IsFinite(ElapsedSeconds)',
    'PassengerDeck->SetBoxExtent(FVector(720.f, 235.f, 23.f))',
    'PassengerDeck->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics)',
    'PassengerDeck->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block)',
    'PassengerBoardingMarker = CreateDefaultSubobject<USceneComponent>',
    'PassengerBoardingMarker->SetRelativeLocation(FVector(390.f, 0.f, 390.f))',
    'PortPassengerRail = MakeRail(',
    'StarboardPassengerRail = MakeRail(',
    'BowPassengerRail = MakeRail(',
    'SternPassengerRail = MakeRail(',
    'SetCanEverAffectNavigation(false)',
    'Rail->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block)',
    'bShowGothicFerry',
    'SetCollisionEnabled(ECollisionEnabled::NoCollision)',
    'SetGenerateOverlapEvents(false)',
    'HullPieces = MakePieces',
    'GothicIronwork = MakePieces',
    'CrimsonSails = MakePieces',
    'RavenFigurehead = MakePieces',
    'CrimsonLanterns = MakePieces',
    'AddBox(HullPieces',
    'AddBox(GothicIronwork',
    'AddBox(CrimsonSails',
    'AddBox(RavenFigurehead',
    'AddBox(CrimsonLanterns',
    'SetCastShadows(false)',
    'SetAttenuationRadius(500.f)',
    'ClearInstances()',
    'bFindCameraComponentWhenViewTarget = true',
    'GothicRavenVoyageCamera',
    'VoyageCamera->SetupAttachment(GetRootComponent())',
    'VoyageCamera->SetRelativeLocation(',
    'VoyageCamera->SetRelativeRotation(',
    'VoyageCamera->SetFieldOfView(80.f)',
    'VoyageCamera->bUsePawnControlRotation = false',
    'VoyageCamera->bAutoActivate = true',
    'AMoonfallNorthGothicFerryActor::AnimateCrimsonSails',
    'AMoonfallNorthGothicFerryActor::ResetCrimsonSails',
    'CrimsonSails->UpdateInstanceTransform(',
    'SailRestTransforms[Index]',
    'bSailsWereAnimated = true',
    'if (bReduceMotion)',
    '5.f * FMath::Sin(',
    'CrimsonSails->GetInstanceTransform(',
):
    assert required in cpp, f"Missing gothic ferry guard or detail: {required}"
for required in (
    'UCLASS(Blueprintable)',
    'bShowGothicFerry = true',
    'TObjectPtr<UInstancedStaticMeshComponent> HullPieces',
    'TObjectPtr<UInstancedStaticMeshComponent> RavenFigurehead',
    'TObjectPtr<UCameraComponent> VoyageCamera',
    'TObjectPtr<UBoxComponent> PassengerDeck',
    'TObjectPtr<UTextRenderComponent> PassengerTravelSign',
    'void SetPassengerTravelStatus(const FString& Status)',
    'void ResetPassengerTravelStatus()',
    'void AnimateCrimsonLanterns(float ElapsedSeconds, bool bReduceMotion)',
    'void ResetCrimsonLanterns()',
    'TObjectPtr<USceneComponent> PassengerBoardingMarker',
    'TObjectPtr<UBoxComponent> PortPassengerRail',
    'TObjectPtr<UBoxComponent> StarboardPassengerRail',
    'TObjectPtr<UBoxComponent> BowPassengerRail',
    'TObjectPtr<UBoxComponent> SternPassengerRail',
    'void AnimateCrimsonSails(float ElapsedSeconds, bool bReduceMotion)',
    'void ResetCrimsonSails()',
    'TArray<FTransform> SailRestTransforms',
):
    assert required in header, f"Missing Gothic ferry component: {required}"
for required in (
    'required_class("MoonfallNorthGothicFerryActor")',
    '"Mainland Gothic Raven Ferry"',
    '"Shiverwake Gothic Raven Ferry"',
    '(0.0, -7650.0, -40.0)',
    '(0.0, -2750.0, -40.0)',
    'unreal.Rotator(0.0, 90.0, 0.0)',
    'unreal.Rotator(0.0, -90.0, 0.0)',
):
    assert required in builder, f"Missing ferry editor placement: {required}"
for required in (
    'len(ships) == 2',
    'mesh.get_instance_count() >= 2',
    'unreal.CollisionEnabled.NO_COLLISION',
    'b_show_gothic_ferry',
):
    assert required in validator, f"Missing ferry preview validation: {required}"
for prohibited in ('SaveGameToSlot(', 'LoadGameFromSlot(', 'TeleportTo(',
                   'AMoonfallArena', 'MoonfallHeroPawn.h', 'SpawnActor'):
    assert prohibited not in cpp and prohibited not in header, (
        f"Decorative vessel must not control saves, heroes or boarding: {prohibited}"
    )
print("Gothic raven ferry source and preview placement policy passed")

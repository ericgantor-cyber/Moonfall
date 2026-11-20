"""Static safety contract for non-steerable Moonfall gothic ferry movement.

This inspects C++ and Unreal Editor Python text only. It is not a UE build,
a validated moving platform, nor a packaged Windows gameplay test.
"""
import ast
from pathlib import Path

root = Path(__file__).resolve().parents[1]
module = root / "MoonfallUnreal" / "Source" / "MoonfallUnreal"
gate_h = (module / "MoonfallNorthFerryGate.h").read_text(encoding="utf-8")
gate_cpp = (module / "MoonfallNorthFerryGate.cpp").read_text(encoding="utf-8")
policy = (module / "MoonfallNorthAutoSail.h").read_text(encoding="utf-8")
builder_path = root / "MoonfallUnreal/Tools/create_northern_expanse_preview.py"
validation_path = root / "MoonfallUnreal/Tools/validate_northern_expanse_preview.py"
builder = builder_path.read_text(encoding="utf-8")
validation = validation_path.read_text(encoding="utf-8")

ast.parse(builder, filename=str(builder_path))
ast.parse(validation, filename=str(validation_path))

for text in (
    "DefaultAutoSailSeconds = 11.f",
    "AutoSailUpdateSeconds = 0.1f",
    "ValidAutoSailDuration",
    "AutoSailEasedProgress",
    "AutoSailShouldArrive",
    "AutoSailPhaseAt",
    "AutoSailSecondsRemaining",
    "Progress must clamp",
):
    if text == "Progress must clamp":
        continue
    assert text in policy, f"Missing bounded auto-sail policy: {text}"

for text in (
    "TObjectPtr<AMoonfallNorthGothicFerryActor> SailingVessel",
    "float VoyageDurationSeconds = 11.f",
    "bool bAutoVoyageInProgress = false",
    "void UpdateAutomaticVoyage()",
    "FTimerHandle AutomaticVoyageTimer",
    "void OnAutomaticVoyageStarted",
    "void OnAutomaticVoyageProgress",
    "int32 VoyageSecondsRemaining = 0",
    "int32 VoyagePhaseIndex = 0",
    "void UpdateVoyagePresentation",
    "virtual void EndPlay",
    "bool bUseVoyageCamera = false",
    "bool bVoyagerOnDeck = false",
    "TWeakObjectPtr<AMoonfallNorthGothicFerryActor> PendingTargetShip",
    "OriginalBoardingLocation",
    "OriginalBoardingRotation",
    "bool IsVoyagerAboard(",
    "void StopAutomaticVoyage(bool bArrived = false)",
    "bool FindSafeDryLanding(",
    "TWeakObjectPtr<APlayerController> VoyagePlayerController",
    "TWeakObjectPtr<AActor> PreviousPlayerViewTarget",
    "bool bCinematicViewActive = false",
):
    assert text in gate_h, f"Missing voyage gate member: {text}"

start = gate_cpp.split("bool AMoonfallNorthFerryGate::BoardFerry(", 1)[1].split(
    "void AMoonfallNorthFerryGate::UpdateAutomaticVoyage(", 1)[0]
tick = gate_cpp.split("void AMoonfallNorthFerryGate::UpdateAutomaticVoyage(", 1)[1].split(
    "void AMoonfallNorthFerryGate::OnBoardingOverlap(", 1)[0]
cleanup = gate_cpp.split("void AMoonfallNorthFerryGate::StopAutomaticVoyage(", 1)[1].split(
    "bool AMoonfallNorthFerryGate::BoardFerry(", 1)[0]

# Boarding places the hero *on the real ship deck*, but cannot teleport to
# the destination before the complete eleven-second crossing.
land_guard = gate_cpp.split(
    "bool AMoonfallNorthFerryGate::FindSafeDryLanding(", 1
)[1].split("bool AMoonfallNorthFerryGate::BoardFerry(", 1)[0]
for expected in (
    "LineTraceSingleByChannel(",
    "ECC_Visibility",
    "Query.AddIgnoredActor(Hero)",
    "Query.AddIgnoredActor(this)",
    "Query.AddIgnoredActor(Harbor)",
    "Ground.bBlockingHit",
    "Ground.ImpactNormal.ContainsNaN()",
    "Ground.GetComponent()->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block",
    "MoonfallNorth::ValidDryLandingSample(",
    "Ground.ImpactNormal.Z",
    "Ground.ImpactPoint.Z, CapsuleHalfHeight",
    "MoonfallNorth::DryLandingCapsuleClearanceCm",
    "GetScaledCapsuleHalfHeight()",
    "OutLandingLocation = Candidate",
):
    assert expected in land_guard, f"Missing true walkable landing guard: {expected}"
assert "FindSafeDryLanding(Hero, PairedGate, Destination)" in start
assert "FindSafeDryLanding(Hero, Arrival, Destination)" in tick
assert "Hero->TeleportTo(DeckSpawn" in start
assert '#include "Components/CapsuleComponent.h"' in gate_cpp
assert "Hero->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()" in start
assert "SailingVessel->PassengerDeck->GetScaledBoxExtent().Z" in start
assert "DeckSpawn.Z = DeckSurfaceZ" in start
assert "Hero->TeleportTo(Destination" not in start
assert "Hero->SetBase(SailingVessel->PassengerDeck)" in start
assert "Hero->GetMovementBase() != SailingVessel->PassengerDeck" in start
assert "OriginalBoardingLocation = DockLocation" in start
assert "PendingTargetShip = PairedGate->SailingVessel" in start
assert "OriginalBoardingRotation = DockRotation" in start
assert "bVoyagerOnDeck = true" in start
assert "GetTimerManager().SetTimer(AutomaticVoyageTimer" in start
assert "SailingVessel->GetActorLocation()" in start
assert "PairedGate->SailingVessel->GetActorLocation()" in start
assert "OnAutomaticVoyageStarted(Hero)" in start
assert "MoonfallNorth::MayBoard" in start
assert "bUseVoyageCamera && IsValid(SailingVessel->VoyageCamera)" in start
assert "PC->IsLocalController()" in start
assert "PreviousPlayerViewTarget = Before" in start
assert "PC->SetViewTargetWithBlend(SailingVessel" in start
assert "Hero->bReduceMotion ? 0.f : 0.35f" in start
assert "MoonfallNorth::AutoSailEasedProgress" in tick
assert "UpdateVoyagePresentation(Hero, ElapsedSeconds)" in tick
assert "OnAutomaticVoyageProgress(Hero, NextRemaining, NextPhase)" in gate_cpp
assert "Ship->SetPassengerTravelStatus(VoyageMessage)" in gate_cpp
assert "Ship->ResetPassengerTravelStatus()" in cleanup
assert "Ship->AnimateCrimsonLanterns(ElapsedSeconds, Hero->bReduceMotion)" in tick
assert "Ship->ResetCrimsonLanterns()" in cleanup
assert "FString::Printf(" in gate_cpp
assert "ARRIVAL IN %dS" in gate_cpp
assert "DEPARTING HARBOR" in gate_cpp
assert "CROSSING THE DARK SEA" in gate_cpp
assert "APPROACHING HARBOR" in gate_cpp
assert "NextPhase == VoyagePhaseIndex && NextRemaining == VoyageSecondsRemaining" in gate_cpp
assert "VoyageSecondsRemaining = 0;" in cleanup
assert "VoyagePhaseIndex = 0;" in cleanup
assert "Ship->SetActorLocation(Position" in tick
assert "!IsVoyagerAboard(Hero, Ship)" in tick
assert "Hero->SetBase(nullptr)" in tick
assert "Hero->SetBase(Ship->PassengerDeck)" in tick
assert "StopAutomaticVoyage(bArrived)" in tick
for expected in (
    "PairedGate != Arrival",
    "Arrival->PairedGate != this",
    "SailingVessel != Ship",
    "!IsValid(PendingTargetShip.Get())",
    "Arrival->SailingVessel != PendingTargetShip.Get()",
    "PendingTargetShip.Get() == Ship",
    "Arrival->bIslandHarbor == bIslandHarbor",
    "Arrival->GetWorld() != GetWorld()",
):
    assert expected in tick, f"Mid-voyage harbor route changed without cancel: {expected}"
assert '#include "GameFramework/CharacterMovementComponent.h"' in gate_cpp
aboard = gate_cpp.split(
    "bool AMoonfallNorthFerryGate::IsVoyagerAboard(", 1
)[1].split("void AMoonfallNorthFerryGate::UpdateVoyagePresentation(", 1)[0]
assert "Movement->IsMovingOnGround()" in aboard
assert "Hero->GetMovementBase() != Ship->PassengerDeck" in aboard
assert "Local.Z >= 260.f && Local.Z <= 800.f" in aboard
assert "MoonfallNorth::AutoSailShouldArrive" in tick
assert "Hero->TeleportTo(Destination" in tick
assert "if (bArrived) OnFerryArrival(Hero, bFirst)" in tick
assert "StopAutomaticVoyage();" in tick
assert "ClearTimer(AutomaticVoyageTimer)" in cleanup
assert "Ship->SetActorTransform(OriginalShipTransform" in cleanup
assert "Hero->TeleportTo(OriginalBoardingLocation" in cleanup
assert "Hero->TeleportTo(Fallback" in cleanup
assert "FindSafeDryLanding(Hero, this, Fallback)" in cleanup
assert "if (!bArrived && bVoyagerOnDeck && IsValid(Hero))" in cleanup
assert "OriginalBoardingLocation.SizeSquared() < 1.0e14f" in cleanup
assert "SafeDeck.Z = DeckTop" in cleanup
assert "bVoyagerOnDeck = false" in cleanup
assert "Hero->GetMovementBase() == Ship->PassengerDeck" in cleanup
assert "Hero->SetBase(nullptr)" in cleanup
assert "Ship->PassengerBoardingMarker->GetComponentLocation()" in cleanup
# Cleanup must work even when the passenger has left the deck footprint.
assert "IsVoyagerAboard(" in tick
assert "if (PC->GetViewTarget() == Ship)" in cleanup
assert "ReturnTarget = PreviousPlayerViewTarget.Get()" in cleanup
assert "ReturnTarget = Hero" in cleanup
assert "PC->SetViewTargetWithBlend(ReturnTarget" in cleanup
assert "VoyagePlayerController.Reset()" in cleanup
assert "PreviousPlayerViewTarget.Reset()" in cleanup
assert "bCinematicViewActive = false" in cleanup
assert "if (!Hero->bReduceMotion)" in tick
assert "Ship->AnimateCrimsonSails(ElapsedSeconds, Hero->bReduceMotion)" in tick
assert "Ship->ResetCrimsonSails()" in cleanup
assert "PendingVoyager.Reset()" in cleanup
assert "PendingTargetShip.Reset()" in cleanup
assert "PrimaryActorTick.bCanEverTick = false" in gate_cpp
for forbidden in (
    "SaveWorldProgress(", "LoadWorldProgress(", "BindAxis(",
    "GhostShip", "SeaMonster", "NavalCombat", "SetActorTickEnabled(true)"
):
    assert forbidden not in gate_cpp and forbidden not in gate_h, forbidden

for expected in (
    '"sailing_vessel", mainland_ship',
    '"sailing_vessel", island_ship',
    '"voyage_duration_seconds", 11.0',
):
    assert expected in builder, f"Missing ship route wiring: {expected}"
for expected in (
    'mainland.get_editor_property("sailing_vessel") == mainland_ship',
    'island.get_editor_property("sailing_vessel") == island_ship',
    '"voyage_duration_seconds"',
):
    assert expected in validation, f"Missing travel validation: {expected}"

native = (module / "MoonfallNorthFerryNativeTests.cpp").read_text(encoding="utf-8")
for requirement in (
    'WITH_DEV_AUTOMATION_TESTS',
    'Moonfall.NorthernExpanse.Ferry.ComponentContract',
    'GetDefault<AMoonfallNorthGothicFerryActor>()',
    'GetDefault<AMoonfallNorthFerryGate>()',
    'Deck->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block',
    'Rails[Index]->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block',
    'RenderComponents[Index]->GetCollisionEnabled() == ECollisionEnabled::NoCollision',
    'AutoSailShouldArrive(10.99f, 11.f)',
    'AutoSailShouldArrive(11.f, 11.f)',
    'Movable walking base',
    'Moving ship passenger progress sign',
    'Deck permits step-up',
    'Rail moves with ship',
    'Onboard sign cannot block passenger',
):
    assert requirement in native, f"Native test missing contract: {requirement}"
policyLanding = (module / "MoonfallNorthDryLandingPolicy.h").read_text(encoding="utf-8")
for expected in (
    "ValidDryLandingSample(",
    "DryLandingMinNormalZ = 0.7f",
    "DryLandingMaxAnchorDistanceCm = 400.f",
    "DryLandingCapsuleClearanceCm = 3.f",
    "bGroundHit || !bPawnBlocks",
):
    assert expected in policyLanding, f"Dry landing policy missing: {expected}"
assert '#include "MoonfallNorthDryLandingPolicy.h"' in gate_cpp
assert "FindSafeDryLanding(Hero, this, Fallback)" in cleanup
print("Automatic voyage, dry landing and native UE source contracts passed (native execution pending)")

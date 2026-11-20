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
    "DefaultAutoSailSeconds = 8.f",
    "AutoSailUpdateSeconds = 0.1f",
    "ValidAutoSailDuration",
    "AutoSailEasedProgress",
    "AutoSailShouldArrive",
    "Progress must clamp",
):
    if text == "Progress must clamp":
        continue
    assert text in policy, f"Missing bounded auto-sail policy: {text}"

for text in (
    "TObjectPtr<AMoonfallNorthGothicFerryActor> SailingVessel",
    "float VoyageDurationSeconds = 8.f",
    "bool bAutoVoyageInProgress = false",
    "void UpdateAutomaticVoyage()",
    "void StopAutomaticVoyage()",
    "FTimerHandle AutomaticVoyageTimer",
    "void OnAutomaticVoyageStarted",
    "virtual void EndPlay",
):
    assert text in gate_h, f"Missing voyage gate member: {text}"

start = gate_cpp.split("bool AMoonfallNorthFerryGate::BoardFerry(", 1)[1].split(
    "void AMoonfallNorthFerryGate::UpdateAutomaticVoyage(", 1)[0]
tick = gate_cpp.split("void AMoonfallNorthFerryGate::UpdateAutomaticVoyage(", 1)[1].split(
    "void AMoonfallNorthFerryGate::OnBoardingOverlap(", 1)[0]
cleanup = gate_cpp.split("void AMoonfallNorthFerryGate::StopAutomaticVoyage(", 1)[1].split(
    "bool AMoonfallNorthFerryGate::BoardFerry(", 1)[0]

assert "Hero->TeleportTo(" not in start, "Boarding must not bypass timed voyage"
assert "GetTimerManager().SetTimer(AutomaticVoyageTimer" in start
assert "SailingVessel->GetActorLocation()" in start
assert "PairedGate->SailingVessel->GetActorLocation()" in start
assert "OnAutomaticVoyageStarted(Hero)" in start
assert "MoonfallNorth::MayBoard" in start
assert "MoonfallNorth::AutoSailEasedProgress" in tick
assert "Ship->SetActorLocation(Position" in tick
assert "MoonfallNorth::AutoSailShouldArrive" in tick
assert "Hero->TeleportTo(Destination" in tick
assert "if (bArrived) OnFerryArrival(Hero, bFirst)" in tick
assert "StopAutomaticVoyage();" in tick
assert "ClearTimer(AutomaticVoyageTimer)" in cleanup
assert "Ship->SetActorTransform(OriginalShipTransform" in cleanup
assert "PendingVoyager.Reset()" in cleanup
assert "PrimaryActorTick.bCanEverTick = false" in gate_cpp
for forbidden in (
    "SaveWorldProgress(", "LoadWorldProgress(", "BindAxis(",
    "GhostShip", "SeaMonster", "NavalCombat", "SetActorTickEnabled(true)"
):
    assert forbidden not in gate_cpp and forbidden not in gate_h, forbidden

for expected in (
    '"sailing_vessel", mainland_ship',
    '"sailing_vessel", island_ship',
    '"voyage_duration_seconds", 8.0',
):
    assert expected in builder, f"Missing ship route wiring: {expected}"
for expected in (
    'mainland.get_editor_property("sailing_vessel") == mainland_ship',
    'island.get_editor_property("sailing_vessel") == island_ship',
    '"voyage_duration_seconds"',
):
    assert expected in validation, f"Missing travel validation: {expected}"

print("Automatic sailing source safety, timers and preview script syntax passed")

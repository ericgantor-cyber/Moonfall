"""Static source guards only, not an Unreal compiler or playable voyage."""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
src = root / "MoonfallUnreal/Source/MoonfallUnreal"
header = (src / "MoonfallNorthFerryGate.h").read_text(encoding="utf-8")
code = (src / "MoonfallNorthFerryGate.cpp").read_text(encoding="utf-8")
policy = (src / "MoonfallNorthSeaPassage.h").read_text(encoding="utf-8")

for required in (
    "PrimaryActorTick.bCanEverTick = false",
    "bEnableSeaPassage && PairedGate->bEnableSeaPassage",
    "PairedGate->PairedGate == this",
    "Hero->IsPlayerControlled()",
    "Hero->TeleportTo(",
    "bReachedIslandThisSession = true",
    "FMath::IsFinite(Hero->Health)",
    "Hero->bOptionsOpen",
    "OnFerryArrival(Hero, bFirst)",
):
    assert required in code, f"Missing safe ferry guard: {required}"
for required in (
    "bool bEnableSeaPassage = false",
    "EditInstanceOnly",
    "UFUNCTION(BlueprintCallable",
    "UFUNCTION(BlueprintImplementableEvent",
    "void OnBoardingOverlap",
    "UPROPERTY(VisibleAnywhere, Transient",
):
    assert required in header, f"Missing ferry API/guard: {required}"
assert "ZoneCount == 9" in policy
assert "ValidSeaRoute" in policy and "IsFirstIslandLanding" in policy
assert "SaveWorldProgress(" not in code
assert "LoadWorldProgress(" not in code
assert "SetActorTickEnabled(true)" not in code
print("Northern sea gate source safeguards passed (native UE build still required)")

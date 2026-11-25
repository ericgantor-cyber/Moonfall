"""Source integration checks for the editor-placeable Northern Expanse greybox actor.
These checks do NOT compile Unreal Engine or validate collision in a running game.
"""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
base = root / "MoonfallUnreal" / "Source" / "MoonfallUnreal"
header = (base / "MoonfallNorthSanctumActor.h").read_text(encoding="utf-8")
source = (base / "MoonfallNorthSanctumActor.cpp").read_text(encoding="utf-8")

assert "class MOONFALLUNREAL_API AMoonfallNorthSanctumActor" in header
assert "virtual void OnConstruction" in header
assert "GetBossMarkerWorldLocation" in header
assert "GetEntranceWorldLocation" in header
assert "PrimaryActorTick.bCanEverTick = false;" in source
assert "MoonfallNorth::PlanSanctum(" in source
assert "MoonfallNorth::SafeSanctumPlan(Plan)" in source
assert "SanctumBlocks->ClearInstances();" in source
assert "Index<5" in source
assert "SanctumBlocks->AddInstance" in source
assert "ECollisionEnabled::QueryAndPhysics" in source
assert "DivineSiteIndex < 0 || DivineSiteIndex >= 3" in source
for forbidden in ["SpawnActor", "SaveGameToSlot", "LoadGameFromSlot",
                  "z_void_reaper", "nyra_eclipse_queen", "death_goddess_crimson_reaper"]:
    assert forbidden not in source, forbidden

print("Northern sanctum UE5 actor source policy checks passed")

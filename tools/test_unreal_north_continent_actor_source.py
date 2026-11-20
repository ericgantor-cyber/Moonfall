"""Static safety checks for the optional UE5 Northern Expanse continent actor.
Does not substitute for native Unreal compilation or real navmesh testing.
"""
from pathlib import Path
root=Path(__file__).resolve().parents[1] / "MoonfallUnreal" / "Source" / "MoonfallUnreal"
header=(root/"MoonfallNorthContinentActor.h").read_text(encoding="utf-8")
source=(root/"MoonfallNorthContinentActor.cpp").read_text(encoding="utf-8")
layout=(root/"MoonfallNorthContinentLayout.h").read_text(encoding="utf-8")
assert "AMoonfallNorthContinentActor : public AActor" in header
assert "virtual void OnConstruction" in header
assert "GetProvinceCenterWorldLocation" in header
assert "PrimaryActorTick.bCanEverTick=false;" in source
assert "MoonfallNorth::ValidContinentCenters()" in source
assert "ProvincePads->ClearInstances()" in source
assert "ProvinceRoads->ClearInstances()" in source
assert "Zone<MoonfallNorth::ZoneCount" in source
assert "B=A+1" in source
assert "MoonfallNorth::Linked(A,B)" in source
assert "ProvincePads->AddInstance(" in source
assert "ProvinceRoads->AddInstance(" in source
assert "ECollisionEnabled::QueryAndPhysics" in source
assert "ValidContinentCenters()" in layout
assert "RoadEdgeCount()" in layout
for forbidden in ("SpawnActor", "SaveGameToSlot", "LoadGameFromSlot",
                  "SpawnMorvellaWorldEncounter"):
    assert forbidden not in source, forbidden
print("Northern Expanse connected continent actor source checks passed")

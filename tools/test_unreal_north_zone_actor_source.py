"""Validate Northern Expanse UE5 zone greybox integration without UE5 runtime."""
from pathlib import Path

root=Path(__file__).resolve().parents[1] / "MoonfallUnreal" / "Source" / "MoonfallUnreal"
header=(root/"MoonfallNorthZoneActor.h").read_text(encoding="utf-8")
source=(root/"MoonfallNorthZoneActor.cpp").read_text(encoding="utf-8")
plan=(root/"MoonfallNorthZoneGreybox.h").read_text(encoding="utf-8")
assert "AMoonfallNorthZoneActor : public AActor" in header
assert "virtual void OnConstruction(" in header
assert "int32 ProvinceIndex = 0;" in header
assert "GetDungeonApproachWorldLocation" in header
assert "GetSouthernEntranceWorldLocation" in header
assert "PrimaryActorTick.bCanEverTick = false;" in source
assert "MoonfallNorth::PlanZoneGreybox(ProvinceIndex)" in source
assert "MoonfallNorth::ValidZoneGreybox(Plan)" in source
assert "ZoneFloors->ClearInstances();" in source
assert "ZoneFloors->AddInstance(" in source
assert "ECollisionEnabled::QueryAndPhysics" in source
assert "ECollisionEnabled::NoCollision" in source
assert "ProvinceIndex)" in source
assert "Province.Hub" in source and "Province.Dungeon" in source
assert "Plan.Count=5;" in plan
for dangerous in ("SpawnActor", "LoadGameFromSlot", "SaveGameToSlot", "SpawnMorvellaWorldEncounter"):
    assert dangerous not in source, dangerous
print("Northern Expanse UE5 zone actor source checks passed")

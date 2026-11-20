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
assert "ProvinceLandmarks->AddInstance(" in source
assert "ProvinceLandmarks->ClearInstances()" in source
assert "ProvinceLandmarks->SetCollisionEnabled(ECollisionEnabled::NoCollision)" in source
assert "MoonfallNorth::ValidLandmarks()" in source
assert "MoonfallNorth::LandmarkCount" in source
assert "ProvinceLandmarks" in header
assert "BiomeSurfaceOverlays" in header
assert "bEnableBiomeSurfacePreview=true" in header
assert '#include "MoonfallNorthBiomeTones.h"' in source
assert "MoonfallNorth::ValidBiomeTones()" in source
assert "MoonfallNorth::BiomeTones[Zone]" in source
assert "Overlay->SetCollisionEnabled(ECollisionEnabled::NoCollision)" in source
assert "Overlay->SetGenerateOverlapEvents(false)" in source
assert "Overlay->SetCastShadow(false)" in source
assert "Overlay->CreateAndSetMaterialInstanceDynamic(0)" in source
assert "MID->SetVectorParameterValue(" in source
assert "BiomeSurfaceOverlays.Num() != MoonfallNorth::ZoneCount" in source

assert "ProvinceSigns" in header
assert "CreateDefaultSubobject<UTextRenderComponent>" in source
assert 'ProvinceSigns.Num() != MoonfallNorth::ZoneCount' in source
assert 'Sign->SetCollisionEnabled(ECollisionEnabled::NoCollision)' in source
assert 'Sign->SetGenerateOverlapEvents(false)' in source
assert 'Region.SuggestedLevel' in source
assert 'Region.Hub' in source
assert 'Region.Name' in source
assert '#include "MoonfallNorthClimate.h"' in source
assert "MoonfallNorth::ClimateFor(Zone)" in source
assert "Climate->Name" in source
assert "Sign->SetTextRenderColor" in source
assert "Climate->Red, Climate->Green, Climate->Blue" in source

assert 'ProvinceSigns[Zone]' in source
assert "Region.Dungeon" not in source
assert "ZSanctum" not in source and "NyraSanctum" not in source

assert "ECollisionEnabled::QueryAndPhysics" in source
assert "ValidContinentCenters()" in layout
assert "RoadEdgeCount()" in layout
for forbidden in ("SpawnActor", "SaveGameToSlot", "LoadGameFromSlot",
                  "SpawnMorvellaWorldEncounter"):
    assert forbidden not in source, forbidden
print("Northern Expanse connected continent actor source checks passed")

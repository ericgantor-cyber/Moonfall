"""Static contract for the isolated UE5 Northern Expanse ferry test world.

Run with ordinary Python. Syntax and source checks are NOT a UE5 compile,
world collision test, safe-landing proof, or Windows gameplay validation.
"""
import ast
from pathlib import Path

root = Path(__file__).resolve().parents[1]
ue = root / "MoonfallUnreal"
source = ue / "Source" / "MoonfallUnreal"
builder_path = ue / "Tools" / "create_northern_expanse_preview.py"
validator_path = ue / "Tools" / "validate_northern_expanse_preview.py"
builder = builder_path.read_text(encoding="utf-8")
validator = validator_path.read_text(encoding="utf-8")
pawn = (source / "MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
gate = (source / "MoonfallNorthFerryGate.cpp").read_text(encoding="utf-8")

ast.parse(builder, filename=str(builder_path))
ast.parse(validator, filename=str(validator_path))

for expected in (
    'required_class("MoonfallNorthFerryGate")',
    '"Mainland Harbor Test Dock"',
    'unreal.StaticMeshActor',
    'dock_mesh_component.set_static_mesh(dock_mesh)',
    'dock.set_actor_scale3d(unreal.Vector(34.0, 34.0, 0.4))',
    '"Mainland Ferry To Shiverwake"',
    '"Shiverwake Coast Return Ferry"',
    '"paired_gate", island_gate',
    '"paired_gate", mainland_gate',
    '"b_enable_sea_passage", True',
    '"b_island_harbor", True',
    'unreal.PlayerStart',
    '(1100.0, -10000.0, 190.0)',
    'level_subsystem.save_current_level()',
):
    assert expected in builder, f"Missing preview builder guard: {expected}"

for expected in (
    'len(gates) == 2',
    'len(docks) == 1',
    'mainland.get_editor_property("paired_gate") == island',
    'island.get_editor_property("paired_gate") == mainland',
    'mainland.get_editor_property("b_enable_sea_passage")',
    'abs(island.get_actor_location().y + 1200.0)',
):
    assert expected in validator, f"Missing preview inspection: {expected}"

assert '#include "MoonfallNorthPreviewGameMode.h"' in pawn
begin = pawn.split('void AMoonfallHeroPawn::BeginPlay() {', 1)[1].split(
    'void AMoonfallHeroPawn::Tick(', 1)[0]
load = pawn.split('bool AMoonfallHeroPawn::LoadWorldProgress() {', 1)[1].split(
    'const UMoonfallPersistentSave* Save', 1)[0]
save = pawn.split('bool AMoonfallHeroPawn::SaveWorldProgress() {', 1)[1].split(
    'if (Health <= 0.f', 1)[0]

assert "AMoonfallNorthPreviewGameMode::StaticClass()" in begin
assert "bPersistentSaveWritesBlocked = true;" in begin
assert begin.index("bPersistentSaveWritesBlocked = true;") < begin.index("LoadWorldProgress();")
assert begin.index("!bPersistentSaveWritesBlocked") < begin.index(
    "UGameplayStatics::DoesSaveGameExist(MoonfallWorldSlot, 0)")
assert "AMoonfallNorthPreviewGameMode::StaticClass()" in load
assert load.index("AMoonfallNorthPreviewGameMode::StaticClass()") < load.index(
    "UGameplayStatics::DoesSaveGameExist(MoonfallWorldSlot, 0)")
assert "bPersistentSaveWritesBlocked" in save

assert "Hero->TeleportTo(Destination" in gate
assert "PairedGate->PairedGate == this" in gate
assert "bEnableSeaPassage && PairedGate->bEnableSeaPassage" in gate
assert "GetWorld() != PairedGate->GetWorld()" in gate

# Preview authoring may not mutate the main startup map or game content.
for forbidden in (
    "SaveGameToSlot(", "LoadGameFromSlot(", "delete_asset(",
    "delete_directory(", "set_game_default_map(", "set_project_setting(",
):
    assert forbidden not in builder, forbidden

print("Northern ferry preview syntax and save-isolation source guards passed")

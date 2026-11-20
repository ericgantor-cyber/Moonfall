"""Portable Shiverwake 18-shrine authoring and preview-safety test.

Runs on Linux and Windows without Unreal installed. A fake Editor actor system
tests idempotence; it is NOT a native UE5 map/physics/respawn verification.
"""
import ast
import importlib
import math
import sys
import types
from pathlib import Path

root = Path(__file__).resolve().parents[1]
tools = root / "MoonfallUnreal/Tools"
module_dir = root / "MoonfallUnreal/Source/MoonfallUnreal"
sys.path.insert(0, str(tools))

for file in (
    "shiverwake_shrine_layout.py",
    "shiverwake_shrine_editor.py",
    "create_northern_expanse_preview.py",
    "validate_northern_expanse_preview.py",
    "add_shiverwake_shrines_to_existing_preview.py",
):
    source = (tools / file).read_text(encoding="utf-8")
    ast.parse(source, filename=file)

from shiverwake_shrine_layout import (
    EXPECTED_SHIVERWAKE_SHRINES, SHRINE_SITES, validate_shrine_sites,
)

assert EXPECTED_SHIVERWAKE_SHRINES == 18
assert len(SHRINE_SITES) == 18
assert validate_shrine_sites()
assert len({site[0] for site in SHRINE_SITES}) == 18
assert len({site[1] for site in SHRINE_SITES}) == 18
assert all(all(math.isfinite(v) for v in pos) for _, _, pos in SHRINE_SITES)
assert all(-1200 <= x <= 1200 and -1000 <= y <= 900 and z == 0.0
           for _, _, (x, y, z) in SHRINE_SITES)
# Each 220 cm shrine trigger has clearance from neighboring triggers.
assert all((a[0] - b[0])**2 + (a[1] - b[1])**2 >= 440**2
           for i, (_, _, a) in enumerate(SHRINE_SITES)
           for _, _, b in SHRINE_SITES[i + 1:])

# Verify the editor helper actually creates and reuses the full site layout
# without deleting or renaming an unrelated user's authored shrine.
class FakeVector:
    def __init__(self, x, y, z):
        self.x, self.y, self.z = x, y, z

class FakeActor:
    def __init__(self, position, shrine_id=None, name=""):
        self._position = FakeVector(*position)
        self._properties = {"shrine_id": shrine_id or ""}
        self._name = name

    def get_class(self):
        return types.SimpleNamespace(get_name=lambda: "MoonfallCheckpointShrine")

    def get_editor_property(self, name):
        return self._properties.get(name)

    def set_editor_property(self, name, value):
        self._properties[name] = value

    def get_actor_location(self):
        return self._position

    def get_actor_label(self):
        return self._name

    def set_actor_label(self, value):
        self._name = value

class FakeActors:
    def __init__(self):
        self.actors = []

    def spawn_actor_from_class(self, shrine_class, position, rotation):
        assert shrine_class == "ShrineActor"
        actor = FakeActor((position.x, position.y, position.z))
        self.actors.append(actor)
        return actor

world = FakeActors()
# Preserve an existing shrine, separate from the 18 Shiverwake IDs.
other = FakeActor((3500.0, 3500.0, 0.0), "original_world_shrine", "Existing shrine")
world.actors.append(other)
fake_unreal = types.ModuleType("unreal")
fake_unreal.Name = str
fake_unreal.Text = str
fake_unreal.Vector = FakeVector
fake_unreal.Rotator = lambda *parts: parts
fake_unreal.EditorLevelLibrary = types.SimpleNamespace(
    get_all_level_actors=lambda: world.actors)
sys.modules["unreal"] = fake_unreal
editor = importlib.import_module("shiverwake_shrine_editor")
assert editor.install_shiverwake_shrines(world, "ShrineActor") == 18
assert len(world.actors) == 19
assert editor.install_shiverwake_shrines(world, "ShrineActor") == 0
assert len(world.actors) == 19
assert world.actors[0] is other
assert set(str(actor.get_editor_property("shrine_id")) for actor in world.actors[1:]) == {
    shrine_id for shrine_id, _, _ in SHRINE_SITES
}
for shrine_id, display, position in SHRINE_SITES:
    actor = next(a for a in world.actors if a.get_editor_property("shrine_id") == shrine_id)
    assert actor.get_actor_label() == display
    assert actor.get_editor_property("map_display_name") == display
    assert (actor.get_actor_location().x,
            actor.get_actor_location().y,
            actor.get_actor_location().z) == position

# Conflicting duplicates or moved authored shrines fail instead of being
# silently replaced or moving player checkpoints.
moved = world.actors[1]
moved.get_actor_location().x += 200
try:
    editor.install_shiverwake_shrines(world, "ShrineActor")
    raise AssertionError("Moved shrine was silently overwritten")
except RuntimeError as exc:
    assert "moved" in str(exc)
moved.get_actor_location().x -= 200

# Building and patching both use one shared validated manifest.
builder = (tools / "create_northern_expanse_preview.py").read_text(encoding="utf-8")
updater = (tools / "add_shiverwake_shrines_to_existing_preview.py").read_text(encoding="utf-8")
validator = (tools / "validate_northern_expanse_preview.py").read_text(encoding="utf-8")
for snippet in (
    'required_class("MoonfallCheckpointShrine")',
    'install_shiverwake_shrines(actor_subsystem, shrine_class)',
    'if added_shrines != 18:',
):
    assert snippet in builder, snippet
for snippet in (
    'LEVEL_PATH = "/Game/NorthernExpanse/Maps/NorthernExpansePreview"',
    '"MoonfallNorthPreviewGameMode"',
    'len(harbors) != 2',
    'install_shiverwake_shrines(actors, shrine_class)',
    'level.save_current_level()',
):
    assert snippet in updater, snippet
for snippet in (
    "validate_shrine_sites()",
    'len(found) == 18',
    'set(found) == set(expected_shrines)',
    '"map_display_name"',
    '"discovery_range"',
):
    assert snippet in validator, snippet

hero = (module_dir / "MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
begin = hero.split("void AMoonfallHeroPawn::BeginPlay() {", 1)[1].split(
    "void AMoonfallHeroPawn::Tick(", 1)[0]
discovery = hero.split("bool AMoonfallHeroPawn::DiscoverShrine(", 1)[1].split(
    "int32 AMoonfallHeroPawn::GetDungeonTreasureState(", 1)[0]
respawn = hero.split("void AMoonfallHeroPawn::RespawnAtCheckpoint() {", 1)[1]
assert "bPersistentSaveWritesBlocked = true;" in begin
assert "AMoonfallNorthPreviewGameMode::StaticClass()" in discovery
assert "bPreviewOnlyShrine = bPersistentSaveWritesBlocked" in discovery
assert "(bPersistentSaveWritesBlocked && !bPreviewOnlyShrine)" in discovery
assert "if (!bPreviewOnlyShrine && !SaveWorldProgress())" in discovery
assert "DiscoveredShrineIds.Add(Shrine->ShrineId)" in discovery
assert "ActiveShrineId = Shrine->ShrineId" in discovery
assert "RespawnCheckpointLocation = NewRespawn" in discovery
assert "if (!bPreviewOnlyShrine) SecondsSinceLastWorldSave = 0.f;" in discovery
assert "TeleportTo(RespawnCheckpointLocation" in respawn

for script in (builder, updater):
    for forbidden in ("SaveGameToSlot(", "LoadGameFromSlot(", "delete_asset(",
                      "delete_directory(", "set_game_default_map(",
                      "set_project_setting("):
        assert forbidden not in script, forbidden

print("18 Shiverwake shrine sites, idempotent placement, session-only checkpoints, and preview isolation verified (source/fake Editor only)")

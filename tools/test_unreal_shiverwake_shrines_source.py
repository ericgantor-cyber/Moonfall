"""Portable UE5 Northern Expanse shrine layout and migration regression suite.

Uses a fake Unreal Editor and source assertions, not native UE5 gameplay.
Requires 2 shrines on EACH of the 9 northern province pads, 18 in total.
"""
import ast
import importlib
import math
import re
import sys
import types
from pathlib import Path

root = Path(__file__).resolve().parents[1]
tools = root / "MoonfallUnreal/Tools"
module_dir = root / "MoonfallUnreal/Source/MoonfallUnreal"
sys.path.insert(0, str(tools))

for file in (
    "shiverwake_shrine_layout.py", "shiverwake_shrine_editor.py",
    "create_northern_expanse_preview.py",
    "validate_northern_expanse_preview.py",
    "add_shiverwake_shrines_to_existing_preview.py",
):
    ast.parse((tools / file).read_text(encoding="utf-8"), filename=file)

from shiverwake_shrine_layout import (
    EXPECTED_NORTHERN_SHRINES, LEGACY_SHIVERWAKE_SITES, SHRINE_SITES,
    SHRINE_ZONE_BY_ID, SHRINES_PER_ZONE, ZONE_CENTERS, ZONE_NAMES,
    validate_shrine_sites,
)

assert len(ZONE_NAMES) == len(ZONE_CENTERS) == 9
assert EXPECTED_NORTHERN_SHRINES == len(SHRINE_SITES) == 18
assert SHRINES_PER_ZONE == 2
assert validate_shrine_sites()
assert len({item[0] for item in SHRINE_SITES}) == 18
assert len({item[1] for item in SHRINE_SITES}) == 18
assert [sum(SHRINE_ZONE_BY_ID[item[0]] == zone for item in SHRINE_SITES)
        for zone in range(9)] == [2] * 9
for shrine_id, label, pos in SHRINE_SITES:
    zone = SHRINE_ZONE_BY_ID[shrine_id]
    cx, cy = ZONE_CENTERS[zone]
    assert all(math.isfinite(v) for v in pos)
    assert -1200 <= pos[0]-cx <= 1200 and -1000 <= pos[1]-cy <= 900
    assert pos[2] == 0 and label.endswith(" Shrine")
assert all((a[0]-b[0])**2 + (a[1]-b[1])**2 >= 440**2
           for i, (_, _, a) in enumerate(SHRINE_SITES)
           for _, _, b in SHRINE_SITES[i+1:])
# A nine-province source-of-truth mismatch must fail even if the Python
# catalog and its local tests somehow agree with each other.
layout = (module_dir / "MoonfallNorthContinentLayout.h").read_text(encoding="utf-8")
cpp_centers_body = layout.split(
    "inline constexpr FZonePoint ContinentCenters[ZoneCount] = {", 1
)[1].split("};", 1)[0]
cpp_centers = tuple((int(x), int(y)) for x, y in re.findall(
    r"\{\s*(-?\d+),\s*(-?\d+)\s*\}", cpp_centers_body))
assert cpp_centers == ZONE_CENTERS, "Python shrine centers drifted from UE5 continent"
catalog = (module_dir / "MoonfallNorthZones.h").read_text(encoding="utf-8")
assert "inline constexpr int ZoneCount = 9;" in catalog
assert all('"' + name + '"' in catalog for name in ZONE_NAMES)
for malformed in (SHRINE_SITES[:-1], SHRINE_SITES[:-2] + SHRINE_SITES[0:2]):
    try:
        validate_shrine_sites(malformed)
        raise AssertionError("Accepted missing/duplicate zone shrines")
    except ValueError:
        pass

# Fake editor actor system verifies idempotent creation and lossless
# migration of untouched, generator-tagged old island shrines.
class FakeVector:
    def __init__(self, x, y, z):
        self.x, self.y, self.z = x, y, z

class FakeActor:
    def __init__(self, position, shrine_id="", name="", tags=None):
        self.position = FakeVector(*position)
        self.props = {"shrine_id": shrine_id, "tags": list(tags or []),
                      "map_display_name": name}
        self.label = name

    def get_class(self):
        return types.SimpleNamespace(get_name=lambda: "MoonfallCheckpointShrine")

    def get_editor_property(self, name):
        return self.props.get(name)

    def set_editor_property(self, name, value):
        self.props[name] = value

    def get_actor_location(self):
        return self.position

    def set_actor_location(self, value, sweep, teleport):
        assert not sweep and not teleport
        self.position = value
        return True

    def get_actor_label(self):
        return self.label

    def set_actor_label(self, name):
        self.label = name

class FakeActors:
    def __init__(self):
        self.actors = []

    def spawn_actor_from_class(self, shrine_class, position, rotation):
        assert shrine_class == "ShrineActor"
        actor = FakeActor((position.x, position.y, position.z))
        self.actors.append(actor)
        return actor

world = FakeActors()
unreal = types.ModuleType("unreal")
unreal.Name = str
unreal.Text = str
unreal.log = lambda text: None
unreal.Vector = FakeVector
unreal.Rotator = lambda *args: args
unreal.EditorLevelLibrary = types.SimpleNamespace(
    get_all_level_actors=lambda: world.actors)
sys.modules["unreal"] = unreal
editor = importlib.import_module("shiverwake_shrine_editor")

def check_world(extra_actor):
    shrines = [actor for actor in world.actors if actor is not extra_actor]
    assert len(shrines) == 18
    ids = [actor.get_editor_property("shrine_id") for actor in shrines]
    assert set(ids) == {site[0] for site in SHRINE_SITES}
    assert len(ids) == len(set(ids))
    for shrine_id, label, pos in SHRINE_SITES:
        actor = next(a for a in shrines if a.get_editor_property("shrine_id") == shrine_id)
        p = actor.get_actor_location()
        assert (p.x, p.y, p.z) == pos
        assert actor.get_actor_label() == label
        assert actor.get_editor_property("map_display_name") == label
    assert [sum(SHRINE_ZONE_BY_ID[v] == zone for v in ids) for zone in range(9)] == [2]*9

# Create a fresh map, then run again: no duplicates.
custom = FakeActor((3500., 3500., 0.), "custom_world_shrine", "Authored shrine")
world.actors.append(custom)
assert editor.install_shiverwake_shrines(world, "ShrineActor") == 18
check_world(custom)
assert editor.install_shiverwake_shrines(world, "ShrineActor") == 0
check_world(custom)
assert world.actors[0] is custom
# If a designer changes a generated point, fail closed rather than moving it.
moved = world.actors[1]
moved.get_actor_location().x += 200
try:
    editor.install_shiverwake_shrines(world, "ShrineActor")
    raise AssertionError("A manually moved Northern shrine was overwritten")
except RuntimeError as exc:
    assert "moved" in str(exc)
moved.get_actor_location().x -= 200

# Existing 18-on-Shiverwake greyboxes must Migrate, not retain 16 extras.
world.actors = [custom]
old_generated = []
for shrine_id, label, pos in LEGACY_SHIVERWAKE_SITES:
    actor = FakeActor(pos, shrine_id, label, ["ShiverwakePreviewShrine"])
    old_generated.append(actor)
    world.actors.append(actor)
original_actor_ids = {id(a) for a in world.actors}
assert editor.install_shiverwake_shrines(world, "ShrineActor") == 16
check_world(custom)
assert {id(a) for a in world.actors} == original_actor_ids
assert editor.install_shiverwake_shrines(world, "ShrineActor") == 0
# The original first two Shiverwake identities/actor objects stay unchanged.
assert old_generated[0].get_editor_property("shrine_id") == "shiverwake_landing"
assert old_generated[1].get_editor_property("shrine_id") == "tideglass_watch"
assert world.actors[0] is custom

# A user-edited legacy shrine is NOT safe to migrate; no partial changes.
world.actors = [custom] + [
    FakeActor(pos, shrine_id, label, ["ShiverwakePreviewShrine"])
    for shrine_id, label, pos in LEGACY_SHIVERWAKE_SITES
]
old = world.actors[4]
old.set_actor_label("Designer changed this shrine")
before = [(actor.get_editor_property("shrine_id"), actor.get_actor_label())
          for actor in world.actors]
try:
    editor.install_shiverwake_shrines(world, "ShrineActor")
    raise AssertionError("A custom legacy shrine was silently migrated")
except RuntimeError as exc:
    assert "customized" in str(exc)
assert [(actor.get_editor_property("shrine_id"), actor.get_actor_label())
        for actor in world.actors] == before

# Regression: a *migrated* target must check every existing authored shrine,
# not just the new actors to be spawned. No IDs may change on failed preflight.
world.actors = [custom] + [
    FakeActor(pos, shrine_id, label, ["ShiverwakePreviewShrine"])
    for shrine_id, label, pos in LEGACY_SHIVERWAKE_SITES
]
target_pos = SHRINE_SITES[2][2]  # First new Wolfsong province shrine.
occupied = FakeActor(
    (target_pos[0] + 70.0, target_pos[1], target_pos[2]),
    "authored_wolfsong_altar", "Authored Wolfsong Altar")
world.actors.append(occupied)
ids_before = [actor.get_editor_property("shrine_id") for actor in world.actors]
try:
    editor.install_shiverwake_shrines(world, "ShrineActor")
    raise AssertionError("Migrating shrine overlapped an authored target")
except RuntimeError as exc:
    assert "too close" in str(exc)
assert [actor.get_editor_property("shrine_id") for actor in world.actors] == ids_before
assert occupied.get_actor_label() == "Authored Wolfsong Altar"

# Regression: Unreal can refuse a location change. A later failure must
# roll back earlier moves and their IDs, labels and tags, not half-migrate.
world.actors = [custom] + [
    FakeActor(pos, shrine_id, label, ["ShiverwakePreviewShrine"])
    for shrine_id, label, pos in LEGACY_SHIVERWAKE_SITES
]
old_positions = {
    id(actor): (actor.position.x, actor.position.y, actor.position.z)
    for actor in world.actors
}
old_labels = {id(actor): actor.get_actor_label() for actor in world.actors}
old_ids = {id(actor): actor.get_editor_property("shrine_id") for actor in world.actors}
old_tags = {id(actor): actor.get_editor_property("tags")[:]
            for actor in world.actors}
old_move = FakeActor.set_actor_location
def refuse_second_move(self, value, sweep, teleport):
    if self.get_editor_property("shrine_id") == "ravenrest" and value.y > 1000:
        return False
    return old_move(self, value, sweep, teleport)
FakeActor.set_actor_location = refuse_second_move
try:
    editor.install_shiverwake_shrines(world, "ShrineActor")
    raise AssertionError("Ignored Unreal movement failure during shrine migration")
except RuntimeError as exc:
    assert "refused shrine relocation" in str(exc)
finally:
    FakeActor.set_actor_location = old_move
for actor in world.actors:
    assert (actor.position.x, actor.position.y, actor.position.z) == old_positions[id(actor)]
    assert actor.get_editor_property("shrine_id") == old_ids[id(actor)]
    assert actor.get_actor_label() == old_labels[id(actor)]
    assert actor.get_editor_property("tags") == old_tags[id(actor)]

# A generated shrine with a second (designer-added) tag is no longer
# considered untouched and must not be silently renamed or moved.
world.actors = [custom] + [
    FakeActor(pos, shrine_id, label, ["ShiverwakePreviewShrine"])
    for shrine_id, label, pos in LEGACY_SHIVERWAKE_SITES
]
world.actors[4].set_editor_property("tags",
    ["ShiverwakePreviewShrine", "CustomDesignerTag"])
try:
    editor.install_shiverwake_shrines(world, "ShrineActor")
    raise AssertionError("Migrated designer-tagged shrine")
except RuntimeError as exc:
    assert "customized" in str(exc)

builder = (tools / "create_northern_expanse_preview.py").read_text(encoding="utf-8")
updater = (tools / "add_shiverwake_shrines_to_existing_preview.py").read_text(encoding="utf-8")
validator = (tools / "validate_northern_expanse_preview.py").read_text(encoding="utf-8")
for text in ('required_class("MoonfallCheckpointShrine")',
             'install_shiverwake_shrines(actor_subsystem, shrine_class)',
             'if added_shrines != 18:'):
    assert text in builder, text
for text in ('LEVEL_PATH = "/Game/NorthernExpanse/Maps/NorthernExpansePreview"',
             '"MoonfallNorthPreviewGameMode"', 'len(harbors) != 2',
             'install_shiverwake_shrines(actors, shrine_class)',
             'level.save_current_level()'):
    assert text in updater, text
for text in ("validate_shrine_sites()", "len(found) == 18",
             'per_zone == [SHRINES_PER_ZONE] * 9',
             "LEGACY_SHIVERWAKE_SITES[2:]", '"discovery_range"'):
    assert text in validator, text

hero = (module_dir / "MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
begin = hero.split("void AMoonfallHeroPawn::BeginPlay() {", 1)[1].split(
    "void AMoonfallHeroPawn::Tick(", 1)[0]
discover = hero.split("bool AMoonfallHeroPawn::DiscoverShrine(", 1)[1].split(
    "int32 AMoonfallHeroPawn::GetDungeonTreasureState(", 1)[0]
rest = hero.split("void AMoonfallHeroPawn::RestAtDiscoveredPreviewShrine(", 1)[1].split(
    "int32 AMoonfallHeroPawn::GetDungeonTreasureState(", 1)[0]
respawn = hero.split("void AMoonfallHeroPawn::RespawnAtCheckpoint() {", 1)[1]
assert "bPersistentSaveWritesBlocked = true;" in begin
assert "AMoonfallNorthPreviewGameMode::StaticClass()" in discover
assert "(bPersistentSaveWritesBlocked && !bPreviewOnlyShrine)" in discover
assert "if (!bPreviewOnlyShrine) {" in discover
assert "if (!SaveWorldProgress()) {" in discover
assert discover.index("if (!SaveWorldProgress())") < discover.index(
    "SecondsSinceLastWorldSave = 0.f;")
assert "DiscoveredShrineIds.Add(Shrine->ShrineId)" in discover
assert "ActiveShrineId = Shrine->ShrineId" in discover
assert "RespawnCheckpointLocation = NewRespawn" in discover
assert "TeleportTo(RespawnCheckpointLocation" in respawn
assert "AMoonfallNorthPreviewGameMode::StaticClass()" in rest
assert "HasDiscoveredShrine(Shrine->ShrineId)" in rest
assert all(s in rest for s in ("Health = MaxHealth;", "Mana = MaxMana;",
                              "Stamina = MaxStamina;"))
assert "SaveWorldProgress(" not in rest
shrine_cpp = (module_dir / "MoonfallCheckpointShrine.cpp").read_text(encoding="utf-8")
assert shrine_cpp.count("Hero->RestAtDiscoveredPreviewShrine(this)") == 2

for script in (builder, updater):
    for forbidden in ("SaveGameToSlot(", "LoadGameFromSlot(", "delete_asset(",
                      "delete_directory(", "set_game_default_map(",
                      "set_project_setting("):
        assert forbidden not in script, forbidden

print("Two shrines in each of nine Northern zones, 18 total; fresh/migrated preview and save isolation pass (portable tests only)")

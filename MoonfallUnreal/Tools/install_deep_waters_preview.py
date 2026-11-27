"""Run in UE5.4 after creating the isolated DeepWatersPreview map.
Select one authored rectangular water PhysicsVolume (at least 60 x 40 x 5 m).
Set the level game mode to MoonfallNorthPreviewGameMode before running.
The script never saves or edits the selected volume or any existing actors.
"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from deep_waters_layout import (
    grotto_layout, grotto_geometry, grotto_art, MIN_HALF_WIDTH, MIN_HALF_LENGTH, MIN_DEPTH
)

MAP_PATH = "/Game/DeepWaters/Maps/DeepWatersPreview"
TAG = "MoonfallDeepWatersPreview"


def main():
    import unreal
    import math

    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    world = unreal.EditorLevelLibrary.get_editor_world()
    if world is None or not world.get_path_name().startswith(MAP_PATH + "."):
        raise RuntimeError("Open only the isolated DeepWatersPreview level")
    preview_mode = getattr(unreal, "MoonfallNorthPreviewGameMode", None)
    pocket_class = getattr(unreal, "MoonfallAirPocket", None)
    treasure_class = getattr(unreal, "MoonfallGrottoTreasure", None)
    enemy_class = getattr(unreal, "MoonfallEnemy", None)
    if any(cls is None for cls in (preview_mode, pocket_class, treasure_class, enemy_class)):
        raise RuntimeError("Compile MoonfallUnreal before installing the preview")
    if world.get_world_settings().get_editor_property("default_game_mode") != preview_mode:
        raise RuntimeError("Use the save-isolated MoonfallNorthPreviewGameMode")
    selected = actors.get_selected_level_actors()
    if len(selected) != 1 or not isinstance(selected[0], unreal.PhysicsVolume):
        raise RuntimeError("Select exactly one authored rectangular PhysicsVolume")
    volume = selected[0]
    if not volume.get_editor_property("water_volume"):
        raise RuntimeError("Enable Water Volume on the selected physics volume")
    rotation = volume.get_actor_rotation()
    if any(abs(value) > 0.01 for value in (rotation.pitch, rotation.yaw, rotation.roll)):
        raise RuntimeError("The test pool must be axis aligned")
    origin, extent = volume.get_actor_bounds(False)
    if not all(math.isfinite(v) for v in (origin.x, origin.y, origin.z, extent.x, extent.y, extent.z)):
        raise RuntimeError("Invalid pool bounds")
    if extent.x < MIN_HALF_WIDTH or extent.y < MIN_HALF_LENGTH or extent.z * 2 < MIN_DEPTH:
        raise RuntimeError("Pool must be at least 60 x 40 x 5 meters")
    existing = actors.get_all_level_actors()
    if any(TAG in [str(t) for t in a.get_editor_property("tags")] for a in existing):
        raise RuntimeError("Preview already installed; no duplicate geometry created")
    if any(isinstance(a, unreal.PlayerStart) for a in existing):
        raise RuntimeError("Remove existing PlayerStarts from this dedicated test map first")
    cube = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Cube.Cube")
    if cube is None:
        raise RuntimeError("Engine greybox cube unavailable")
    art_meshes = {
        name: unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/" + name + "." + name)
        for name in ("Cone", "Sphere", "Cylinder")
    }
    if any(mesh is None for mesh in art_meshes.values()):
        raise RuntimeError("Required grotto ornament meshes unavailable")

    surface = origin.z + extent.z
    created = []

    def place(cls, label, pos):
        x, y, z = pos
        actor = actors.spawn_actor_from_class(
            cls, unreal.Vector(origin.x + x, origin.y + y, surface + z),
            unreal.Rotator(0, 0, 0)
        )
        if actor is None:
            raise RuntimeError("Could not place " + label)
        created.append(actor)
        actor.set_actor_label(label)
        actor.set_editor_property("tags", [unreal.Name(TAG)])
        return actor

    def block(label, pos, scale):
        actor = place(unreal.StaticMeshActor, label, pos)
        mesh = actor.get_editor_property("static_mesh_component")
        mesh.set_static_mesh(cube)
        mesh.set_collision_profile_name("BlockAll")
        actor.set_actor_scale3d(unreal.Vector(*scale))

    def decorate(name, index, x, y):
        ornaments, light_pos, color = grotto_art(index, x, y)
        for label, shape, pos, scale in ornaments:
            actor = place(unreal.StaticMeshActor, name + " " + label, pos)
            mesh = actor.get_editor_property("static_mesh_component")
            mesh.set_static_mesh(art_meshes[shape])
            mesh.set_collision_profile_name("NoCollision")
            mesh.set_editor_property("cast_shadow", False)
            actor.set_actor_scale3d(unreal.Vector(*scale))
        lamp = place(unreal.PointLight, name + " ambient beacon", light_pos)
        light = lamp.get_component_by_class(unreal.PointLightComponent)
        if light is None:
            raise RuntimeError("Missing grotto point light component")
        light.set_light_color(unreal.LinearColor(*color, 1.0))
        light.set_intensity(1800.0)
        light.set_attenuation_radius(480.0)
        light.set_editor_property("cast_shadows", False)

    try:
        # A dry deck adjacent to the pool, with a submerged step and open exit.
        block("Deep Waters dry dock", (-extent.x - 300, 0, -20), (7, 12, 0.4))
        block("Deep Waters entry step", (-extent.x + 150, 0, -90), (4, 12, 0.4))
        place(unreal.PlayerStart, "Deep Waters start", (-extent.x - 300, 0, 150))
        block("Deep Waters pool floor", (0, 0, -430), (extent.x / 50, extent.y / 50, 0.4))
        for index, (name, (x, y, z)) in enumerate(grotto_layout()):
            for piece, position, scale in grotto_geometry(x, y):
                block(name + " " + piece, position, scale)
            place(pocket_class, name + " Air Pocket", (x, y, z))
            decorate(name, index, x, y)
            chest = place(treasure_class, name + " Treasure", (x, y + 155.0, -300.0))
            chest.set_editor_property("grotto_index", index)
            enemy = place(enemy_class, name + " Aquatic Guardian", (x, y - 240.0, -225.0))
            enemy.set_editor_property("tags", [unreal.Name(TAG), unreal.Name("MoonfallAquaticPreview")])
            enemy.set_editor_property("health", 60.0)
        place(unreal.DirectionalLight, "Deep Waters preview light", (0, 0, 1500))
    except Exception:
        for actor in reversed(created):
            actors.destroy_actor(actor)
        raise
    unreal.log("Installed six grotto greyboxes, six air pockets, 24 ornaments and six ambient beacons. Inspect and save manually.")


if __name__ == "__main__":
    main()

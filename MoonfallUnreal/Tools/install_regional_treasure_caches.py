"""Add regional tonic treasure caches to selected installed water volumes.

Run in DeepWatersPreview after compiling the V4 regional treasure changes.
No existing chest is modified, and the map is never saved automatically.
"""
import math

NAMES = ("Frozen Shipwreck", "Drowned Root Caverns", "Frostveil Caves",
         "Sunken Necropolis", "Emerald Reef", "Thunderstone Cavern",
         "Lost Royal City", "Astral Crystal Grotto", "Abyssal Eclipse Temple")


def main():
    import unreal
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    world = unreal.EditorLevelLibrary.get_editor_world()
    if world is None or not world.get_path_name().startswith("/Game/DeepWaters/Maps/DeepWatersPreview."):
        raise RuntimeError("Open only DeepWatersPreview")
    mode = getattr(unreal, "MoonfallNorthPreviewGameMode", None)
    treasure = getattr(unreal, "MoonfallGrottoTreasure", None)
    if mode is None or treasure is None or world.get_world_settings().get_editor_property("default_game_mode") != mode:
        raise RuntimeError("Compile and select isolated preview game mode")
    selected = actors.get_selected_level_actors()
    if not selected:
        raise RuntimeError("Select installed regional water volumes")
    existing = actors.get_all_level_actors()
    def tags(a):
        return {str(t) for t in a.get_editor_property("tags")}
    used = set()
    plans = []
    for volume in selected:
        if not isinstance(volume, unreal.PhysicsVolume) or not volume.get_editor_property("water_volume"):
            raise RuntimeError("Select only water PhysicsVolumes")
        ids = [i for i in range(9) if f"MoonfallFishingProvince_{i}" in tags(volume)]
        if len(ids) != 1 or ids[0] in used:
            raise RuntimeError("Each selected pool must have a distinct province")
        i = ids[0]
        used.add(i)
        tag = f"MoonfallRegionalWaterInstalled_{i}"
        region = f"MoonfallFishingProvince_{i}"
        if any(a != volume and isinstance(a, unreal.PhysicsVolume) and region in tags(a) for a in existing):
            raise RuntimeError("Duplicate province volume")
        if not any(tag in tags(a) for a in existing):
            raise RuntimeError("Install regional water first")
        rot = volume.get_actor_rotation()
        if any(not math.isfinite(v) or abs(v) > .01 for v in (rot.pitch, rot.yaw, rot.roll)):
            raise RuntimeError("Axis-aligned water only")
        origin, extent = volume.get_actor_bounds(False)
        if not all(math.isfinite(v) for v in (origin.x,origin.y,origin.z,extent.x,extent.y,extent.z)):
            raise RuntimeError("Invalid bounds")
        if extent.x < 600 or extent.y < 600 or extent.z < 250:
            raise RuntimeError("Pool must be at least 12 x 12 x 5 m")
        matches = [a for a in existing if isinstance(a, treasure) and
                   a.get_editor_property("resource_index") == -1 and
                   a.get_editor_property("grotto_index") == 6+i]
        if len(matches) > 1:
            raise RuntimeError("Duplicate regional treasure identity")
        if matches:
            loc = matches[0].get_actor_location()
            if not (abs(loc.x-origin.x) < extent.x and abs(loc.y-origin.y) < extent.y
                    and abs(loc.z-origin.z) < extent.z):
                raise RuntimeError("Existing cache outside its region")
            continue
        pos = unreal.Vector(origin.x-.1*extent.x, origin.y+.18*extent.y, origin.z+extent.z-300)
        plans.append((i, tag, pos))
    created = []
    try:
        for i, tag, pos in plans:
            chest = actors.spawn_actor_from_class(treasure, pos, unreal.Rotator(0,0,0))
            if chest is None:
                raise RuntimeError("Regional cache spawn failed")
            created.append(chest)
            chest.set_actor_label(NAMES[i]+" tonic cache")
            chest.set_editor_property("tags",[unreal.Name(tag)])
            chest.set_editor_property("grotto_index",6+i)
            chest.set_editor_property("resource_index",-1)
            if chest.get_editor_property("grotto_index") != 6+i:
                raise RuntimeError("Regional cache ID rejected; compile V4 first")
    except Exception:
        for chest in reversed(created):
            actors.destroy_actor(chest)
        raise
    unreal.log(f"Added {len(created)} regional caches. Inspect collision, access and reward behavior before saving.")


if __name__ == "__main__":
    main()

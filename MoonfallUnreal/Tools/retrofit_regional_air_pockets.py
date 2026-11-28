"""Add a breathing source to existing regional pools in DeepWatersPreview.

Select already installed province-tagged water volumes. No terrain changes,
automatic map saves, or removal of existing actors. Inspect clearance first.
"""
import math


def main():
    import unreal
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    world = unreal.EditorLevelLibrary.get_editor_world()
    if world is None or not world.get_path_name().startswith(
            "/Game/DeepWaters/Maps/DeepWatersPreview."):
        raise RuntimeError("Open only DeepWatersPreview")
    mode = getattr(unreal, "MoonfallNorthPreviewGameMode", None)
    pocket = getattr(unreal, "MoonfallAirPocket", None)
    if mode is None or pocket is None or world.get_world_settings().get_editor_property("default_game_mode") != mode:
        raise RuntimeError("Compile and select isolated preview game mode")
    selected = actors.get_selected_level_actors()
    if not selected:
        raise RuntimeError("Select installed regional water volumes")
    existing = actors.get_all_level_actors()
    def tags(actor):
        return {str(t) for t in actor.get_editor_property("tags")}
    plans = []
    used = set()
    for volume in selected:
        if not isinstance(volume, unreal.PhysicsVolume) or not volume.get_editor_property("water_volume"):
            raise RuntimeError("Select only water PhysicsVolumes")
        ids = [i for i in range(9) if f"MoonfallFishingProvince_{i}" in tags(volume)]
        if len(ids) != 1 or ids[0] in used:
            raise RuntimeError("Exactly one distinct province per volume")
        province = ids[0]
        used.add(province)
        region_tag = f"MoonfallFishingProvince_{province}"
        install_tag = f"MoonfallRegionalWaterInstalled_{province}"
        if any(a != volume and isinstance(a, unreal.PhysicsVolume) and region_tag in tags(a) for a in existing):
            raise RuntimeError("Duplicate province volume")
        if not any(install_tag in tags(a) for a in existing):
            raise RuntimeError("Install regional fishing banks first")
        rot = volume.get_actor_rotation()
        if any(not math.isfinite(v) or abs(v) > .01 for v in (rot.pitch, rot.yaw, rot.roll)):
            raise RuntimeError("Axis-aligned water only")
        origin, extent = volume.get_actor_bounds(False)
        if not all(math.isfinite(v) for v in (origin.x, origin.y, origin.z, extent.x, extent.y, extent.z)):
            raise RuntimeError("Invalid bounds")
        if extent.x < 600 or extent.y < 600 or extent.z < 250:
            raise RuntimeError("Pool must be at least 12 x 12 x 5 m")
        # Skip any existing air pocket in the pool, including those installed
        # by the newer regional installer. Never alter user-authored pockets.
        has_pocket = False
        for actor in existing:
            if not isinstance(actor, pocket):
                continue
            loc = actor.get_actor_location()
            if (abs(loc.x-origin.x) <= extent.x and
                abs(loc.y-origin.y) <= extent.y and
                abs(loc.z-origin.z) <= extent.z):
                has_pocket = True
                break
            if install_tag in tags(actor):
                raise RuntimeError("Tagged breathing source outside its pool; inspect manually")
        if not has_pocket:
            pos = unreal.Vector(origin.x + .4*extent.x, origin.y, origin.z + extent.z - 280)
            if not all(math.isfinite(v) for v in (pos.x,pos.y,pos.z)):
                raise RuntimeError("Invalid pocket location")
            plans.append((province, install_tag, pos))
    created = []
    try:
        for province, tag, pos in plans:
            actor = actors.spawn_actor_from_class(pocket, pos, unreal.Rotator(0,0,0))
            if actor is None:
                raise RuntimeError("Breathing source spawn failed")
            created.append(actor)
            actor.set_actor_label(f"Province {province+1} breathing bubble source")
            actor.set_editor_property("tags", [unreal.Name(tag)])
    except Exception:
        for actor in reversed(created):
            actors.destroy_actor(actor)
        raise
    unreal.log(f"Added {len(created)} breathing sources. Inspect collision clearance and swim access before manually saving.")


if __name__ == "__main__":
    main()

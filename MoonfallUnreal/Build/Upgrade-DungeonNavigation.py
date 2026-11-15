"""Explicit, backup-first nav bounds retrofit for an EXISTING MoonfallStart.umap.

Run in UE5.4 Editor Python console or with -ExecutePythonScript.
Does not replace maps, game objects, artwork, or authored nav volumes.
Writes a .before-dungeon-nav.umap backup first and adds only one labeled
NavMeshBoundsVolume when no matching generated volume exists.
"""
from pathlib import Path
import shutil
import unreal

MAP = "/Game/Maps/MoonfallStart"
LABEL = "Moonfall_DungeonWorld_NavBounds"
CENTER = unreal.Vector(0.0, -10000.0, 100.0)
SCALE = unreal.Vector(225.0, 115.0, 6.0)


def upgrade_existing_map_navigation():
    root = Path(unreal.Paths.project_dir()).resolve()
    if not (root / "MoonfallUnreal.uproject").is_file():
        raise RuntimeError("Open MoonfallUnreal.uproject first")
    level_file = root / "Content" / "Maps" / "MoonfallStart.umap"
    if not level_file.is_file() or level_file.stat().st_size == 0:
        raise RuntimeError("Cannot retrofit missing/empty MoonfallStart.umap")
    if not unreal.EditorAssetLibrary.does_asset_exist(MAP):
        raise RuntimeError("Existing map asset is not readable; refusing to overwrite")
    world = unreal.EditorLoadingAndSavingUtils.load_map(MAP)
    if world is None:
        raise RuntimeError("Could not load existing MoonfallStart map")
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    matching = [
        actor for actor in actors.get_all_level_actors()
        if isinstance(actor, unreal.NavMeshBoundsVolume)
        and actor.get_actor_label() == LABEL
    ]
    if len(matching) > 1:
        raise RuntimeError("Duplicate named dungeon nav bounds; inspect map manually")
    if matching:
        unreal.log("Existing dungeon navigation bounds detected, no changes saved")
        return

    # Never touch a real authored map without a byte-for-byte recovery copy.
    backup = level_file.with_suffix(".before-dungeon-nav.umap")
    if backup.exists() and backup.stat().st_size == 0:
        raise RuntimeError("Existing nav backup is empty, manual recovery needed")
    if not backup.exists():
        shutil.copy2(level_file, backup)
    nav = actors.spawn_actor_from_class(
        unreal.NavMeshBoundsVolume, CENTER, unreal.Rotator(0.0, 0.0, 0.0))
    if nav is None:
        raise RuntimeError("Unreal could not place dungeon NavMeshBoundsVolume")
    nav.set_actor_label(LABEL)
    nav.set_actor_scale3d(SCALE)
    # Verify brush geometry, not just scale numbers. Engine default builder
    # dimensions vary across editor installations.
    origin, extent = nav.get_actor_bounds(False)
    if (origin.x - extent.x > -20500.0
            or origin.x + extent.x < 20500.0
            or origin.y - extent.y > -19800.0
            or origin.y + extent.y < -1600.0):
        actors.destroy_actor(nav)
        raise RuntimeError(
            "Dungeon nav bounds brush too small; map NOT saved. "
            "Resize brush to cover X +/-20500 and Y -19800 to -1600.")
    if not unreal.EditorLoadingAndSavingUtils.save_current_level():
        raise RuntimeError("Could not save upgraded map; backup preserved")
    unreal.log(
        "Dungeon navigation bounds added. Original map backed up to " + str(backup)
        + ". Build/rebuild navigation, press P in Editor, and playtest.")


if __name__ == "__main__":
    upgrade_existing_map_navigation()

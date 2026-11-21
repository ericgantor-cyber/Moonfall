"""Backup-first NavMesh bounds retrofit for the new outdoor overworld.

Run inside Unreal Editor 5.4 on an existing MoonfallStart.umap.
Never overwrites the map, any authored actor, hero art, or existing bounds.
This script adds ONE labeled NavMeshBoundsVolume if absent.
"""
from pathlib import Path
import filecmp
import shutil
import unreal

MAP = "/Game/Maps/MoonfallStart"
LABEL = "Moonfall_ExpandedWorld_NavBounds"
CENTER = unreal.Vector(0.0, 170000.0, 100.0)
SCALE = unreal.Vector(1900.0, 2000.0, 6.0)


def sufficient_bounds(actor):
    origin, extent = actor.get_actor_bounds(False)
    return not (
        origin.x - extent.x > -170500.0
        or origin.x + extent.x < 170500.0
        or origin.y - extent.y > 12000.0
        or origin.y + extent.y < 352000.0
    )


def upgrade_existing_map_navigation():
    root = Path(unreal.Paths.project_dir()).resolve()
    if not (root / "MoonfallUnreal.uproject").is_file():
        raise RuntimeError("Open MoonfallUnreal.uproject first")
    level_file = root / "Content" / "Maps" / "MoonfallStart.umap"
    if not level_file.is_file() or level_file.stat().st_size <= 0:
        raise RuntimeError("Missing or empty MoonfallStart.umap; refusing to create a replacement")
    if not unreal.EditorAssetLibrary.does_asset_exist(MAP):
        raise RuntimeError("MoonfallStart map is unreadable; refusing to overwrite")
    world = unreal.EditorLoadingAndSavingUtils.load_map(MAP)
    if world is None:
        raise RuntimeError("Could not load the original MoonfallStart level")

    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    matches = [
        actor for actor in actors.get_all_level_actors()
        if isinstance(actor, unreal.NavMeshBoundsVolume)
        and actor.get_actor_label() == LABEL
    ]
    if len(matches) > 1:
        raise RuntimeError("Duplicate expanded-world NavMesh bounds; inspect map manually")
    if matches:
        if not sufficient_bounds(matches[0]):
            raise RuntimeError("Existing expanded-world navigation brush is undersized")
        unreal.log("Existing expanded-world NavMesh bounds already adequate; no changes saved")
        return

    # Byte-for-byte recovery first. Never silently replace an older backup.
    backup = level_file.with_suffix(".before-expanded-world-nav.umap")
    if backup.exists():
        if backup.stat().st_size <= 0 or not filecmp.cmp(level_file, backup, shallow=False):
            raise RuntimeError("Existing expanded-world backup differs. Review before retrying.")
    else:
        shutil.copy2(level_file, backup)

    nav = actors.spawn_actor_from_class(
        unreal.NavMeshBoundsVolume, CENTER, unreal.Rotator(0.0, 0.0, 0.0)
    )
    if nav is None:
        raise RuntimeError("Unable to create NavMeshBoundsVolume; backup preserved")
    nav.set_actor_label(LABEL)
    nav.set_actor_scale3d(SCALE)
    if not sufficient_bounds(nav):
        actors.destroy_actor(nav)
        raise RuntimeError("Expanded-world nav brush too small; no map saved")

    if not unreal.EditorLoadingAndSavingUtils.save_current_level():
        raise RuntimeError("Failed saving expanded-world navigation; backup preserved")
    unreal.log(
        "Expanded-world navigation bounds added; map backed up to " + str(backup)
        + ". Press P in Unreal Editor and test walkable tiles and ferry shores."
    )


if __name__ == "__main__":
    upgrade_existing_map_navigation()

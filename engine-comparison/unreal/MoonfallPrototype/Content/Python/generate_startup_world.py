"""Generate a minimal saved UE5.6 startup level for the procedural MoonfallPawn world.

Run INSIDE Unreal Editor Python. This produces a .umap only on a machine with UE5.6.
Never overwrite an artist-authored existing startup level.
"""
import unreal

LEVEL_PATH = "/Game/Maps/MoonfallWorld"
assets = unreal.EditorAssetLibrary
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

if assets.does_asset_exist(LEVEL_PATH):
    unreal.log("Moonfall startup map already exists; preserving the authored asset.")
    if not levels.load_level(LEVEL_PATH):
        raise RuntimeError("Existing MoonfallWorld could not be loaded.")
else:
    if not levels.new_level(LEVEL_PATH, False):
        raise RuntimeError("Failed to create persistent /Game/Maps/MoonfallWorld")

    # Keep this stage minimal; the pawn builds six connected regions on BeginPlay.
    # PlayerStart spawn location is inside the procedural world bounds.
    start = actors.spawn_actor_from_class(
        unreal.PlayerStart, unreal.Vector(0.0, 0.0, 150.0), unreal.Rotator()
    )
    sun = actors.spawn_actor_from_class(
        unreal.DirectionalLight, unreal.Vector(0.0, 0.0, 1400.0),
        unreal.Rotator(-45.0, 20.0, 0.0)
    )
    ambient = actors.spawn_actor_from_class(
        unreal.SkyLight, unreal.Vector(0.0, 0.0, 1200.0), unreal.Rotator()
    )
    if not start or not sun or not ambient:
        raise RuntimeError("Could not spawn required startup level actors.")
    if not levels.save_current_level():
        raise RuntimeError("MoonfallWorld save failed.")

if not assets.does_asset_exist(LEVEL_PATH):
    raise RuntimeError("MoonfallWorld asset missing after editor generation.")
unreal.log("Moonfall startup level saved at " + LEVEL_PATH)

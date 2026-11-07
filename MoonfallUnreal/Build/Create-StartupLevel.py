"""Create a minimal, project-owned Moonfall startup map in Unreal Editor 5.4.

Run from Unreal Editor's Python console:
  exec(open(r"ABSOLUTE_PATH_TO/MoonfallUnreal/Build/Create-StartupLevel.py", encoding="utf-8").read())

Requires Python Editor Script Plugin; does not overwrite an existing level.
The C++ MoonfallUnrealGameMode spawns the placeholder arena at runtime.
"""
import unreal

MAP = "/Game/Maps/MoonfallStart"


def create_moonfall_start():
    if unreal.EditorAssetLibrary.does_asset_exist(MAP):
        raise RuntimeError(
            "MoonfallStart already exists. Open it for manual editing; "
            "this script deliberately refuses to overwrite a saved level."
        )

    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if not levels.new_level(MAP, is_partitioned_world=False):
        raise RuntimeError("Unreal Editor failed to create " + MAP)

    # Spawn is above the C++ arena floor at Z=-60 to avoid starting embedded.
    player = actors.spawn_actor_from_class(
        unreal.PlayerStart, unreal.Vector(0.0, 0.0, 130.0),
        unreal.Rotator(0.0, 0.0, 0.0))
    if player is None:
        raise RuntimeError("Could not create PlayerStart in " + MAP)

    nav = actors.spawn_actor_from_class(
        unreal.NavMeshBoundsVolume, unreal.Vector(0.0, 0.0, 100.0),
        unreal.Rotator(0.0, 0.0, 0.0))
    if nav is None:
        raise RuntimeError("Could not create NavMeshBoundsVolume in " + MAP)
    # Extents depend on Unreal's default brush, verify coverage in Editor (P).
    nav.set_actor_scale3d(unreal.Vector(20.0, 20.0, 4.0))

    if not levels.save_current_level():
        raise RuntimeError("Created level, but could not save " + MAP)

    unreal.log(
        "Created /Game/Maps/MoonfallStart. In DefaultEngine.ini set "
        "GameDefaultMap and EditorStartupMap to /Game/Maps/MoonfallStart; "
        "then verify navigation, player spawn and combat in Play In Editor."
    )


if __name__ == "__main__":
    create_moonfall_start()

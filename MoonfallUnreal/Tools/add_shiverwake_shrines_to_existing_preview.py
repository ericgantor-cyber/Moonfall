"""Add missing Shiverwake shrines to an EXISTING isolated UE5 preview map.

Run in UE5 Editor via Tools > Execute Python Script. Unlike the creation
script, this deliberately updates ONLY /Game/NorthernExpanse/Maps/NorthernExpansePreview.
Other levels, player saves and authored shrines are never touched.
"""
import sys
from pathlib import Path
import unreal

sys.path.insert(0, str(Path(__file__).resolve().parent))
from shiverwake_shrine_editor import install_shiverwake_shrines

LEVEL_PATH = "/Game/NorthernExpanse/Maps/NorthernExpansePreview"


def main():
    shrine_class = getattr(unreal, "MoonfallCheckpointShrine", None)
    if shrine_class is None:
        raise RuntimeError("Compile the Moonfall UE5 shrine actor before updating the preview")
    if not unreal.EditorAssetLibrary.does_asset_exist(LEVEL_PATH):
        raise RuntimeError("Isolated Northern preview map not present; use creation script first")
    level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if level is None or actors is None or not level.load_level(LEVEL_PATH):
        raise RuntimeError("Cannot safely open the isolated Northern preview map")
    world = unreal.EditorLevelLibrary.get_editor_world()
    if world is None:
        raise RuntimeError("Unreal preview world is not available")
    settings = world.get_world_settings()
    if settings is None or "MoonfallNorthPreviewGameMode" not in str(
            settings.get_editor_property("default_game_mode")):
        raise RuntimeError("Refusing to modify a world without the isolated preview game mode")
    world_actors = unreal.EditorLevelLibrary.get_all_level_actors()
    if len([actor for actor in world_actors
            if actor.get_class().get_name() == "MoonfallNorthContinentActor"]) != 1:
        raise RuntimeError("Refusing to add island shrines without the correct continent")
    harbors = [actor for actor in world_actors
               if actor.get_class().get_name() == "MoonfallNorthFerryGate"]
    if len(harbors) != 2 or sum(bool(h.get_editor_property("b_island_harbor"))
                               for h in harbors) != 1:
        raise RuntimeError("Refusing to alter a preview without both ferry endpoints")
    created = install_shiverwake_shrines(actors, shrine_class)
    if created > 0:
        if not level.save_current_level():
            raise RuntimeError("Could not save the 18-shrine preview update")
    unreal.log("Shiverwake shrines updated: %d created, previously authored actors preserved" % created)


if __name__ == "__main__":
    main()

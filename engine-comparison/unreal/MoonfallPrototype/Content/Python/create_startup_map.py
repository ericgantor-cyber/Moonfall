"""Create a minimal Moonfall startup level using Unreal Editor 5.6 Python.

Only runs inside Unreal Editor with PythonScriptPlugin enabled. The C++ game
mode creates the experimental runtime world, so no Godot assets are changed.
"""
import unreal

LEVEL_PATH = "/Game/Maps/MoonfallPrototype"

assets = unreal.EditorAssetLibrary
if assets.does_asset_exist(LEVEL_PATH):
    unreal.log("Moonfall startup map already exists; leaving it unchanged.")
else:
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not levels.new_level(LEVEL_PATH, is_partitioned_world=False):
        raise RuntimeError("Unable to create " + LEVEL_PATH)
    # A created editor world is not enough: persist the level package to disk
    # before the separate Win64 packaging process looks for its .umap file.
    if not levels.save_current_level():
        raise RuntimeError("Unreal Editor could not save the startup map: " + LEVEL_PATH)
    if not assets.does_asset_exist(LEVEL_PATH):
        raise RuntimeError("Editor reported success but the startup map is missing.")
    unreal.log("Created Moonfall startup map: " + LEVEL_PATH)

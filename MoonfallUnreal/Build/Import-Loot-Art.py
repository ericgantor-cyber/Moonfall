"""Import Moonfall's painted loot atlas into the UE5.4 Content Browser.

Run in the Unreal Editor Python console:
  exec(open(r"<PROJECT>/Build/Import-Loot-Art.py", encoding="utf-8").read())

Or run from UnrealEditor-Cmd with the PythonScriptPlugin enabled.
Generated assets must be saved/cooked for packaged Windows builds.
"""
from pathlib import Path
import unreal

SOURCE = Path(unreal.Paths.project_dir()).resolve() / "Data" / "LootArt" / "Moonfall_LootAtlas.png"
DESTINATION = "/Game/Moonfall/LootArt"
ASSET_NAME = "T_MoonfallLootAtlas"
ASSET_PATH = f"{DESTINATION}/{ASSET_NAME}"

if not SOURCE.is_file() or SOURCE.stat().st_size < 64:
    raise FileNotFoundError(f"Missing Moonfall source loot PNG: {SOURCE}")

# Idempotent reimport. Save a real Texture2D so the native HUD's hard
# reference can be cooked; the source PNG alone is not a cooked UE asset.
task = unreal.AssetImportTask()
task.set_editor_property("filename", str(SOURCE))
task.set_editor_property("destination_path", DESTINATION)
task.set_editor_property("destination_name", ASSET_NAME)
task.set_editor_property("automated", True)
task.set_editor_property("replace_existing", True)
task.set_editor_property("save", True)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
asset = unreal.load_asset(ASSET_PATH)
if not asset or not isinstance(asset, unreal.Texture2D):
    raise RuntimeError(f"Moonfall loot art import did not create Texture2D: {ASSET_PATH}")
# Do not silently overwrite unrelated textures in a different directory.
unreal.EditorAssetLibrary.save_loaded_asset(asset)
unreal.log(f"Moonfall loot art imported: {ASSET_PATH}. Reopen Play/Editor to load the HUD atlas.")

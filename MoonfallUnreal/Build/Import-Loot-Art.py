"""Import Moonfall's two painted loot atlases as UE5.4 Texture2D assets.

Run once in the Unreal Editor Python console:
  exec(open(r"<PROJECT>/Build/Import-Loot-Art.py", encoding="utf-8").read())

Both source PNGs are under Data/LootArt. The output assets must be saved
and cooked with the project for the optional native Canvas icon layers.
"""
from pathlib import Path
import struct
import unreal

SOURCE_DIR = Path(unreal.Paths.project_dir()).resolve() / "Data" / "LootArt"
DESTINATION = "/Game/Moonfall/LootArt"
ATLASES = (
    ("Moonfall_LootAtlas.png", "T_MoonfallLootAtlas", (120, 80)),
    ("Moonfall_LootVariants_Atlas.png", "T_MoonfallLootVariants", (120, 120)),
)

# Validate both source images before changing any Unreal asset. This avoids
# partial imports after a missing or damaged source file.
for filename, _, expected_size in ATLASES:
    source = SOURCE_DIR / filename
    if not source.is_file() or not 64 <= source.stat().st_size <= 512 * 1024:
        raise FileNotFoundError(f"Moonfall loot art source is missing or oversized: {source}")
    with source.open("rb") as image_file:
        header = image_file.read(24)
    if len(header) != 24 or header[:8] != bytes.fromhex("89504e470d0a1a0a"):
        raise ValueError(f"Moonfall loot art is not a PNG: {source}")
    if struct.unpack(">II", header[16:24]) != expected_size:
        raise ValueError(f"Wrong PNG atlas size for {filename}: expected {expected_size}")

for filename, asset_name, _ in ATLASES:
    source = SOURCE_DIR / filename
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(source))
    task.set_editor_property("destination_path", DESTINATION)
    task.set_editor_property("destination_name", asset_name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    asset_path = f"{DESTINATION}/{asset_name}"
    asset = unreal.load_asset(asset_path)
    if not asset or not isinstance(asset, unreal.Texture2D):
        raise RuntimeError(f"Moonfall loot art did not import as Texture2D: {asset_path}")
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset):
        raise RuntimeError(f"Could not save imported Moonfall loot art: {asset_path}")
    unreal.log(f"Moonfall loot art imported: {asset_path}")

unreal.log("Moonfall loot art ready. Restart the Editor so native HUD default assets refresh.")

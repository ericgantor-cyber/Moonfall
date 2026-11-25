"""Import generated texture sources into isolated UE5 content.

No existing packages are changed. Run the texture generator first.
"""
from pathlib import Path
import unreal

PROJECT = Path(unreal.Paths.project_dir()).resolve()
SOURCE = PROJECT / "Art/Production/Textures"
DEST = "/Game/MoonfallProduction/Textures"
NAMES = ("Bronze", "Leather", "Steel", "Stone", "Moonstone")
MAPS = ("BaseColor", "Normal", "ORM")

def run():
    if not (PROJECT / "MoonfallUnreal.uproject").is_file():
        raise RuntimeError("Open MoonfallUnreal first.")
    tasks = []
    for surface in NAMES:
        for kind in MAPS:
            name = "T_MF_" + surface + "_" + kind
            path = SOURCE / (name + ".png")
            if not path.is_file() or path.resolve().parent != SOURCE.resolve():
                raise RuntimeError("Generate missing source: " + str(path))
            target = DEST + "/" + name
            if unreal.EditorAssetLibrary.does_asset_exist(target):
                if not isinstance(unreal.EditorAssetLibrary.load_asset(target), unreal.Texture2D):
                    raise RuntimeError("Unexpected existing asset: " + target)
                continue
            task = unreal.AssetImportTask()
            task.set_editor_property("filename", str(path))
            task.set_editor_property("destination_path", DEST)
            task.set_editor_property("destination_name", name)
            task.set_editor_property("automated", True)
            task.set_editor_property("replace_existing", False)
            task.set_editor_property("save", False)
            tasks.append((target, kind, task))
    if tasks:
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t for _, _, t in tasks])
    for target, kind, task in tasks:
        texture = unreal.EditorAssetLibrary.load_asset(target)
        if not task.get_editor_property("imported_object_paths") or not isinstance(texture, unreal.Texture2D):
            raise RuntimeError("Texture import failed: " + target)
        texture.set_editor_property("srgb", kind == "BaseColor")
        if kind == "Normal":
            texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
        elif kind == "ORM":
            texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
        if not unreal.EditorAssetLibrary.save_loaded_asset(texture, False):
            raise RuntimeError("Texture save failed: " + target)
    unreal.log("Production texture imports complete. Material graph binding is a separate step.")

if __name__ == "__main__":
    run()

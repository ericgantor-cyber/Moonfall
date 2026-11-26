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

def validate_texture(texture, kind, target):
    if texture.get_editor_property("srgb") != (kind == "BaseColor"):
        raise RuntimeError("Incorrect texture color space; review existing asset: " + target)
    expected = {"Normal": unreal.TextureCompressionSettings.TC_NORMALMAP,
                "ORM": unreal.TextureCompressionSettings.TC_MASKS}.get(kind)
    if expected is not None and texture.get_editor_property("compression_settings") != expected:
        raise RuntimeError("Incorrect texture compression; review existing asset: " + target)
    if texture.get_editor_property("virtual_texture_streaming"):
        raise RuntimeError("Virtual textures require different material samplers: " + target)
    if texture.get_editor_property("never_stream"):
        unreal.log_warning("Texture is nonstreaming; review VRAM policy: " + target)


def run():
    if not (PROJECT / "MoonfallUnreal.uproject").is_file():
        raise RuntimeError("Open MoonfallUnreal first.")
    tasks = []
    for surface in NAMES:
        for kind in MAPS:
            name = "T_MF_" + surface + "_" + kind
            target = DEST + "/" + name
            if unreal.EditorAssetLibrary.does_asset_exist(target):
                texture = unreal.EditorAssetLibrary.load_asset(target)
                if not isinstance(texture, unreal.Texture2D):
                    raise RuntimeError("Unexpected existing asset: " + target)
                validate_texture(texture, kind, target)
                continue
            path = SOURCE / (name + ".png")
            if not path.is_file() or path.resolve().parent != SOURCE.resolve():
                raise RuntimeError("Generate missing source: " + str(path))
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
        # Keep ordinary sampler compatibility and allow standard mip streaming.
        texture.set_editor_property("virtual_texture_streaming", False)
        texture.set_editor_property("never_stream", False)
        texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_FROM_TEXTURE_GROUP)
        if kind == "Normal":
            texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
        elif kind == "ORM":
            texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
        validate_texture(texture, kind, target)
        if not unreal.EditorAssetLibrary.save_loaded_asset(texture, False):
            raise RuntimeError("Texture save failed: " + target)
    unreal.log("Production texture imports complete. Material graph binding is a separate step.")

if __name__ == "__main__":
    run()

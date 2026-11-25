"""Create opt-in Nanite benchmark copies of production environment meshes.

The shipped mesh references are untouched. Run manually in UE5.4 after
importing the original production pack; compare these copies in a test map.
"""
from pathlib import Path
import unreal

PROJECT = Path(unreal.Paths.project_dir()).resolve()
SOURCE = "/Game/MoonfallProduction/Meshes"
DEST = "/Game/MoonfallProduction/NanitePreview"
NAMES = ("SM_MF_GothicColumn", "SM_MF_LunarReliquary")

def run():
    if not (PROJECT / "MoonfallUnreal.uproject").is_file():
        raise RuntimeError("Open MoonfallUnreal first.")
    editor = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    if editor is None:
        raise RuntimeError("Static mesh editor subsystem unavailable.")
    sources = []
    for name in NAMES:
        source = SOURCE + "/" + name
        target = DEST + "/" + name + "_Nanite"
        mesh = unreal.EditorAssetLibrary.load_asset(source)
        if not isinstance(mesh, unreal.StaticMesh):
            raise RuntimeError("Missing imported source mesh: " + source)
        if unreal.EditorAssetLibrary.does_asset_exist(target):
            raise RuntimeError("Benchmark already exists; refusing to overwrite " + target)
        sources.append((source, target))
    for source, target in sources:
        mesh = unreal.EditorAssetLibrary.duplicate_asset(source, target)
        if not isinstance(mesh, unreal.StaticMesh):
            raise RuntimeError("Could not duplicate " + source)
        settings = editor.get_nanite_settings(mesh)
        settings.set_editor_property("enabled", True)
        settings.set_editor_property("keep_percent_triangles", 1.0)
        settings.set_editor_property("trim_relative_error", 0.0)
        settings.set_editor_property("fallback_target", unreal.NaniteFallbackTarget.PERCENT_TRIANGLES)
        settings.set_editor_property("fallback_percent_triangles", 1.0)
        editor.set_nanite_settings(mesh, settings, True)
        result = editor.get_nanite_settings(mesh)
        if not result.get_editor_property("enabled"):
            raise RuntimeError("Nanite settings not applied: " + target)
        if not unreal.EditorAssetLibrary.save_loaded_asset(mesh, False):
            raise RuntimeError("Could not save " + target)
        unreal.log("Benchmark copy created: " + target)
    unreal.log("Compare conventional and Nanite copies using GPU timing and visual inspection. No runtime assets were replaced.")

if __name__ == "__main__":
    run()

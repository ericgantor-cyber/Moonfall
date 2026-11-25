"""Run in the MoonfallUnreal UE5 editor, not the comparison project.
Imports only the allowlisted production source meshes into a new namespace.
Existing packages are never replaced, and no hero asset is touched.
"""
import json
from pathlib import Path
import unreal

PROJECT = Path(unreal.Paths.project_dir()).resolve()
SOURCE = PROJECT / "Art" / "Production" / "Meshes"
DEST = "/Game/MoonfallProduction/Meshes"
NAMES = {"SM_MF_FirstlightSword", "SM_MF_LunarReliquary", "SM_MF_GothicColumn"}

def run():
    if not (PROJECT / "MoonfallUnreal.uproject").is_file():
        raise RuntimeError("Open MoonfallUnreal, not the comparison project.")
    manifest = json.loads((SOURCE.parent / "manifest.json").read_text(encoding="utf-8"))
    assets = manifest["assets"]
    if manifest["destination"] != DEST or len(assets) != len(NAMES) or {a["name"] for a in assets} != NAMES:
        raise RuntimeError("Unexpected production art manifest.")
    tasks = []
    for asset in assets:
        name = asset["name"]
        if asset["file"] != name + ".obj":
            raise RuntimeError("Unexpected source filename.")
        path = SOURCE / asset["file"]
        if not path.is_file() or path.resolve().parent != SOURCE.resolve():
            raise RuntimeError("Missing or unsafe source: " + str(path))
        target = DEST + "/" + name
        if unreal.EditorAssetLibrary.does_asset_exist(target):
            unreal.log_warning("Existing asset left unchanged: " + target)
            continue
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", str(path))
        task.set_editor_property("destination_path", DEST)
        task.set_editor_property("destination_name", name)
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", False)
        task.set_editor_property("save", True)
        options = unreal.FbxImportUI()
        options.set_editor_property("import_as_skeletal", False)
        options.set_editor_property("import_mesh", True)
        options.set_editor_property("import_materials", False)
        options.set_editor_property("import_textures", False)
        options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
        data = options.get_editor_property("static_mesh_import_data")
        data.set_editor_property("combine_meshes", True)
        data.set_editor_property("auto_generate_collision", False)
        data.set_editor_property("generate_lightmap_u_vs", True)
        task.set_editor_property("options", options)
        tasks.append((target, task))
    if tasks:
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t for _, t in tasks])
    for target, task in tasks:
        paths = task.get_editor_property("imported_object_paths")
        mesh = unreal.EditorAssetLibrary.load_asset(target)
        if not paths or not isinstance(mesh, unreal.StaticMesh):
            raise RuntimeError("Static mesh import not verified: " + target)
    for name in NAMES:
        if not isinstance(unreal.EditorAssetLibrary.load_asset(DEST + "/" + name), unreal.StaticMesh):
            raise RuntimeError("Missing imported static mesh: " + name)
    unreal.log("Production source mesh pack verified in editor. Runtime binding and cooking are separate.")

if __name__ == "__main__":
    run()

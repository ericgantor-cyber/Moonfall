"""Unreal Editor Python: import copied Moonfall SourceArt as Unreal Content assets.

Requires Python Editor Script Plugin, with the experiment project open in UE5.
Run: Tools > Execute Python Script > Content/Python/import_moonfall_art.py
This NEVER edits original source images or Godot asset references.
"""
import unreal
from pathlib import Path
import json
import hashlib

PROJECT = Path(unreal.Paths.project_dir()).resolve()
SOURCE = PROJECT / "SourceArt" / "MoonfallArt"
MANIFEST = PROJECT.parent.parent / "ART_IMPORT_MANIFEST.json"
ROOT = "/Game/MoonfallArt"
NEW_SOURCE = PROJECT / "SourceArt" / "MoonfallGeneratedArt"
NEW_ROOT = "/Game/MoonfallGeneratedArt"

def import_task(tools, path, destination):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(path))
    task.set_editor_property("destination_path", destination)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", False)
    task.set_editor_property("save", True)
    tools.import_asset_tasks([task])
    return bool(task.get_editor_property("imported_object_paths"))


def run():
    if not SOURCE.is_dir():
        raise RuntimeError("No copied SourceArt found. Run import_godot_art.py first.")
    if not MANIFEST.is_file():
        raise RuntimeError("Missing checksum manifest.")
    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    success, unsupported, errors = 0, [], []
    for entry in manifest["assets"]:
        output = next(o for o in entry["outputs"] if o["engine"] == "unreal_source_art")
        path = PROJECT / Path(output["path"]).relative_to(
            "engine-comparison/unreal/MoonfallPrototype")
        if not path.is_file():
            errors.append("Missing " + str(path))
            continue
        if path.suffix.lower() not in {".png", ".jpg", ".jpeg", ".bmp", ".tga",
                                       ".hdr", ".exr", ".fbx", ".obj"}:
            unsupported.append(str(path))
            continue
        parent = path.parent.relative_to(SOURCE).as_posix()
        target = ROOT if parent == "." else ROOT + "/" + parent
        try:
            if import_task(tools, path, target):
                success += 1
            else:
                errors.append("No imported object for " + str(path))
        except Exception as exc:
            errors.append(str(path) + ": " + str(exc))

    # Two optional new AI environment images, packaged outside this source branch.
    # The images are never generated here and original Z/Nyra files are untouched.
    generated = ("moonfall_ai_moonlit_gate.jpg", "moonfall_ai_waterfall_ruins.jpg")
    generated_count = 0
    for name in generated:
        path = NEW_SOURCE / name
        if not path.is_file():
            unreal.log_warning("Optional generated art missing: " + str(path))
            continue
        if path.stat().st_size < 1024:
            errors.append("Generated artwork too small: " + str(path))
            continue
        try:
            if import_task(tools, path, NEW_ROOT):
                generated_count += 1
                unreal.log("Imported generated art: {} sha256={}".format(
                    name, hashlib.sha256(path.read_bytes()).hexdigest()))
            else:
                errors.append("No imported generated art object for " + str(path))
        except Exception as exc:
            errors.append(str(path) + ": " + str(exc))
    unreal.log("Moonfall art: existing {}, generated {}, unsupported {}, errors {}".format(
        success, generated_count, len(unsupported), len(errors)))
    for path in unsupported:
        unreal.log_warning("Manual importer required: " + path)
    for err in errors:
        unreal.log_error(err)
    if errors:
        raise RuntimeError("{} art imports failed, inspect Unreal Output Log".format(len(errors)))
    return success

if __name__ == "__main__":
    run()

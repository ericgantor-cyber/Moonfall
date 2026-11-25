"""UE5 editor-only material installer; no runtime generation or hero edits.

Creates five PBR-parameter materials and fills default/unassigned slots on
the six allowlisted production meshes. Existing authored materials are kept.
Run after importing the production meshes.
"""
import json
import math
from pathlib import Path
import unreal

PROJECT = Path(unreal.Paths.project_dir()).resolve()
MESH_ROOT = "/Game/MoonfallProduction/Meshes"
MATERIAL_ROOT = "/Game/MoonfallProduction/Materials"
SLOTS = {"bronze", "leather", "steel", "stone", "moonstone"}
MESH_NAMES = {"SM_MF_FirstlightSword", "SM_MF_FirstlightCuirass",
              "SM_MF_FirstlightShield", "SM_MF_EclipseAmulet",
              "SM_MF_LunarReliquary", "SM_MF_GothicColumn"}
DEFAULT = "/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial"

def load_profiles():
    profiles = json.loads((PROJECT / "Art/Production/materials.json").read_text(encoding="utf-8"))
    if profiles["schema"] != 1 or set(profiles["materials"]) != SLOTS:
        raise RuntimeError("Unexpected production material catalog")
    for profile in profiles["materials"].values():
        color = profile["base_color"]
        values = list(color) + [profile["metallic"], profile["roughness"]]
        if len(color) != 3 or not all(math.isfinite(v) and 0 <= v <= 1 for v in values):
            raise RuntimeError("Invalid production material parameters")
    return profiles["materials"]

def create_material(slot, profile):
    name = "M_MF_" + slot.title()
    target = MATERIAL_ROOT + "/" + name
    if unreal.EditorAssetLibrary.does_asset_exist(target):
        mat = unreal.EditorAssetLibrary.load_asset(target)
        if not isinstance(mat, unreal.Material):
            raise RuntimeError("Unexpected existing material: " + target)
        return mat
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name, MATERIAL_ROOT, unreal.Material, unreal.MaterialFactoryNew())
    if not isinstance(mat, unreal.Material):
        raise RuntimeError("Failed to create " + target)
    lib = unreal.MaterialEditingLibrary
    color = lib.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -350, -160)
    color.set_editor_property("constant", unreal.LinearColor(*profile["base_color"], 1.0))
    if not lib.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR):
        raise RuntimeError("Could not connect Base Color: " + target)
    for idx, (field, prop) in enumerate((
        ("metallic", unreal.MaterialProperty.MP_METALLIC),
        ("roughness", unreal.MaterialProperty.MP_ROUGHNESS))):
        node = lib.create_material_expression(mat, unreal.MaterialExpressionConstant, -350, idx * 120)
        node.set_editor_property("r", float(profile[field]))
        if not lib.connect_material_property(node, "", prop):
            raise RuntimeError("Could not connect " + field + ": " + target)
    lib.recompile_material(mat)
    if not unreal.EditorAssetLibrary.save_loaded_asset(mat, False):
        raise RuntimeError("Could not save " + target)
    return mat

def run():
    if not (PROJECT / "MoonfallUnreal.uproject").is_file():
        raise RuntimeError("Open MoonfallUnreal first.")
    profiles = load_profiles()
    # Resolve and validate every expected slot before modifying a mesh.
    assignments = []
    for name in sorted(MESH_NAMES):
        mesh = unreal.EditorAssetLibrary.load_asset(MESH_ROOT + "/" + name)
        if not isinstance(mesh, unreal.StaticMesh):
            raise RuntimeError("Import the mesh pack first: " + name)
        source = PROJECT / "Art/Production/Meshes" / (name + ".obj")
        expected = {line.split()[1] for line in source.read_text(encoding="utf-8").splitlines()
                    if line.startswith("usemtl ")}
        if not expected or not expected <= SLOTS:
            raise RuntimeError("Unexpected source slots: " + name)
        found = set()
        for index, slot in enumerate(mesh.get_editor_property("static_materials")):
            key = str(slot.get_editor_property("material_slot_name")).lower()
            if key not in expected:
                raise RuntimeError("Unrecognized imported slot in " + name + ": " + key)
            found.add(key)
            current = slot.get_editor_property("material_interface")
            if current is None or current.get_path_name() == DEFAULT:
                assignments.append((mesh, index, key))
        if found != expected:
            raise RuntimeError("Imported material slots differ from source: " + name)
    materials = {slot: create_material(slot, profiles[slot]) for slot in sorted(SLOTS)}
    changed = {}
    for mesh, index, slot in assignments:
        mesh.set_material(index, materials[slot])
        changed[mesh.get_path_name()] = mesh
    for mesh in changed.values():
        if not unreal.EditorAssetLibrary.save_loaded_asset(mesh, False):
            raise RuntimeError("Could not save " + mesh.get_path_name())
    unreal.log("Production PBR constants installed; {} default slots assigned. Existing authored slots preserved.".format(len(assignments)))

if __name__ == "__main__":
    run()

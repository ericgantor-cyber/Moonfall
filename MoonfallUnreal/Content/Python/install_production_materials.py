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

TEXTURE_ROOT = "/Game/MoonfallProduction/Textures"

def load_texture_pack():
    textures = {}
    kinds = ("BaseColor", "Normal", "ORM")
    for slot in sorted(SLOTS):
        for kind in kinds:
            name = "T_MF_" + slot.title() + "_" + kind
            target = TEXTURE_ROOT + "/" + name
            if not unreal.EditorAssetLibrary.does_asset_exist(target):
                continue
            texture = unreal.EditorAssetLibrary.load_asset(target)
            if not isinstance(texture, unreal.Texture2D):
                raise RuntimeError("Unexpected texture asset: " + target)
            if texture.get_editor_property("srgb") != (kind == "BaseColor"):
                raise RuntimeError("Incorrect texture color space: " + target)
            expected = {"Normal": unreal.TextureCompressionSettings.TC_NORMALMAP,
                        "ORM": unreal.TextureCompressionSettings.TC_MASKS}.get(kind)
            if expected is not None and texture.get_editor_property("compression_settings") != expected:
                raise RuntimeError("Incorrect texture compression: " + target)
            textures[(slot, kind)] = texture
    if textures and len(textures) != len(SLOTS)*len(kinds):
        raise RuntimeError("Import all fifteen production textures before installing textured materials.")
    return textures

def connect_textures(lib, mat, slot, textures):
    samplers = {
        "BaseColor": unreal.MaterialSamplerType.SAMPLERTYPE_COLOR,
        "Normal": unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL,
        "ORM": unreal.MaterialSamplerType.SAMPLERTYPE_MASKS,
    }
    nodes = {}
    for idx, kind in enumerate(("BaseColor", "Normal", "ORM")):
        node = lib.create_material_expression(mat, unreal.MaterialExpressionTextureSample, -450, idx*260)
        node.set_editor_property("texture", textures[(slot, kind)])
        node.set_editor_property("sampler_type", samplers[kind])
        nodes[kind] = node
    for kind, channel, prop in (
        ("BaseColor", "RGB", unreal.MaterialProperty.MP_BASE_COLOR),
        ("Normal", "RGB", unreal.MaterialProperty.MP_NORMAL),
        ("ORM", "R", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION),
        ("ORM", "G", unreal.MaterialProperty.MP_ROUGHNESS),
        ("ORM", "B", unreal.MaterialProperty.MP_METALLIC),
    ):
        if not lib.connect_material_property(nodes[kind], channel, prop):
            raise RuntimeError("Could not bind " + kind + "/" + channel + " for " + slot)

def create_material(slot, profile, textures):
    name = "M_MF_" + slot.title() + ("_Textured" if textures else "")
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
    if textures:
        connect_textures(lib, mat, slot, textures)
    else:
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
    textures = load_texture_pack()
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
            # Replace only empty/default slots or our exact previous constant
            # material reference; never edit an existing material package.
            previous_name = "M_MF_" + key.title()
            previous = MATERIAL_ROOT + "/" + previous_name + "." + previous_name
            if current is None or current.get_path_name() == DEFAULT:
                assignments.append((mesh, index, key))
            elif textures and current.get_path_name() == previous:
                assignments.append((mesh, index, key))
        if found != expected:
            raise RuntimeError("Imported material slots differ from source: " + name)
    materials = {slot: create_material(slot, profiles[slot], textures) for slot in sorted(SLOTS)}
    changed = {}
    for mesh, index, slot in assignments:
        mesh.set_material(index, materials[slot])
        changed[mesh.get_path_name()] = mesh
    for mesh in changed.values():
        if not unreal.EditorAssetLibrary.save_loaded_asset(mesh, False):
            raise RuntimeError("Could not save " + mesh.get_path_name())
    unreal.log("Production {} PBR materials installed; {} slots assigned. Existing custom slots preserved.".format("textured" if textures else "constant", len(assignments)))

if __name__ == "__main__":
    run()

"""Create the preview-only water material in UE5.4, without overwriting assets."""
PACKAGE = "/Game/DeepWaters/Materials"
NAME = "M_DeepWatersSurface"


def main():
    import unreal
    path = PACKAGE + "/" + NAME
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.log("Water material already exists; leaving it untouched.")
        return
    unreal.EditorAssetLibrary.make_directory(PACKAGE)
    factory = unreal.MaterialFactoryNew()
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        NAME, PACKAGE, unreal.Material, factory
    )
    if mat is None:
        raise RuntimeError("Could not create preview water material")
    try:
        mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
        mat.set_editor_property("two_sided", True)
        mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
        edit = unreal.MaterialEditingLibrary
        color = edit.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -300, -100)
        color.set_editor_property("constant", unreal.LinearColor(0.045, 0.27, 0.32, 1.0))
        alpha = edit.create_material_expression(mat, unreal.MaterialExpressionConstant, -300, 80)
        alpha.set_editor_property("r", 0.23)
        if not edit.connect_material_property(color, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR):
            raise RuntimeError("Water color connection failed")
        if not edit.connect_material_property(alpha, "", unreal.MaterialProperty.MP_OPACITY):
            raise RuntimeError("Water opacity connection failed")
        edit.recompile_material(mat)
        if not unreal.EditorAssetLibrary.save_loaded_asset(mat):
            raise RuntimeError("Could not save new water material")
    except Exception:
        unreal.log_error("Incomplete new material: inspect or remove it before retrying.")
        raise


if __name__ == "__main__":
    main()

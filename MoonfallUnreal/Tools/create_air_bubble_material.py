"""Create and apply a preview-only translucent bubble material in UE5.4.

Run after compiling the bubble-marker actor. The new material is saved,
but the level is never saved automatically. Existing assets are not overwritten.
"""
PACKAGE = "/Game/DeepWaters/Materials"
NAME = "M_DeepWatersAirBubble"
MARKERS = {"AirBubbleSmall", "AirBubbleMedium", "AirBubbleLarge"}


def main():
    import unreal
    world = unreal.EditorLevelLibrary.get_editor_world()
    if world is None or not world.get_path_name().startswith("/Game/DeepWaters/Maps/DeepWatersPreview."):
        raise RuntimeError("Open only DeepWatersPreview")
    pocket = getattr(unreal, "MoonfallAirPocket", None)
    if pocket is None:
        raise RuntimeError("Compile air pocket actor first")
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    components = []
    for actor in actors.get_all_level_actors():
        if isinstance(actor, pocket):
            components.extend(c for c in actor.get_components_by_class(unreal.StaticMeshComponent)
                              if c.get_name() in MARKERS)
    if not components:
        raise RuntimeError("No compiled air bubble markers found")
    path = PACKAGE + "/" + NAME
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        mat = unreal.EditorAssetLibrary.load_asset(path)
        if not isinstance(mat, unreal.Material):
            raise RuntimeError("Existing bubble asset is not a material")
    else:
        unreal.EditorAssetLibrary.make_directory(PACKAGE)
        mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            NAME, PACKAGE, unreal.Material, unreal.MaterialFactoryNew())
        if mat is None:
            raise RuntimeError("Could not create bubble material")
        try:
            mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
            mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
            edit = unreal.MaterialEditingLibrary
            color = edit.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -420, -140)
            color.set_editor_property("constant", unreal.LinearColor(.22, .82, 1.0, 1.0))
            rim = edit.create_material_expression(mat, unreal.MaterialExpressionFresnel, -420, 80)
            rim.set_editor_property("exponent", 3.0)
            rim.set_editor_property("base_reflect_fraction", .08)
            if not edit.connect_material_property(color, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR):
                raise RuntimeError("Bubble color connection failed")
            if not edit.connect_material_property(rim, "", unreal.MaterialProperty.MP_OPACITY):
                raise RuntimeError("Bubble rim connection failed")
            edit.recompile_material(mat)
            if not unreal.EditorAssetLibrary.save_loaded_asset(mat):
                raise RuntimeError("Could not save bubble material")
        except Exception:
            unreal.log_error("Incomplete new bubble material; inspect before retrying.")
            raise
    with unreal.ScopedEditorTransaction("Apply underwater bubble material"):
        for component in components:
            component.modify()
            component.set_material(0, mat)
    unreal.log(f"Applied translucent bubble material to {len(components)} markers. Inspect underwater visibility and save level manually.")


if __name__ == "__main__":
    main()

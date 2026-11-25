"""Opt-in Lumen and cinematic grading for the isolated Northern preview.
Run inside UE5.4. Never modifies the campaign or original hero assets.
The level remains unsaved until an artist reviews and saves it explicitly.
"""
import unreal

MAP = "/Game/NorthernExpanse/Maps/NorthernExpansePreview"
TAG = unreal.Name("MoonfallCinematicPreview")
LABEL = "Moonfall Cinematic Preview"

def run():
    world = unreal.EditorLevelLibrary.get_editor_world()
    if not world or world.get_path_name().split(".")[0] != MAP:
        raise RuntimeError("Open the isolated Northern Expanse preview first.")
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if actors is None:
        raise RuntimeError("Editor actor subsystem unavailable.")
    if any(TAG in a.get_editor_property("tags") for a in actors.get_all_level_actors()):
        raise RuntimeError("Cinematic preview already exists; existing actors preserved.")
    settings = unreal.PostProcessSettings()
    values = {
        "dynamic_global_illumination_method": unreal.DynamicGlobalIlluminationMethod.LUMEN,
        "reflection_method": unreal.ReflectionMethod.LUMEN,
        "bloom_intensity": 0.35,
        "vignette_intensity": 0.12,
        "motion_blur_amount": 0.0,
        "scene_fringe_intensity": 0.0,
        "lens_flare_intensity": 0.0,
        "film_grain_intensity": 0.0,
    }
    # Validate the complete property set before placing the volume.
    for name, value in values.items():
        settings.set_editor_property("override_" + name, True)
        settings.set_editor_property(name, value)
    with unreal.ScopedEditorTransaction("Install Moonfall cinematic preview"):
        volume = actors.spawn_actor_from_class(
            unreal.PostProcessVolume, unreal.Vector(0,0,0), unreal.Rotator(0,0,0))
        if not isinstance(volume, unreal.PostProcessVolume):
            raise RuntimeError("Could not place cinematic volume.")
        volume.set_actor_label(LABEL)
        volume.set_editor_property("tags", [TAG])
        volume.set_editor_property("unbound", True)
        volume.set_editor_property("priority", 10.0)
        volume.set_editor_property("blend_weight", 1.0)
        volume.set_editor_property("settings", settings)
    unreal.log("Unsaved cinematic preview created. Review low/high scalability, shader readiness and combat readability before saving.")

if __name__ == "__main__":
    run()

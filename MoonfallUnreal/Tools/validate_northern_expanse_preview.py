"""Read-only Unreal Editor 5.4 validation of the isolated Northern Expanse map.

Run via Tools > Execute Python Script after running create_northern_expanse_preview.py.
Uses editor scene inspection only. This cannot prove PIE movement, weather timing,
collision traversal, frame rate, or controller support, which need human/native QA.
"""
import unreal

MAP = "/Game/NorthernExpanse/Maps/NorthernExpansePreview"
EXPECTED_PROVINCES = 9


def require(condition, message):
    if not condition:
        raise RuntimeError("NORTHERN PREVIEW FAIL: " + message)


def main():
    require(unreal.EditorAssetLibrary.does_asset_exist(MAP),
            "The actual .umap asset has not been generated at " + MAP)
    level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    require(level is not None, "Level Editor subsystem unavailable")
    require(level.load_level(MAP), "Could not open the saved preview map")
    world = unreal.EditorLevelLibrary.get_editor_world()
    require(world is not None, "Editor world unavailable")
    settings = world.get_world_settings()
    require(settings is not None, "World settings unavailable")
    override = settings.get_editor_property("default_game_mode")
    require(override is not None and "MoonfallNorthPreviewGameMode" in str(override),
            "Preview must use isolated game mode to avoid legacy arena spawning")

    actors = unreal.EditorLevelLibrary.get_all_level_actors()
    required = {
        "MoonfallNorthContinentActor": 0,
        "MoonfallNorthWeatherActor": 0,
        "PlayerStart": 0,
    }
    forbidden = ("MoonfallArena", "MoonfallDungeonLayout", "Morvella", "Nyra", "VoidReaper")
    continent = None
    weather = None
    for actor in actors:
        name = actor.get_class().get_name()
        for key in required:
            if key == name:
                required[key] += 1
        if name == "MoonfallNorthContinentActor":
            continent = actor
        if name == "MoonfallNorthWeatherActor":
            weather = actor
        require(not any(token in name for token in forbidden),
                "Preview contains unexpected legacy or divine actor " + name)
    for key, count in required.items():
        require(count == 1, "Expected one %s, found %d" % (key, count))

    pads = continent.get_editor_property("province_pads")
    roads = continent.get_editor_property("province_roads")
    landmarks = continent.get_editor_property("province_landmarks")
    overlays = continent.get_editor_property("biome_surface_overlays")
    require(pads.get_instance_count() == EXPECTED_PROVINCES,
            "Nine continent pads were not constructed")
    require(roads.get_instance_count() >= EXPECTED_PROVINCES - 1,
            "Connected roads were not built")
    require(landmarks.get_instance_count() == EXPECTED_PROVINCES * 2,
            "Expected two landmark instances per biome")
    require(len(overlays) == EXPECTED_PROVINCES,
            "Expected one visual overlay per biome")
    require(weather.get_editor_property("b_follow_player_province"),
            "Player-following weather is disabled")
    require(weather.get_editor_property("b_enable_weather_preview"),
            "Weather preview is disabled")

    unreal.log("NORTHERN PREVIEW EDITOR INSPECTION PASS")
    unreal.log("Map=%s, pads=%d, roads=%d, landmarks=%d, overlays=%d"
               % (MAP, pads.get_instance_count(), roads.get_instance_count(),
                  landmarks.get_instance_count(), len(overlays)))
    unreal.log("PIE movement/collision/weather/controller still require native testing")


if __name__ == "__main__":
    main()

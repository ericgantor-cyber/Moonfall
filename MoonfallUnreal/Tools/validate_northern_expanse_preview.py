"""Read-only Unreal Editor 5.4 validation of the isolated Northern Expanse map.

Run via Tools > Execute Python Script after running create_northern_expanse_preview.py.
Uses editor scene inspection only. This cannot prove PIE movement, weather timing,
collision traversal, frame rate, or controller support, which need human/native QA.
"""
import unreal
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from shiverwake_shrine_layout import (
    SHRINE_SITES, SHRINE_ZONE_BY_ID, SHRINES_PER_ZONE, ZONE_NAMES,
    LEGACY_SHIVERWAKE_SITES, validate_shrine_sites,
)

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

    # Two explicit, reciprocal harbors in the isolated test map.
    gates = [actor for actor in actors
             if actor.get_class().get_name() == "MoonfallNorthFerryGate"]
    docks = [actor for actor in actors
             if actor.get_actor_label() == "Mainland Harbor Test Dock"]
    require(len(gates) == 2, "Expected two linked Northern Expanse ferry gates")
    require(len(docks) == 1, "Expected exactly one walkable mainland harbor dock")
    mainland = [gate for gate in gates
                if not gate.get_editor_property("b_island_harbor")]
    island = [gate for gate in gates
              if gate.get_editor_property("b_island_harbor")]
    require(len(mainland) == 1 and len(island) == 1,
            "Ferry gates must contain one mainland and one island endpoint")
    mainland, island = mainland[0], island[0]
    require(mainland.get_editor_property("paired_gate") == island
            and island.get_editor_property("paired_gate") == mainland,
            "Ferry gates must be mutually paired")
    require(mainland.get_editor_property("b_enable_sea_passage")
            and island.get_editor_property("b_enable_sea_passage"),
            "Preview ferry gates must be explicitly enabled")
    require(abs(mainland.get_actor_location().y + 10000.0) < 100.0,
            "Mainland harbor deck is no longer near the QA dock")
    require(abs(island.get_actor_location().y + 1200.0) < 100.0,
            "Island ferry gate must remain at Shiverwake Coast")
    require(abs(mainland.get_actor_location().y
                - island.get_actor_location().y) > 6000.0,
            "Island and mainland harbors need a separate sea gap")

    # Two gothic boat silhouettes are purely decorative, nonblocking, and
    # placed out over the sea rather than on either safe landing marker.
    ships = [actor for actor in actors
             if actor.get_class().get_name() == "MoonfallNorthGothicFerryActor"]
    require(len(ships) == 2, "Expected mainland and Shiverwake gothic ferries")
    for ship in ships:
        require(ship.get_editor_property("b_show_gothic_ferry"),
                "The gothic ferry silhouette is disabled")
        # Ship render instances remain decorative. Only a dedicated movable
        # walking floor and safety rails block the player capsule.
        passenger_deck = ship.get_editor_property("passenger_deck")
        boarding_marker = ship.get_editor_property("passenger_boarding_marker")
        require(passenger_deck is not None and boarding_marker is not None,
                "Gothic ferry is missing the walkable passenger deck or boarding marker")
        require(passenger_deck.get_collision_enabled()
                == unreal.CollisionEnabled.QUERY_AND_PHYSICS,
                "Passenger deck collision is not active")
        # Unreal C++ constructor sets ECC_Pawn to Block; verify the actual
        # pawn response in a native Editor/PIE collision acceptance test.
        for rail_name in ("port_passenger_rail", "starboard_passenger_rail",
                          "bow_passenger_rail", "stern_passenger_rail"):
            rail = ship.get_editor_property(rail_name)
            require(rail is not None and rail.get_collision_enabled()
                    == unreal.CollisionEnabled.QUERY_AND_PHYSICS,
                    "Passenger safety railing missing pawn collision: " + rail_name)
        for part in ("hull_pieces", "gothic_ironwork", "crimson_sails",
                     "raven_figurehead", "crimson_lanterns"):
            mesh = ship.get_editor_property(part)
            require(mesh is not None and mesh.get_instance_count() >= 2,
                    "Missing gothic ship details: " + part)
            require(mesh.get_collision_enabled() == unreal.CollisionEnabled.NO_COLLISION,
                    "Gothic ship must not block safe ferry travel: " + part)
    sea_y_positions = sorted([ship.get_actor_location().y for ship in ships])
    require(abs(sea_y_positions[0] + 7650.0) < 100.0
            and abs(sea_y_positions[1] + 2750.0) < 100.0,
            "Gothic ships must remain offshore at the two test harbors")
    mainland_ship = next(ship for ship in ships if ship.get_actor_label()
                         == "Mainland Gothic Raven Ferry")
    island_ship = next(ship for ship in ships if ship.get_actor_label()
                       == "Shiverwake Gothic Raven Ferry")
    require(mainland.get_editor_property("sailing_vessel") == mainland_ship
            and island.get_editor_property("sailing_vessel") == island_ship,
            "Both automatic ferry routes need their own offshore visual ship")
    for gate in (mainland, island):
        require(not gate.get_editor_property("b_use_voyage_camera"),
                "Walkable ferry preview should use the regular hero camera")
        require(abs(gate.get_editor_property("voyage_duration_seconds") - 11.0) < 0.01,
                "The test crossing should take eleven seconds")
        require(not gate.get_editor_property("b_auto_voyage_in_progress"),
                "Editor preview gate should not start sailing before PIE")

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
    # Exactly two checkpoints on each of the nine Northern provinces.
    # Verify ACTUAL saved editor actors, not only the Python manifest.
    validate_shrine_sites()
    expected_shrines = {site[0]: site for site in SHRINE_SITES}
    shrine_actors = [actor for actor in actors
                     if actor.get_class().get_name() == "MoonfallCheckpointShrine"]
    found = {}
    for shrine in shrine_actors:
        shrine_id = str(shrine.get_editor_property("shrine_id"))
        if shrine_id not in expected_shrines:
            continue  # Preserve independently authored shrine actors.
        require(shrine_id not in found, "Duplicate Shiverwake shrine ID: " + shrine_id)
        found[shrine_id] = shrine
    require(len(found) == 18 and set(found) == set(expected_shrines),
            "Expected 18 unique Northern shrines, exactly two in each of nine zones")
    legacy_ids = {item[0] for item in LEGACY_SHIVERWAKE_SITES[2:]}
    require(not legacy_ids.intersection(
        str(actor.get_editor_property("shrine_id")) for actor in shrine_actors),
        "Old all-on-Shiverwake generated shrines require safe migration")
    per_zone = [0] * len(ZONE_NAMES)
    for shrine_id, (_, display, location) in expected_shrines.items():
        shrine = found[shrine_id]
        per_zone[SHRINE_ZONE_BY_ID[shrine_id]] += 1
        pos = shrine.get_actor_location()
        require((pos.x - location[0]) ** 2 + (pos.y - location[1]) ** 2
                + (pos.z - location[2]) ** 2 < 100.0,
                "Shiverwake shrine is not on its intended grounded pad: " + shrine_id)
        require(shrine.get_actor_label() == display,
                "Shiverwake shrine needs its own readable name: " + shrine_id)
        require(str(shrine.get_editor_property("map_display_name")) == display,
                "Shiverwake shrine map title is missing: " + shrine_id)
        require(shrine.get_editor_property("discovery_range") is not None,
                "Shiverwake shrine has no discoverable checkpoint trigger: " + shrine_id)

    require(per_zone == [SHRINES_PER_ZONE] * 9,
            "Each Northern Expanse province must have exactly two shrines")

    require(weather.get_editor_property("b_follow_player_province"),
            "Player-following weather is disabled")
    require(weather.get_editor_property("b_enable_weather_preview"),
            "Weather preview is disabled")

    unreal.log("NORTHERN PREVIEW EDITOR INSPECTION PASS")
    unreal.log("Map=%s, pads=%d, roads=%d, landmarks=%d, overlays=%d, shrines=%d"
               % (MAP, pads.get_instance_count(), roads.get_instance_count(),
                  landmarks.get_instance_count(), len(overlays), len(found)))
    unreal.log("PIE movement/collision/weather/controller still require native testing")


if __name__ == "__main__":
    main()

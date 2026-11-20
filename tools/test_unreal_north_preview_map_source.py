"""Static, editor-independent safety checks for the Northern Expanse map builder.

Does not prove that UE5.4 compiled the actors or generated a .umap.
"""
import ast
from pathlib import Path

script = Path(__file__).resolve().parents[1] / "MoonfallUnreal" / "Tools" / "create_northern_expanse_preview.py"
source = script.read_text(encoding="utf-8")
ast.parse(source, filename=str(script))
validator = script.with_name("validate_northern_expanse_preview.py")
validation_source = validator.read_text(encoding="utf-8")
ast.parse(validation_source, filename=str(validator))
game_mode_header = script.parents[1] / "Source" / "MoonfallUnreal" / "MoonfallNorthPreviewGameMode.h"
game_mode_source = game_mode_header.with_suffix(".cpp")
mode_h = game_mode_header.read_text(encoding="utf-8")
mode_cpp = game_mode_source.read_text(encoding="utf-8")
assert "public AGameModeBase" in mode_h
assert "DefaultPawnClass=AMoonfallHeroPawn::StaticClass();" in mode_cpp
assert "SpawnActor" not in mode_cpp


assert 'LEVEL_PATH = "/Game/NorthernExpanse/Maps/NorthernExpansePreview"' in source
assert "if unreal.EditorAssetLibrary.does_asset_exist(LEVEL_PATH):" in source
assert "refusing to overwrite it" in source
assert 'required_class("MoonfallNorthContinentActor")' in source
assert 'required_class("MoonfallNorthWeatherActor")' in source
assert "level_subsystem.new_level(LEVEL_PATH)" in source
assert "actor_subsystem.spawn_actor_from_class(" in source
assert '"b_follow_player_province", True' in source
assert '"b_enable_weather_preview", True' in source
assert "unreal.PlayerStart" in source
assert "unreal.DirectionalLight" in source
assert "level_subsystem.save_current_level()" in source
assert 'required_class("MoonfallNorthPreviewGameMode")' in source
assert 'settings.set_editor_property("default_game_mode", preview_game_mode)' in source
assert 'settings.get_editor_property("default_game_mode") != preview_game_mode' in source
assert 'unreal.EditorAssetLibrary.does_asset_exist(MAP)' in validation_source
assert 'level.load_level(MAP)' in validation_source
assert '"MoonfallNorthPreviewGameMode" in str(override)' in validation_source
assert 'pads.get_instance_count() == EXPECTED_PROVINCES' in validation_source
assert 'roads.get_instance_count() >= EXPECTED_PROVINCES - 1' in validation_source
assert 'landmarks.get_instance_count() == EXPECTED_PROVINCES * 2' in validation_source
assert 'len(overlays) == EXPECTED_PROVINCES' in validation_source
assert 'PIE movement/collision/weather/controller still require native testing' in validation_source


# Isolated editor tool may create a single map, but never deletes or edits
# a preexisting map, opens save files, or spawns divine encounter actors.
for forbidden in (
    "delete_asset(", "delete_directory(", "overwrite=True",
    "SaveGameToSlot", "LoadGameFromSlot", "SpawnMorvellaWorldEncounter",
    "MorvellaActor", "NyraActor", "ZActor", "set_project_setting(",
    "EditorLoadingAndSavingUtils.load_map(", "set_game_default_map(",
):
    assert forbidden not in source, forbidden

print("Northern Expanse editor map script syntax and safety checks passed")

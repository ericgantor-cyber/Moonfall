"""Static, editor-independent safety checks for the Northern Expanse map builder.

Does not prove that UE5.4 compiled the actors or generated a .umap.
"""
import ast
from pathlib import Path

script = Path(__file__).resolve().parents[1] / "MoonfallUnreal" / "Tools" / "create_northern_expanse_preview.py"
source = script.read_text(encoding="utf-8")
ast.parse(source, filename=str(script))

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

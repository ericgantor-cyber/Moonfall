"""Create a minimal, project-owned Moonfall startup map in Unreal Editor 5.4.

Run in the project's Unreal Editor Python console:
  exec(open(r"C:\\path\\to\\MoonfallUnreal\\Build\\Create-StartupLevel.py", encoding="utf-8").read())

Requires Python Editor Script Plugin. Never overwrites an existing level.
The game mode creates the temporary combat arena at runtime.
"""
from pathlib import Path
import re
import unreal

MAP = "/Game/Maps/MoonfallStart"
MAP_SECTION = "[/Script/EngineSettings.GameMapsSettings]"
MAP_KEYS = ("GameDefaultMap", "EditorStartupMap")


def updated_map_config(config: str) -> str:
    """Replace only the two game-map entries in an existing game-map section."""
    newline = "\r\n" if "\r\n" in config else "\n"
    lines = config.splitlines(keepends=True)
    section_indices = [
        i for i, line in enumerate(lines) if line.strip() == MAP_SECTION
    ]
    if len(section_indices) != 1:
        raise ValueError("Expected exactly one Unreal GameMapsSettings section")

    start = section_indices[0] + 1
    end = next((i for i in range(start, len(lines))
                if lines[i].strip().startswith("[")
                and lines[i].strip().endswith("]")), len(lines))
    for key in MAP_KEYS:
        matches = [i for i in range(start, end)
                   if re.match(r"^\\s*" + key + r"\\s*=", lines[i])]
        if len(matches) != 1:
            raise ValueError("Expected exactly one " + key + " in GameMapsSettings")
        idx = matches[0]
        lines[idx] = key + "=" + MAP + newline
    return "".join(lines)


def create_moonfall_start():
    if not unreal.Paths.get_project_file_path().replace("\\", "/").endswith(
            "/MoonfallUnreal.uproject"):
        raise RuntimeError("Open MoonfallUnreal.uproject before running this script")

    project_root = Path(unreal.Paths.project_dir())
    config_path = project_root / "Config" / "DefaultEngine.ini"
    if not config_path.is_file():
        raise RuntimeError("Missing Unreal configuration: " + str(config_path))
    original_config = config_path.read_text(encoding="utf-8")
    new_config = updated_map_config(original_config)

    if unreal.EditorAssetLibrary.does_asset_exist(MAP):
        raise RuntimeError("MoonfallStart already exists; refusing to overwrite it")

    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if not levels.new_level(MAP, is_partitioned_world=False):
        raise RuntimeError("Could not create " + MAP)

    player = actors.spawn_actor_from_class(
        unreal.PlayerStart, unreal.Vector(0.0, 0.0, 130.0),
        unreal.Rotator(0.0, 0.0, 0.0))
    if player is None:
        raise RuntimeError("Could not spawn PlayerStart; inspect the unsaved map")

    nav = actors.spawn_actor_from_class(
        unreal.NavMeshBoundsVolume, unreal.Vector(0.0, 0.0, 100.0),
        unreal.Rotator(0.0, 0.0, 0.0))
    if nav is None:
        raise RuntimeError("Could not spawn NavMeshBoundsVolume; inspect the unsaved map")
    # Default brush dimensions vary. Verify and adjust navigation with P in Editor.
    nav.set_actor_scale3d(unreal.Vector(20.0, 20.0, 4.0))

    if not levels.save_current_level():
        raise RuntimeError("Could not save MoonfallStart; configuration is unchanged")

    # Only update config after the map has been successfully saved. Preserve
    # unrelated user settings and a backup if the write fails.
    backup_path = config_path.with_suffix(".ini.before-moonfall-startup")
    if new_config != original_config:
        if backup_path.exists():
            raise RuntimeError(
                "Startup level saved, but config backup already exists: " +
                str(backup_path) + ". Check it before changing configuration.")
        backup_path.write_text(original_config, encoding="utf-8")
        config_path.write_text(new_config, encoding="utf-8", newline="")

    unreal.log("Saved " + MAP + " and configured game/editor startup maps. "
               "Verify PlayerStart, navigation (P), collision, combat and HUD "
               "using Play In Editor before packaging.")


if __name__ == "__main__":
    create_moonfall_start()

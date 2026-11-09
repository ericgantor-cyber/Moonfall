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
                   if re.match(r"^\s*" + key + r"\s*=", lines[i])]
        if len(matches) != 1:
            raise ValueError("Expected exactly one " + key + " in GameMapsSettings")
        idx = matches[0]
        lines[idx] = key + "=" + MAP + newline
    return "".join(lines)


DUNGEON_LABEL = "Moonfall_RandomizedDungeon_Zone1_01"
DUNGEON_CLASS_PATH = "/Script/MoonfallUnreal.MoonfallDungeonLayout"


def install_randomized_dungeon():
    """Add one playable seeded dungeon to MoonfallStart without duplicates.

    This updates only the startup level in the currently open Unreal project.
    Native C++ must first be compiled so its dungeon class is registered.
    """
    if not unreal.EditorAssetLibrary.does_asset_exist(MAP):
        raise RuntimeError("Generate MoonfallStart before installing the dungeon")
    dungeon_class = unreal.load_class(None, DUNGEON_CLASS_PATH)
    if dungeon_class is None:
        raise RuntimeError(
            "MoonfallDungeonLayout Unreal C++ class is missing. "
            "Compile the project and restart Unreal Editor before retrying.")
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if not levels.load_level(MAP):
        raise RuntimeError("Could not load the MoonfallStart map")
    for actor in actors.get_all_level_actors():
        if actor.get_actor_label() == DUNGEON_LABEL:
            if not actor.is_a(dungeon_class):
                raise RuntimeError(
                    "Dungeon label is occupied by a different actor; refusing to modify it")
            unreal.log("Randomized dungeon already installed; existing actor preserved")
            return False

    # Place the first entrance away from the original combat spawn. The 14-room
    # block extends along +X and must be visually collision-checked in Editor.
    dungeon = actors.spawn_actor_from_class(
        dungeon_class, unreal.Vector(3500.0, 4200.0, 0.0),
        unreal.Rotator(0.0, 0.0, 0.0))
    if dungeon is None:
        raise RuntimeError("Failed to place the randomized dungeon in MoonfallStart")
    dungeon.set_actor_label(DUNGEON_LABEL)
    dungeon.set_editor_property("zone_index", 0)
    dungeon.set_editor_property("dungeon_index", 0)
    dungeon.set_editor_property("layout_seed", 90111)
    dungeon.set_editor_property("b_randomize_chambers", True)
    if not levels.save_current_level():
        raise RuntimeError("Dungeon actor was placed but MoonfallStart could not be saved")
    unreal.log("Installed randomized dungeon 1/30 in MoonfallStart: "
               "14 connected chambers, seeded traps, treasure and shrines")
    return True


def create_moonfall_start():
    # project_dir() is exposed by the 5.4 Python API. Validate the actual
    # project layout without depending on project-file-path helper availability.
    project_root = Path(unreal.Paths.project_dir()).resolve()
    if not (project_root / "MoonfallUnreal.uproject").is_file():
        raise RuntimeError("Open MoonfallUnreal.uproject before running this script")
    config_path = project_root / "Config" / "DefaultEngine.ini"
    if not config_path.is_file():
        raise RuntimeError("Missing Unreal configuration: " + str(config_path))
    original_config = config_path.read_text(encoding="utf-8")
    new_config = updated_map_config(original_config)

    # Retry-safe: a prior run may have saved the map but failed before the
    # configuration write. Keep an existing level intact and repair config only.
    map_file = project_root / "Content" / "Maps" / "MoonfallStart.umap"
    map_exists = unreal.EditorAssetLibrary.does_asset_exist(MAP)
    if map_exists:
        if not map_file.is_file() or map_file.stat().st_size <= 0:
            raise RuntimeError("MoonfallStart asset exists but its .umap is missing or empty")
        unreal.log("Using existing MoonfallStart level without overwriting it")
    else:
        if map_file.exists():
            raise RuntimeError("MoonfallStart.umap is already on disk but Unreal cannot load the asset; refusing to overwrite")

    if not map_exists:
        create_new_startup_level()

    # Configuration is updated only after a verified, saved .umap exists.
    backup_path = config_path.with_suffix(".ini.before-moonfall-startup")
    if new_config != original_config:
        if backup_path.exists():
            # An interrupted previous attempt may have written the backup but
            # not the new config. Resume only if the backup matches exactly.
            if backup_path.read_text(encoding="utf-8") != original_config:
                raise RuntimeError(
                    "Existing config backup differs from current settings: " +
                    str(backup_path) + ". Review it before changing configuration.")
        else:
            backup_path.write_text(original_config, encoding="utf-8")
        config_path.write_text(new_config, encoding="utf-8", newline="")

    # Install exactly one seeded dungeon into the real startup level, including
    # previously created maps. Never overwrite user-authored level actors.
    install_randomized_dungeon()

    unreal.log("Verified " + MAP + " and configured game/editor startup maps. "
               "Verify PlayerStart, navigation (P), collision, combat and HUD "
               "using Play In Editor before packaging.")


def create_new_startup_level():
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

if __name__ == "__main__":
    create_moonfall_start()

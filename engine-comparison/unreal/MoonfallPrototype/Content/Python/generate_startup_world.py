"""Create or validate the persistent Moonfall UE5.6 startup world.

Runs ONLY inside UnrealEditor-Cmd's Python plugin.
Never changes an existing artist-authored map. On newly created maps it adds a
PlayerStart, directional sunlight and a skylight; runtime geometry is supplied
by AMoonfallPawn. Validation is deliberately strict so cooking fails closed.
"""
import unreal

LEVEL_PATH = "/Game/Maps/MoonfallWorld"


def verify_actor(actors, actor_class):
    return any(actor and actor.is_a(actor_class) for actor in actors)


def main():
    assets = unreal.EditorAssetLibrary
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    existing = assets.does_asset_exist(LEVEL_PATH)
    if existing:
        unreal.log("Moonfall: existing level found; loading without modifying it.")
        if not levels.load_level(LEVEL_PATH):
            raise RuntimeError("Existing MoonfallWorld could not be loaded.")
    else:
        unreal.log("Moonfall: generating new standalone startup map.")
        if not levels.new_level(LEVEL_PATH, False):
            raise RuntimeError("Failed to create /Game/Maps/MoonfallWorld")
        start = subsystem.spawn_actor_from_class(
            unreal.PlayerStart, unreal.Vector(0, 0, 150), unreal.Rotator()
        )
        sun = subsystem.spawn_actor_from_class(
            unreal.DirectionalLight, unreal.Vector(0, 0, 1400),
            unreal.Rotator(-45, 20, 0)
        )
        sky = subsystem.spawn_actor_from_class(
            unreal.SkyLight, unreal.Vector(0, 0, 1200), unreal.Rotator()
        )
        if not all((start, sun, sky)):
            raise RuntimeError("Missing PlayerStart, directional light or skylight.")
        if not levels.save_current_level():
            raise RuntimeError("Failed to save generated MoonfallWorld.")

    if not assets.does_asset_exist(LEVEL_PATH):
        raise RuntimeError("Unreal has no MoonfallWorld map asset after generation.")

    world_actors = subsystem.get_all_level_actors()
    if not verify_actor(world_actors, unreal.PlayerStart):
        raise RuntimeError("MoonfallWorld needs a PlayerStart or the packaged game may spawn blank.")

    # Do not resave existing authored maps during validation.
    unreal.log("Moonfall startup world validated: " + LEVEL_PATH)


if __name__ == "__main__":
    main()

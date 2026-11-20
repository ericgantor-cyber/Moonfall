"""Unreal Editor helper: add 18 original Moonfall shrine actors to Shiverwake.

Does not change project startup maps, save slots, existing actors, heroes, or
legacy shrines. Returns the number created, so migration is idempotent.
"""
import unreal
from shiverwake_shrine_layout import SHRINE_SITES, validate_shrine_sites

SHRINE_TAG = unreal.Name("ShiverwakePreviewShrine")


def install_shiverwake_shrines(actor_subsystem, shrine_class):
    validate_shrine_sites()
    if actor_subsystem is None or shrine_class is None:
        raise RuntimeError("UE5 actor subsystem/shrine class unavailable")
    actors = unreal.EditorLevelLibrary.get_all_level_actors()
    existing = {}
    for actor in actors:
        if actor.get_class().get_name() != "MoonfallCheckpointShrine":
            continue
        shrine_id = str(actor.get_editor_property("shrine_id"))
        if shrine_id in existing:
            raise RuntimeError("Duplicate shrine ID in preview map: " + shrine_id)
        existing[shrine_id] = actor

    planned = {site[0] for site in SHRINE_SITES}
    for shrine_id, display, position in SHRINE_SITES:
        if shrine_id not in existing:
            continue
        actor = existing[shrine_id]
        pos = actor.get_actor_location()
        if ((pos.x - position[0]) ** 2 +
                (pos.y - position[1]) ** 2 +
                (pos.z - position[2]) ** 2 > 100.0):
            raise RuntimeError("Existing Shiverwake shrine was moved; refusing to overwrite: " + shrine_id)
        if actor.get_actor_label() != display:
            raise RuntimeError("Existing Shiverwake shrine name changed: " + shrine_id)
    # An authored shrine outside our manifest is preserved, not deleted.
    # Never silently reuse another shrine's location or identity.
    for shrine_id, display, position in SHRINE_SITES:
        if shrine_id in existing:
            continue
        for other_id, actor in existing.items():
            if other_id in planned:
                continue
            pos = actor.get_actor_location()
            if ((pos.x - position[0]) ** 2 +
                    (pos.y - position[1]) ** 2 < 300.0 ** 2):
                raise RuntimeError("Authored shrine too close to " + shrine_id)

    created = 0
    for shrine_id, display, position in SHRINE_SITES:
        if shrine_id in existing:
            continue
        actor = actor_subsystem.spawn_actor_from_class(
            shrine_class, unreal.Vector(*position), unreal.Rotator(0.0, 0.0, 0.0))
        if actor is None:
            raise RuntimeError("Could not place Shiverwake shrine " + shrine_id)
        actor.set_editor_property("shrine_id", unreal.Name(shrine_id))
        actor.set_editor_property("map_display_name", unreal.Text(display))
        actor.set_editor_property("tags", [SHRINE_TAG])
        actor.set_actor_label(display)
        if str(actor.get_editor_property("shrine_id")) != shrine_id:
            raise RuntimeError("Shrine ID was not retained: " + shrine_id)
        created += 1
    unreal.log("Shiverwake shrine layout: %d new, %d total requested" %
               (created, len(SHRINE_SITES)))
    return created

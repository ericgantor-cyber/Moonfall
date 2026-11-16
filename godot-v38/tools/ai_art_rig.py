#!/usr/bin/env python3
"""Offline glTF 2.0 skin, skeleton and animation-clip validation for Moonfall.

A skeleton is *not* inferred from a pretty 3D mesh. A rigged asset must have
glTF skin joints bound to a mesh, and actual animation channels with accessors.
No AI service is contacted; humanoid auto-rigging is a separate paid process.
"""
import json
import math
import re
import struct

from ai_art_3d import glb_info

# No arbitrary creative caps on bones, skins, clips, channels, keys or duration.
# GLB bytes and referenced structures are still validated against the input.
PREFERRED_CLIPS = ("idle", "move", "attack", "hurt", "death")
OPTIONAL_CLIPS = ("spawn", "telegraph", "special_1", "special_2", "victory")
ALL_CLIPS = set(PREFERRED_CLIPS + OPTIONAL_CLIPS)
ALIASES = {
    "idle": "idle", "breathe": "idle", "breathing": "idle", "stand": "idle",
    "walk": "move", "walking": "move", "run": "move", "running": "move",
    "locomotion": "move", "move": "move", "sprint": "move",
    "attack": "attack", "attacking": "attack", "bite": "attack",
    "slash": "attack", "punch": "attack", "strike": "attack", "shoot": "attack",
    "hit": "hurt", "damage": "hurt", "hurt": "hurt", "flinch": "hurt",
    "stagger": "hurt", "impact": "hurt",
    "die": "death", "dead": "death", "death": "death", "dying": "death",
    "spawn": "spawn", "appear": "spawn", "summon": "spawn",
    "warning": "telegraph", "telegraph": "telegraph", "chargeup": "telegraph",
    "special1": "special_1", "special2": "special_2", "spell": "special_1",
    "victory": "victory", "taunt": "victory",
}


def canonical_clip(name: str) -> str:
    """Map common Blender/Meshy aliases, preserving exact user-labelled clips."""
    if not isinstance(name, str) or not name.strip():
        return ""
    normalized = name.strip().lower()
    if normalized in ALL_CLIPS:
        return normalized
    # Godot may import names like AnimationLibrary/Armature|Walk_001.
    parts = [p for p in re.split(r"[^a-z0-9]+", normalized) if p]
    for part in reversed(parts):
        if part in ALIASES:
            return ALIASES[part]
    squashed = "".join(parts)
    return ALIASES.get(squashed, "")


def _json_document(payload: bytes) -> dict:
    glb_info(payload)
    size, kind = struct.unpack_from("<II", payload, 12)
    if kind != 0x4E4F534A or size <= 0 or size > len(payload) - 20:
        raise ValueError("Missing glTF JSON.")
    document = json.loads(payload[20:20 + size].rstrip(b" \x00"))
    if not isinstance(document, dict):
        raise ValueError("GLB metadata must be an object.")
    return document


def _reference(index, collection: list, label: str) -> dict:
    if not isinstance(index, int) or isinstance(index, bool) or not 0 <= index < len(collection):
        raise ValueError(f"Invalid {label} reference in rigged model.")
    entry = collection[index]
    if not isinstance(entry, dict):
        raise ValueError(f"Invalid {label} entry.")
    return entry


def rig_report(payload: bytes, require_rig: bool = False) -> dict:
    """Inspect actual bound skeletons, animation tracks and Moonfall aliases."""
    doc = _json_document(payload)
    skins, nodes = doc.get("skins", []), doc.get("nodes", [])
    animations, accessors = doc.get("animations", []), doc.get("accessors", [])
    if not all(isinstance(items, list) for items in (skins, nodes, animations, accessors)):
        raise ValueError("Rig/animation records must be arrays.")
    # The container-size and glTF index checks bound memory without limiting
    # the number of animations a creator may author.
    total_bones = 0
    bound_meshes = 0
    for skin in skins:
        if not isinstance(skin, dict):
            raise ValueError("Malformed skeleton skin.")
        joints = skin.get("joints")
        if not isinstance(joints, list) or not joints:
            raise ValueError("A skeleton must contain valid joints.")
        if len(set(map(str, joints))) != len(joints):
            raise ValueError("Duplicate bone joints in a skin.")
        for joint in joints:
            _reference(joint, nodes, "skeleton joint")
        total_bones += len(joints)
        # No independent bone-count cap: only valid scene-node refs pass.
        if "skeleton" in skin:
            _reference(skin["skeleton"], nodes, "skeleton root")
        if "inverseBindMatrices" in skin:
            matrix = _reference(skin["inverseBindMatrices"], accessors, "inverse-bind matrices")
            if (matrix.get("type") != "MAT4" or matrix.get("componentType") != 5126
                    or int(matrix.get("count", -1)) < len(joints)):
                raise ValueError("Inverse-bind matrices must be float MAT4 entries per joint.")
    for node in nodes:
        if not isinstance(node, dict):
            raise ValueError("Malformed rig node.")
        if "skin" in node:
            _reference(node["skin"], skins, "skin")
            if "mesh" not in node:
                raise ValueError("Rigged node has skin but no mesh.")
            _reference(node["mesh"], doc.get("meshes", []), "skinned mesh")
            bound_meshes += 1
    if skins and not bound_meshes:
        raise ValueError("Skeleton isn't bound to visible model mesh geometry.")
    clips = []
    mapped: dict[str, str] = {}
    warnings: list[str] = []
    for index, animation in enumerate(animations):
        if not isinstance(animation, dict):
            raise ValueError("Malformed animation clip.")
        name = str(animation.get("name", f"Animation_{index + 1}"))
        samplers, channels = animation.get("samplers", []), animation.get("channels", [])
        if not isinstance(samplers, list) or not isinstance(channels, list) or not channels:
            raise ValueError("Animation must contain valid keyed channels.")
        used = set()
        duration = 0.0
        for channel in channels:
            if not isinstance(channel, dict):
                raise ValueError("Malformed animation channel.")
            target = channel.get("target", {})
            if not isinstance(target, dict) or target.get("path") not in (
                    "translation", "rotation", "scale", "weights"):
                raise ValueError("Unsupported animation target.")
            if "node" in target:
                _reference(target["node"], nodes, "animated node")
            else:
                raise ValueError("Animation must target a model node.")
            sampler_id = channel.get("sampler")
            sampler = _reference(sampler_id, samplers, "animation sampler")
            interpolation = sampler.get("interpolation", "LINEAR")
            if interpolation not in ("LINEAR", "STEP", "CUBICSPLINE"):
                raise ValueError("Unsupported keyframe interpolation.")
            accessor_id = sampler.get("input")
            times = _reference(accessor_id, accessors, "keyframe input")
            _reference(sampler.get("output"), accessors, "keyframe output")
            count = times.get("count")
            if not isinstance(count, int) or isinstance(count, bool) or count < 1:
                raise ValueError("Animation channel requires positive keyframe counts.")
            # Protect against malformed accessors claiming billions of keyframes
            # without real file data. This is a file-integrity constraint,
            # not an animation-authoring limit.
            if count > len(payload):
                raise ValueError("Keyframe count cannot exceed source file bytes.")
            # glTF min/max are optional; validate them if present.
            lo, hi = times.get("min"), times.get("max")
            if isinstance(lo, list) and isinstance(hi, list) and lo and hi:
                start, finish = lo[0], hi[0]
                if (not isinstance(start, (int, float)) or not isinstance(finish, (int, float))
                        or not math.isfinite(start) or not math.isfinite(finish)
                        or start < 0 or finish < start):
                    raise ValueError("Animation duration is invalid.")
                duration = max(duration, float(finish - start))
            used.add(sampler_id)
        clip = {"name": name, "channels": len(channels),
                "keyframe_samplers": len(used), "duration_seconds": round(duration, 3),
                "state": canonical_clip(name)}
        clips.append(clip)
        if clip["state"] and clip["state"] not in mapped:
            mapped[clip["state"]] = name
    if animations and not skins:
        warnings.append("Animated scene has no skeletal skin: clips may animate objects, not a creature rig.")
    if skins and not animations:
        warnings.append("Rigged mesh has no animation clips; it will render in bind pose.")
    missing = [key for key in PREFERRED_CLIPS if key not in mapped]
    if skins and missing:
        warnings.append("Missing preferred Moonfall clips: " + ", ".join(missing))
    if animations and not mapped:
        warnings.append("Animation names need idle/move/attack/hurt/death aliases.")
    if require_rig and not (skins and bound_meshes and animations and mapped):
        raise ValueError("Animated rig required: a skinned mesh, animation clips and recognizable names are needed.")
    return {
        "rigged": bool(skins and bound_meshes),
        "skin_count": len(skins),
        "bone_count": total_bones,
        "skinned_meshes": bound_meshes,
        "animation_count": len(clips),
        "clips": clips,
        "mapped_clips": mapped,
        "missing_preferred": missing,
        "warnings": warnings,
    }

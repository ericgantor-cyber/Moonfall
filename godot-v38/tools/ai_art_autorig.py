#!/usr/bin/env python3
"""Optional AI humanoid rigging and local pre-rigged GLB import for Moonfall.

Meshy's auto-rig API supports HUMANOID/BIPED meshes only. Animals, hydras,
quadrupeds and serpents need an external compatible rig (e.g. from Blender).
Nothing is submitted without both --submit and --confirm-humanoid, plus an
API key. Approved rigged GLBs live alongside original static model assets.
"""
import argparse
import base64
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import sys

import ai_art_studio as studio
import ai_art_3d as models
from ai_art_rig import rig_report

RIG_API = "https://api.meshy.ai/openapi/v1/rigging"
ANIMATION_API = "https://api.meshy.ai/openapi/v1/animations"
ANIMATION_TASKS = studio.STAGING / "humanoid_animation_tasks"
TASKS = studio.STAGING / "humanoid_rig_tasks"
CANDIDATES = studio.STAGING / "rigged_candidates"
APPROVED = studio.ROOT / "rigged"
MAX_UPLOADED_GLB_BYTES = 60 * 1024 * 1024
TASK_ID = re.compile(r"^[a-zA-Z0-9_-]{8,90}$")
OPTIONS = ("combined", "walking", "running", "base")


def choose(args: argparse.Namespace) -> dict:
    if args.guardian is not None:
        return studio.guardian_profile(args.guardian)
    return studio.animal_profile(args.animal)


def paths(item: dict, variant: str = "combined") -> tuple[Path, Path, Path]:
    if item["category"] not in ("guardians", "animals") or variant not in OPTIONS:
        raise ValueError("Rigged 3D gameplay art is supported only for known creatures.")
    identifier = item["asset_id"]
    suffix = "" if variant == "combined" else "_" + variant
    staged = CANDIDATES / item["category"] / (identifier + suffix + ".glb")
    approved = APPROVED / item["category"] / (identifier + ".glb")
    task = TASKS / (identifier + ".json")
    return staged, approved, task


def verified_static_source(item: dict) -> tuple[bytes, str]:
    _, original, _ = models.paths(item)
    manifest = original.with_suffix(".json")
    if not original.is_file() or original.is_symlink() or not manifest.is_file():
        raise FileNotFoundError("Approve an existing textured GLB model before auto-rigging it.")
    payload = original.read_bytes()
    details = models.glb_info(payload)
    if len(payload) > MAX_UPLOADED_GLB_BYTES:
        raise ValueError("Reduce source GLB under 60 MiB before submitting an AI rig task.")
    record = json.loads(manifest.read_text(encoding="utf-8"))
    digest = hashlib.sha256(payload).hexdigest()
    if (not isinstance(record, dict) or record.get("reviewed") is not True
            or record.get("asset_id") != item["asset_id"] or record.get("sha256") != digest):
        raise ValueError("Original model's review manifest has been altered.")
    if not details.get("texture_images"):
        raise ValueError("Meshy requires a textured humanoid GLB, not an untextured mesh.")
    if rig_report(payload)["rigged"]:
        raise ValueError("This model is already rigged. Import a combined animated GLB instead.")
    return payload, digest


def submit(item: dict, confirmed_humanoid: bool, height: float) -> str:
    if not confirmed_humanoid:
        raise ValueError("Meshy auto-rig works only for humanoid bipeds. Explicit --confirm-humanoid is required.")
    if not 0.5 <= height <= 4.0:
        raise ValueError("Humanoid reference height must be 0.5–4.0 meters.")
    payload, source_digest = verified_static_source(item)
    _, approved, task_path = paths(item)
    if task_path.exists() or approved.exists():
        raise FileExistsError("Rig task/approved character already exists. No duplicate paid tasks.")
    # A data URI avoids exposing user asset URLs publicly. No keys written to disk.
    request = {"model_url": "data:model/gltf-binary;base64," +
               base64.b64encode(payload).decode("ascii"), "height_meters": height}
    result = models.api_call("POST", RIG_API, request)
    task_id = str(result.get("result", ""))
    if not TASK_ID.fullmatch(task_id):
        raise RuntimeError("Rig service returned no valid task identifier.")
    task_path.parent.mkdir(parents=True, exist_ok=True)
    task_path.write_text(json.dumps({
        "task_id": task_id, "asset_id": item["asset_id"],
        "source_sha256": source_digest, "provider": "Meshy", "humanoid_confirmed": True
    }, indent=2) + "\n", encoding="utf-8")
    return task_id


def status(item: dict) -> dict:
    _, _, task_path = paths(item)
    if not task_path.is_file():
        raise FileNotFoundError("No submitted rig task for this creature.")
    record = json.loads(task_path.read_text(encoding="utf-8"))
    if (record.get("asset_id") != item["asset_id"]
            or not TASK_ID.fullmatch(str(record.get("task_id", "")))):
        raise ValueError("Rig task ID does not match the selected creature.")
    reply = models.api_call("GET", RIG_API + "/" + record["task_id"])
    if reply.get("id") != record["task_id"]:
        raise RuntimeError("Rig provider returned the wrong task.")
    return reply


def submit_animation(item: dict, action_ids: list[int]) -> str:
    # These must be actual IDs selected from Meshy's live Animation Library;
    # the tool intentionally does not guess or invent animation preset IDs.
    if not 1 <= len(action_ids) <= 10 or len(set(action_ids)) != len(action_ids):
        raise ValueError("Choose 1–10 different official animation IDs.")
    if any(type(number) is not int or number <= 0 for number in action_ids):
        raise ValueError("Animation preset IDs must be positive integers.")
    rig_status = status(item)
    if rig_status.get("status") != "SUCCEEDED":
        raise ValueError("Humanoid rigging must finish before applying AI animations.")
    task_file = ANIMATION_TASKS / (item["asset_id"] + ".json")
    if task_file.exists():
        raise FileExistsError("An animation task is already in progress or recorded.")
    result = models.api_call("POST", ANIMATION_API, {
        "rig_task_id": rig_status["id"], "action_ids": action_ids,
    })
    animation_id = str(result.get("result", ""))
    if not TASK_ID.fullmatch(animation_id):
        raise RuntimeError("Animation service returned no valid task identifier.")
    task_file.parent.mkdir(parents=True, exist_ok=True)
    task_file.write_text(json.dumps({
        "asset_id": item["asset_id"], "task_id": animation_id,
        "rig_task_id": rig_status["id"], "action_ids": action_ids,
    }, indent=2) + "\n", encoding="utf-8")
    return animation_id


def animation_status(item: dict) -> dict:
    task_file = ANIMATION_TASKS / (item["asset_id"] + ".json")
    if not task_file.is_file():
        raise FileNotFoundError("No animation job; submit actions after rigging first.")
    record = json.loads(task_file.read_text(encoding="utf-8"))
    if (record.get("asset_id") != item["asset_id"]
            or not TASK_ID.fullmatch(str(record.get("task_id", "")))):
        raise ValueError("Animation task record is invalid.")
    result = models.api_call("GET", ANIMATION_API + "/" + record["task_id"])
    if result.get("id") != record["task_id"]:
        raise RuntimeError("Animation service returned the wrong task.")
    return result


def fetch_animation(item: dict) -> Path:
    result = animation_status(item)
    if result.get("status") != "SUCCEEDED":
        raise RuntimeError(f"Animation job is not complete ({result.get('status', 'unknown')}).")
    url = (result.get("result") or {}).get("animation_glb_url", "")
    if not isinstance(url, str) or not url:
        raise RuntimeError("Animation task returned no GLB with skin and action clips.")
    return stage(item, models.download_glb(url), "combined", "meshy-animation")


def stage(item: dict, blob: bytes, variant: str, origin: str) -> Path:
    mesh = models.glb_info(blob)
    rig = rig_report(blob, require_rig=True)
    staged, approved, _ = paths(item, variant)
    if staged.exists() or approved.exists():
        raise FileExistsError("Rigged artwork already staged or approved; no overwrites.")
    staged.parent.mkdir(parents=True, exist_ok=True)
    staged.write_bytes(blob)
    staged.with_suffix(".json").write_text(json.dumps({
        "asset_id": item["asset_id"], "origin": origin, "variant": variant,
        "sha256": hashlib.sha256(blob).hexdigest(), "reviewed": False,
        "mesh": mesh, "rig": rig,
    }, indent=2) + "\n", encoding="utf-8")
    return staged


def fetch(item: dict, variant: str) -> Path:
    task = status(item)
    if task.get("status") != "SUCCEEDED":
        raise RuntimeError(f"Rig task not complete ({task.get('status', 'unknown')}).")
    result = task.get("result") or {}
    if variant == "walking":
        url = (result.get("basic_animations") or {}).get("walking_glb_url", "")
    elif variant == "running":
        url = (result.get("basic_animations") or {}).get("running_glb_url", "")
    else:
        url = result.get("rigged_character_glb_url", "")
    if not isinstance(url, str) or not url:
        raise RuntimeError("Provider result doesn't contain the requested rigged GLB.")
    blob = models.download_glb(url)
    return stage(item, blob, variant, "meshy-rigging")


def import_local(item: dict, source: Path) -> Path:
    source = source.expanduser().resolve(strict=True)
    if not source.is_file() or source.suffix.lower() != ".glb":
        raise ValueError("Import an actual combined rigged .glb model file.")
    if source.is_relative_to((studio.PROJECT / "assets" / "heroes").resolve()):
        raise ValueError("Z and Nyra original hero assets may never be used.")
    if source.stat().st_size > models.MAX_GLB_BYTES:
        raise ValueError("Rigged GLB exceeds maximum model size.")
    return stage(item, source.read_bytes(), "combined", "local-rigged-glb")


def approve(item: dict, variant: str) -> Path:
    staged, approved, _ = paths(item, variant)
    manifest = staged.with_suffix(".json")
    if not staged.is_file() or staged.is_symlink() or not manifest.is_file():
        raise FileNotFoundError("Review a staged rigged GLB before approval.")
    if approved.exists() or approved.is_symlink():
        raise FileExistsError("Approved rigged model already exists; original art is never overwritten.")
    blob = staged.read_bytes()
    mesh = models.glb_info(blob)
    rig = rig_report(blob, require_rig=True)
    record = json.loads(manifest.read_text(encoding="utf-8"))
    if (not isinstance(record, dict) or record.get("asset_id") != item["asset_id"]
            or record.get("sha256") != hashlib.sha256(blob).hexdigest()
            or record.get("mesh") != mesh or record.get("rig") != rig
            or record.get("reviewed") is not False):
        raise ValueError("Rigged GLB changed after review; refusing approval.")
    approved.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(staged, approved)
    record["reviewed"] = True
    approved.with_suffix(".json").write_text(json.dumps(record, indent=2) + "\n", encoding="utf-8")
    return approved


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    subject = parser.add_mutually_exclusive_group(required=True)
    subject.add_argument("--guardian", type=int)
    subject.add_argument("--animal")
    action = parser.add_mutually_exclusive_group()
    action.add_argument("--submit", action="store_true", help="One paid Meshy humanoid-rig task")
    action.add_argument("--status", action="store_true", help="Read-only query of submitted rig task")
    action.add_argument("--fetch", action="store_true", help="Fetch rigged GLB to offline review staging")
    action.add_argument("--import-rig", type=Path, help="Import an already-rigged GLB, no API fee")
    action.add_argument("--approve", action="store_true", help="Explicitly approve a reviewed rigged GLB")
    action.add_argument("--animate", action="store_true", help="Explicit paid Meshy animation task after successful humanoid rigging")
    action.add_argument("--animation-status", action="store_true", help="Check the earlier AI animation task")
    action.add_argument("--animation-fetch", action="store_true", help="Stage completed animation GLB for review")
    action.add_argument("--list-actions", action="store_true", help="List available official Meshy animation preset IDs, no credits")
    parser.add_argument("--confirm-humanoid", action="store_true", help="Required for paid AI rigging")
    parser.add_argument("--height", type=float, default=1.8)
    parser.add_argument("--action-ids", help="Comma-separated official Meshy Animation Library IDs, maximum 10")
    parser.add_argument("--variant", choices=OPTIONS, default="combined")
    args = parser.parse_args(argv)
    try:
        item = choose(args)
        if args.submit:
            print("Submitted one paid humanoid rig task:", submit(item, args.confirm_humanoid, args.height))
        elif args.status:
            data = status(item)
            print(json.dumps({key: data.get(key) for key in ("id", "status", "progress", "consumed_credits")}, indent=2))
        elif args.fetch:
            print("Fetched rigged candidate for review:", fetch(item, args.variant))
        elif args.import_rig is not None:
            print("Imported rig for review:", import_local(item, args.import_rig))
        elif args.animate:
            if not args.action_ids:
                raise ValueError("--animate requires comma-separated --action-ids from Meshy's official library.")
            try:
                ids = [int(v.strip()) for v in args.action_ids.split(",")]
            except ValueError as error:
                raise ValueError("Animation action IDs must be integers.") from error
            print("Submitted one paid animation task:", submit_animation(item, ids))
        elif args.animation_status:
            data = animation_status(item)
            print(json.dumps({key: data.get(key) for key in ("id","status","progress","consumed_credits")}, indent=2))
        elif args.animation_fetch:
            print("Staged animated GLB for review:", fetch_animation(item))
        elif args.list_actions:
            print(json.dumps(models.api_call("GET", ANIMATION_API + "/library"), indent=2))
        elif args.approve:
            print("Approved rigged model for Godot:", approve(item, args.variant))
        else:
            staged, approved, task = paths(item, args.variant)
            print(json.dumps({
                "asset_id": item["asset_id"], "staged": str(staged),
                "approved": str(approved), "task_file": str(task),
                "note": "Preview only: no paid rigging, network or file writes.",
            }, indent=2))
        return 0
    except (ValueError, RuntimeError, FileNotFoundError, FileExistsError, OSError) as error:
        print("Moonfall rigging:", error, file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())

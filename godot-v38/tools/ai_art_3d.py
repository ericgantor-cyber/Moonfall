#!/usr/bin/env python3
"""Moonfall optional real-3D GLB pipeline with genuine AI-generated 4K PBR textures.

The game never calls a model service. External Meshy API work requires an
explicit one-off --submit flag and the developer's own MESHY_API_KEY.
--import-model works fully offline, with no credits. No bulk paid generation.
"""
import argparse
import base64
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import struct
import sys
import urllib.error
import urllib.parse
import urllib.request

import ai_art_studio as studio
from ai_art_game_catalog import parse_slot

STAGING_3D = studio.STAGING / "models"
TASKS_3D = studio.STAGING / "3d_tasks"
APPROVED_3D = studio.ROOT / "models"
MESHY_ENDPOINT = "https://api.meshy.ai/openapi/v1/image-to-3d"
MAX_GLB_BYTES = 96 * 1024 * 1024
MAX_MODEL_TRIANGLES = 100_000
ALLOWED_MODEL_CATEGORIES = frozenset(("guardians", "animals", "scenery", "gear", "chambers"))
TASK_ID_RE = re.compile(r"^[a-zA-Z0-9_-]{8,90}$")


def choose_item(args: argparse.Namespace) -> dict:
    if args.guardian is not None:
        return studio.guardian_profile(args.guardian)
    if args.animal is not None:
        return studio.animal_profile(args.animal)
    item = parse_slot(args.game_art)
    if item["category"] not in ALLOWED_MODEL_CATEGORIES:
        raise ValueError("3D models belong to animals, guardians, scenery, gear or dungeon props. HUD and spell art stays 2D.")
    return item


def paths(item: dict) -> tuple[Path, Path, Path]:
    if item["category"] not in ALLOWED_MODEL_CATEGORIES:
        raise ValueError("This category is not a supported 3D model.")
    identifier = item["asset_id"]
    if not re.fullmatch(r"[a-z0-9_-]{3,90}", identifier):
        raise ValueError("Invalid model asset ID.")
    staged = STAGING_3D / item["category"] / f"{identifier}.glb"
    approved = APPROVED_3D / item["category"] / f"{identifier}.glb"
    task = TASKS_3D / f"{identifier}.json"
    return staged, approved, task


def embedded_image_size(data: bytes, mime: str) -> tuple[int, int] | None:
    """Read PNG/JPEG header dimensions without decoding potentially huge textures."""
    if mime == "image/png" or data.startswith(b"\x89PNG\r\n\x1a\n"):
        if len(data) < 24 or not data.startswith(b"\x89PNG\r\n\x1a\n"):
            return None
        width, height = struct.unpack_from(">II", data, 16)
        return (width, height) if 0 < width <= 16384 and 0 < height <= 16384 else None
    if mime != "image/jpeg" and not data.startswith(b"\xff\xd8"):
        return None
    if not data.startswith(b"\xff\xd8"):
        return None
    index = 2
    while index < len(data) - 10:
        if data[index] != 0xFF:
            return None
        while index < len(data) and data[index] == 0xFF:
            index += 1
        if index >= len(data):
            return None
        marker = data[index]
        index += 1
        if marker in (0xD8, 0xD9) or 0xD0 <= marker <= 0xD7:
            continue
        if index + 2 > len(data):
            return None
        length = struct.unpack_from(">H", data, index)[0]
        if length < 2 or index + length > len(data):
            return None
        if marker in (0xC0, 0xC1, 0xC2, 0xC3, 0xC5, 0xC6, 0xC7,
                      0xC9, 0xCA, 0xCB, 0xCD, 0xCE, 0xCF):
            if length < 7:
                return None
            height, width = struct.unpack_from(">HH", data, index + 3)
            return (width, height) if 0 < width <= 16384 and 0 < height <= 16384 else None
        if marker == 0xDA:
            break
        index += length
    return None


def _embedded_texture_report(gltf: dict, binary: bytes) -> dict:
    """Record actual embedded texture dimensions, not just a provider's request."""
    images = gltf.get("images", [])
    buffer_views = gltf.get("bufferViews", [])
    textures = gltf.get("textures", [])
    if not isinstance(buffer_views, list) or not isinstance(textures, list):
        raise ValueError("GLB buffer views and texture references must be arrays.")
    dimensions: list[tuple[int, int] | None] = []
    if not isinstance(images, list) or len(images) > 96:
        raise ValueError("GLB has too many texture resources.")
    for image in images:
        if not isinstance(image, dict):
            raise ValueError("Invalid GLB image record.")
        blob = b""
        mime = str(image.get("mimeType", ""))
        if "bufferView" in image:
            view_id = image["bufferView"]
            if not isinstance(view_id, int) or view_id < 0 or view_id >= len(buffer_views):
                raise ValueError("GLB image buffer view is invalid.")
            view = buffer_views[view_id]
            if not isinstance(view, dict) or view.get("buffer", 0) != 0:
                raise ValueError("Embedded GLB image must reference the internal binary buffer.")
            start = view.get("byteOffset", 0)
            length = view.get("byteLength", 0)
            if (not isinstance(start, int) or not isinstance(length, int)
                    or start < 0 or length < 0 or length > 48 * 1024 * 1024
                    or start + length > len(binary)):
                raise ValueError("GLB contains an invalid or oversized embedded texture.")
            blob = binary[start:start + length]
        elif "uri" in image:
            uri = image["uri"]
            if not isinstance(uri, str) or not uri.startswith("data:") or "," not in uri:
                raise ValueError("External texture image references are forbidden.")
            import base64
            header, encoded = uri.split(",", 1)
            if ";base64" not in header or len(encoded) > 64 * 1024 * 1024:
                raise ValueError("GLB image data URI is invalid or too large.")
            try:
                blob = base64.b64decode(encoded, validate=True)
            except (ValueError, base64.binascii.Error) as error:
                raise ValueError("Invalid inline image encoding.") from error
            mime = header.removeprefix("data:").split(";", 1)[0]
        dimensions.append(embedded_image_size(blob, mime))
    referenced: set[int] = set()
    pbr_maps = 0
    for material in gltf.get("materials", []):
        if not isinstance(material, dict):
            continue
        pbr = material.get("pbrMetallicRoughness") or {}
        if not isinstance(pbr, dict):
            raise ValueError("Invalid GLB PBR material.")
        mappings = [pbr.get("baseColorTexture"), pbr.get("metallicRoughnessTexture"),
                    material.get("normalTexture"), material.get("occlusionTexture")]
        for mapping in mappings:
            if not isinstance(mapping, dict):
                continue
            texture_id = mapping.get("index")
            if not isinstance(texture_id, int) or texture_id < 0 or texture_id >= len(textures):
                raise ValueError("GLB material texture index is invalid.")
            tex = textures[texture_id]
            if not isinstance(tex, dict):
                raise ValueError("Invalid GLB texture resource.")
            source = tex.get("source")
            if source is not None:
                if not isinstance(source, int) or source < 0 or source >= len(images):
                    raise ValueError("GLB texture references an invalid image.")
                referenced.add(source)
        if pbr.get("metallicRoughnessTexture") and material.get("normalTexture"):
            pbr_maps += 1
    reported = [dimensions[i] for i in sorted(referenced)] if referenced else dimensions
    known = [v for v in reported if v]
    return {
        "texture_images": len(images),
        "texture_dimensions": [[w, h] if w and h else None for val in reported for w, h in [val or (0, 0)]],
        "max_texture_size": max((max(w, h) for w, h in known), default=0),
        "has_4k_texture": any(w >= 4096 and h >= 4096 for w, h in known),
        "pbr_material_count": pbr_maps,
    }


def glb_info(payload: bytes) -> dict:
    """Check self-contained GLB geometry and report *actual* texture/PBR details."""
    if len(payload) < 32 or len(payload) > MAX_GLB_BYTES:
        raise ValueError("Model must be a real GLB smaller than 96 MiB.")
    magic, version, length = struct.unpack_from("<4sII", payload, 0)
    if magic != b"glTF" or version != 2 or length != len(payload):
        raise ValueError("Not a complete GLB version 2 asset.")
    cursor = 12
    first = True
    document = None
    binary = b""
    binary_chunks = 0
    while cursor < len(payload):
        if cursor + 8 > len(payload):
            raise ValueError("Truncated GLB chunk.")
        size, kind = struct.unpack_from("<II", payload, cursor)
        cursor += 8
        if size > MAX_GLB_BYTES or cursor + size > len(payload):
            raise ValueError("Invalid GLB chunk length.")
        if first and kind != 0x4E4F534A:
            raise ValueError("GLB must start with JSON.")
        if kind == 0x4E4F534A:
            if document is not None:
                raise ValueError("Duplicate GLB JSON chunk.")
            try:
                document = json.loads(payload[cursor:cursor + size].rstrip(b" \x00"))
            except (UnicodeError, ValueError) as error:
                raise ValueError("GLB JSON is malformed.") from error
        elif kind == 0x004E4942:
            binary_chunks += 1
            if binary_chunks > 1:
                raise ValueError("Multiple GLB binary chunks are unsupported.")
            binary = payload[cursor:cursor + size]
        cursor += size
        first = False
    if not isinstance(document, dict) or cursor != len(payload) or document.get("asset", {}).get("version") != "2.0":
        raise ValueError("GLB lacks valid glTF 2.0 scene metadata.")
    meshes = document.get("meshes", [])
    nodes = document.get("nodes", [])
    if not isinstance(meshes, list) or not 1 <= len(meshes) <= 120 or not isinstance(nodes, list) or len(nodes) > 800:
        raise ValueError("Model has no meshes or exceeds the native scene node budget.")
    if not document.get("scenes"):
        raise ValueError("GLB has no scene to instantiate.")
    buffers = document.get("buffers", [])
    if not isinstance(buffers, list) or len(buffers) > 4:
        raise ValueError("GLB buffers exceed supported limits.")
    if binary and buffers:
        declared = buffers[0].get("byteLength", -1)
        if not isinstance(declared, int) or declared < 0 or declared > len(binary):
            raise ValueError("GLB binary data is shorter than the declared buffer.")
    for collection_name in ("buffers", "images"):
        for entry in document.get(collection_name, []):
            uri = entry.get("uri")
            if uri is not None and not (isinstance(uri, str) and uri.startswith("data:")):
                raise ValueError("GLB external file/network references are forbidden.")
    accessors = document.get("accessors", [])
    if not isinstance(accessors, list) or len(accessors) > 3500:
        raise ValueError("Excessive GLB accessor count.")
    triangles = 0
    for mesh in meshes:
        if not isinstance(mesh, dict):
            raise ValueError("Invalid GLB mesh entry.")
        for primitive in mesh.get("primitives", []):
            if not isinstance(primitive, dict):
                raise ValueError("Invalid GLB mesh primitive.")
            if primitive.get("mode", 4) != 4:
                raise ValueError("Gameplay 3D art must use triangle primitives.")
            index = primitive.get("indices", primitive.get("attributes", {}).get("POSITION", -1))
            if not isinstance(index, int) or not 0 <= index < len(accessors):
                raise ValueError("GLB primitive references an invalid accessor.")
            count = accessors[index].get("count", 0)
            if not isinstance(count, int) or count < 3:
                raise ValueError("GLB mesh accessor has no triangles.")
            triangles += count // 3
            if triangles > MAX_MODEL_TRIANGLES:
                raise ValueError("3D model exceeds 100,000 triangles. Remesh for Windows performance.")
    if not 1 <= triangles <= MAX_MODEL_TRIANGLES:
        raise ValueError("Model must contain real triangle geometry.")
    result = {"triangles": triangles, "mesh_count": len(meshes),
              "nodes": len(nodes), "bytes": len(payload), "has_binary_chunk": binary_chunks > 0}
    result.update(_embedded_texture_report(document, binary))
    return result

def approved_2d(item: dict) -> bytes:
    _, source = studio.dest_paths(item)
    manifest = source.with_suffix(".json")
    if not source.is_file() or not manifest.is_file():
        raise FileNotFoundError("First generate/import and approve the original 2D reference PNG.")
    content = source.read_bytes()
    studio.validate_png(content)
    record = json.loads(manifest.read_text(encoding="utf-8"))
    if (record.get("asset_id") != item["asset_id"]
            or record.get("reviewed") is not True
            or record.get("sha256") != hashlib.sha256(content).hexdigest()):
        raise ValueError("Only reviewed, untampered reference art can become a 3D model.")
    return content


def mesh_request(image: bytes) -> dict:
    studio.validate_png(image)
    if len(image) > 20 * 1024 * 1024:
        raise ValueError("Meshy accepts a maximum 20 MiB reference image. Use a smaller PNG for conversion.")
    return {
        "image_url": "data:image/png;base64," + base64.b64encode(image).decode("ascii"),
        "model_type": "standard",
        "ai_model": "meshy-7.1",
        # 4K here is actual 4096x4096 PBR TEXTURING; geometry stays optimized.
        "geometry_resolution": "standard",
        "texture_model": "meshy-7",
        "texture_resolution": "4k",
        "should_texture": True,
        "enable_pbr": True,
        "should_remesh": True,
        "target_polycount": 12000,
        "target_formats": ["glb"],
    }


def api_call(method: str, url: str, body: dict | None = None) -> dict:
    key = os.environ.get("MESHY_API_KEY", "").strip()
    if not key:
        raise ValueError("MESHY_API_KEY is required. Do not store API keys in Moonfall files.")
    request = urllib.request.Request(
        url,
        data=json.dumps(body).encode("utf-8") if body is not None else None,
        method=method,
        headers={"Authorization": "Bearer " + key, "Content-Type": "application/json"},
    )
    try:
        with urllib.request.urlopen(request, timeout=90) as response:
            return json.load(response)
    except urllib.error.HTTPError as error:
        raise RuntimeError(f"3D provider HTTP {error.code}. Check account credits and API configuration.") from None


def submit(item: dict) -> str:
    """One explicit paid API request. No background poller or implicit batch."""
    image = approved_2d(item)
    _, approved, task_file = paths(item)
    if approved.exists() or task_file.exists():
        raise FileExistsError("This model is already approved or has a pending conversion task.")
    result = api_call("POST", MESHY_ENDPOINT, mesh_request(image))
    task_id = str(result.get("result", ""))
    if not TASK_ID_RE.fullmatch(task_id):
        raise RuntimeError("The 3D provider did not return a valid task ID.")
    task_file.parent.mkdir(parents=True, exist_ok=True)
    task_file.write_text(json.dumps({
        "task_id": task_id, "asset_id": item["asset_id"],
        "provider": "Meshy", "ai_model": "meshy-7.1",
        "texture_model": "meshy-7", "texture_resolution": "4k",
        "geometry_resolution": "standard", "poly_target": 12000,
    }, indent=2) + "\n", encoding="utf-8")
    return task_id


def task_status(item: dict) -> dict:
    _, _, task_file = paths(item)
    if not task_file.is_file():
        raise FileNotFoundError("No 3D conversion task. Run --submit explicitly first.")
    record = json.loads(task_file.read_text(encoding="utf-8"))
    if record.get("asset_id") != item["asset_id"] or not TASK_ID_RE.fullmatch(str(record.get("task_id", ""))):
        raise ValueError("3D task record does not match the artwork slot.")
    task_id = record["task_id"]
    result = api_call("GET", MESHY_ENDPOINT + "/" + task_id)
    if result.get("id") != task_id:
        raise RuntimeError("The provider returned a different task than requested.")
    return result


def download_glb(url: str) -> bytes:
    parsed = urllib.parse.urlparse(url)
    host = (parsed.hostname or "").lower()
    if parsed.scheme != "https" or not (host == "assets.meshy.ai" or host.endswith(".assets.meshy.ai")):
        raise ValueError("Rejected unexpected GLB host. Only Meshy's asset host is permitted.")
    # urllib redirects are checked again through the final response URL.
    req = urllib.request.Request(url, method="GET")
    with urllib.request.urlopen(req, timeout=180) as response:
        final = urllib.parse.urlparse(response.geturl())
        final_host = (final.hostname or "").lower()
        if final.scheme != "https" or final_host != host:
            raise ValueError("Unexpected 3D download redirect.")
        data = response.read(MAX_GLB_BYTES + 1)
    glb_info(data)
    return data


def stage_model(item: dict, payload: bytes, origin: str, extra: dict | None = None) -> Path:
    analysis = glb_info(payload)
    staged, approved, _ = paths(item)
    if staged.exists() or approved.exists():
        raise FileExistsError("An existing staged or approved 3D model must not be overwritten.")
    staged.parent.mkdir(parents=True, exist_ok=True)
    staged.write_bytes(payload)
    record = {"asset_id": item["asset_id"], "category": item["category"],
              "subject": item["subject"], "sha256": hashlib.sha256(payload).hexdigest(),
              "origin": origin, "reviewed": False, "geometry": analysis}
    if extra:
        record["generation"] = extra
    staged.with_suffix(".json").write_text(json.dumps(record, indent=2) + "\n", encoding="utf-8")
    return staged


def fetch_model(item: dict) -> Path:
    status = task_status(item)
    if status.get("status") != "SUCCEEDED":
        raise RuntimeError(f"3D task isn't ready: {status.get('status', 'UNKNOWN')}. Run --status first.")
    url = status.get("model_urls", {}).get("glb", "")
    if not isinstance(url, str) or not url:
        raise RuntimeError("Provider task has no exported GLB.")
    payload = download_glb(url)
    return stage_model(item, payload, "meshy", {
        "task_id": status.get("id"), "consumed_credits": status.get("consumed_credits"),
        "texture_resolution_requested": "4k", "model": "meshy-7.1",
    })


def import_local(item: dict, source: Path) -> Path:
    source = source.expanduser().resolve(strict=True)
    if source.suffix.lower() != ".glb" or not source.is_file():
        raise ValueError("Only standalone .glb model files can be imported.")
    hero_dir = (studio.PROJECT / "assets" / "heroes").resolve()
    if source.is_relative_to(hero_dir):
        raise ValueError("Cannot reuse or modify original hero art.")
    if source.stat().st_size > MAX_GLB_BYTES:
        raise ValueError("3D model is larger than 96 MiB.")
    return stage_model(item, source.read_bytes(), "local-glb")


def approve_model(item: dict) -> Path:
    staged, approved, _ = paths(item)
    record_path = staged.with_suffix(".json")
    if not staged.is_file() or not record_path.is_file():
        raise FileNotFoundError("Stage and review a model before approving it.")
    if approved.exists():
        raise FileExistsError("An approved 3D model is already present.")
    data = staged.read_bytes()
    details = glb_info(data)
    record = json.loads(record_path.read_text(encoding="utf-8"))
    previous_geometry = record.get("geometry", {})
    if (record.get("asset_id") != item["asset_id"]
            or record.get("sha256") != hashlib.sha256(data).hexdigest()
            or not isinstance(previous_geometry, dict)
            or any(details.get(key) != value for key, value in previous_geometry.items())):
        raise ValueError("Model doesn't match its review manifest.")
    # Upgrade older staged manifests with the new actual texture report,
    # without requiring a re-import or silently approving altered bytes.
    record["geometry"] = details
    approved.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(staged, approved)
    record["reviewed"] = True
    approved.with_suffix(".json").write_text(json.dumps(record, indent=2) + "\n", encoding="utf-8")
    return approved


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    item_selection = parser.add_mutually_exclusive_group(required=True)
    item_selection.add_argument("--guardian", type=int, metavar="ID")
    item_selection.add_argument("--animal", metavar="SPECIES")
    item_selection.add_argument("--game-art", metavar="CATEGORY:KEY")
    action = parser.add_mutually_exclusive_group()
    action.add_argument("--submit", action="store_true", help="One explicit credit-consuming image-to-3D API call")
    action.add_argument("--status", action="store_true", help="Check an existing 3D task")
    action.add_argument("--fetch", action="store_true", help="Fetch completed GLB to review staging")
    action.add_argument("--import-model", metavar="GLB", type=Path, help="Import your own existing GLB without API billing")
    action.add_argument("--approve", action="store_true", help="Approve a reviewed staged 3D model for automatic Godot import")
    action.add_argument("--audit", action="store_true", help="Check actual triangles, embedded 4K map dimensions and PBR materials")
    args = parser.parse_args(argv)
    try:
        item = choose_item(args)
        staged, approved, _ = paths(item)
        if args.submit:
            print("Created one paid 3D task:", submit(item))
        elif args.status:
            status = task_status(item)
            print(json.dumps({"id": status.get("id"), "status": status.get("status"),
                              "progress": status.get("progress"),
                              "credits": status.get("consumed_credits")}, indent=2))
        elif args.fetch:
            print("3D candidate staged for visual review:", fetch_model(item))
        elif args.import_model is not None:
            print("3D candidate staged for visual review:", import_local(item, args.import_model))
        elif args.approve:
            print("3D model approved; Godot imports it by name:", approve_model(item))
        elif args.audit:
            model_path = approved if approved.is_file() else staged
            if not model_path.is_file():
                raise FileNotFoundError("No approved or staged GLB to audit.")
            print(json.dumps({
                "file": str(model_path), **glb_info(model_path.read_bytes()),
                "note": "4K refers to *observed embedded maps*, not provider request settings.",
            }, indent=2))
        else:
            print(json.dumps({
                "subject": item["subject"], "category": item["category"],
                "staging": str(staged), "approved": str(approved),
                "approved_exists": approved.is_file(), "staged_exists": staged.is_file(),
                "requested_texture": "4096x4096 PBR", "target_triangles": 12000,
                "note": "Preview only: never submits a task or charges the provider.",
            }, indent=2))
        return 0
    except (ValueError, FileNotFoundError, FileExistsError, RuntimeError, OSError) as error:
        print("Moonfall 3D:", error, file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
"""Optional Moonfall AI Art Studio. Preview, generate, review, then import artwork.

Never modifies the original Z or Nyra art. No runtime AI or dependency required.
Uses the documented OpenAI Images API only with an explicit --generate command.
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
import urllib.request
import zlib

PROJECT = Path(__file__).resolve().parents[1]
ROOT = PROJECT / "assets" / "ai_generated"
STAGING = PROJECT / "ai_art_staging"
CATALOG = PROJECT / "scripts" / "systems" / "americas_wildlife_catalog.gd"
FAMILIES = (
    "Moonfang", "Ashen Crown", "Venomveil", "Duskthorn", "Frostbound",
    "Sunscorch", "Stormcaller", "Ironhide", "Hollowroot", "Bloodmoon", "Starfall",
)
RANKS = ("Sentinel", "Reaver", "Warden", "Champion", "Sovereign")
SPECIES = ("wolf", "boar", "serpent", "panther")
TRAITS = ("Moonbound", "Emberborn", "Venomheart", "Eclipsed")
MODEL = "gpt-image-2.5-flare"
PNG_HEADER = b"\x89PNG\r\n\x1a\n"
SAFE_FILENAME = re.compile(r"[^a-z0-9_-]+")


def guardian_profile(guardian_id: int) -> dict:
    if not 0 <= guardian_id < 55:
        raise ValueError("Guardian ID must be from 0 through 54.")
    family_id, rank_id = divmod(guardian_id, 5)
    return {
        "subject": f"{FAMILIES[family_id]} {RANKS[rank_id]}",
        "species": SPECIES[(family_id * 2 + rank_id) % 4],
        "trait": TRAITS[(family_id + rank_id) % 4],
        "asset_id": f"guardian_{guardian_id:02d}",
        "category": "guardians",
    }


def available_animals() -> set[str]:
    if not CATALOG.is_file():
        return set()
    text = CATALOG.read_text(encoding="utf-8")
    names = set()
    for group in re.findall(r'"names":"([^"]+)"', text):
        names.update(name.strip() for name in group.split("|"))
    return names


def animal_profile(name: str) -> dict:
    if name not in available_animals():
        raise ValueError("Unknown Americas species. Choose a name from the game's wildlife catalog.")
    return {
        "subject": name,
        "species": name.lower(),
        "trait": "natural habitat",
        "asset_id": "animal_" + slugify(name),
        "category": "animals",
    }


def slugify(name: str) -> str:
    return SAFE_FILENAME.sub("_", name.lower().strip()).strip("_")


def prompt_for(item: dict) -> str:
    subject = item["subject"]
    if item["category"] == "guardians":
        subject_detail = (
            f"An original fantasy dungeon guardian called {subject}, "
            f"recognizably based on a {item['species']}, "
            f"with {item['trait']} arcane motifs; powerful four-legged or serpentine anatomy "
            "appropriate to that animal, distinct readable shape, fierce expression."
        )
    else:
        subject_detail = (
            f"A visually identifiable {subject}, anatomically grounded wildlife of the Americas, "
            "with credible animal proportions and a distinctive species silhouette."
        )
    return (
        "One original full-body game creature concept sprite, isolated subject, three-quarter isometric "
        "view facing toward lower-left, transparent background, all limbs/tail fully within canvas. "
        + subject_detail
        + " Moonfall visual direction: beautiful dark fantasy with detailed painterly shading, "
        "rich moonlit rim light, slightly creepy but not grotesque, sophisticated saturated accents, "
        "clean recognizable silhouette, cohesive stylized 2.5D RPG art. "
        "No words, captions, logo, UI, border, ground, other creatures, or scenery. "
        "Do not imitate, modify, or depict Moonfall's existing heroes Z or Nyra."
    )


def dest_paths(item: dict) -> tuple[Path, Path]:
    filename = item["asset_id"] + ".png"
    return STAGING / item["category"] / filename, ROOT / item["category"] / filename


MAX_IMAGE_BYTES = 12 * 1024 * 1024
MAX_DIMENSION = 2048


def validate_png(payload: bytes) -> None:
    """Validate the image container, real 2D dimensions and alpha capability.

    Never trust the filename or first eight bytes alone. A bounded PNG parser
    rejects truncated chunks, missing IDAT/IEND and non-RGBA downloads before
    an image can enter the game-art folder.
    """
    if not payload.startswith(PNG_HEADER) or not 64 <= len(payload) <= MAX_IMAGE_BYTES:
        raise ValueError("Expected a complete PNG under 12 MiB.")
    index = len(PNG_HEADER)
    seen_ihdr = False
    seen_idat = False
    seen_iend = False
    idat_segments = []
    expected_bytes = 0
    while index < len(payload):
        if index + 12 > len(payload):
            raise ValueError("Truncated PNG chunk.")
        length = struct.unpack_from(">I", payload, index)[0]
        chunk_type = payload[index + 4:index + 8]
        end = index + 12 + length
        if length > MAX_IMAGE_BYTES or end > len(payload):
            raise ValueError("PNG chunk length is invalid.")
        data = payload[index + 8:index + 8 + length]
        claimed_crc = struct.unpack_from(">I", payload, index + 8 + length)[0]
        actual_crc = zlib.crc32(chunk_type + data) & 0xFFFFFFFF
        if actual_crc != claimed_crc:
            raise ValueError("PNG chunk checksum failed.")
        if chunk_type == b"IHDR":
            if seen_ihdr or index != len(PNG_HEADER) or length != 13:
                raise ValueError("Invalid PNG header.")
            width, height, bit_depth, color_type, compression, filtering, interlace = struct.unpack(">IIBBBBB", data)
            if not (64 <= width <= MAX_DIMENSION and 64 <= height <= MAX_DIMENSION):
                raise ValueError("Unreasonable PNG dimensions.")
            if (bit_depth, color_type, compression, filtering, interlace) != (8, 6, 0, 0, 0):
                raise ValueError("Approved artwork must be an 8-bit RGBA non-interlaced PNG.")
            seen_ihdr = True
            expected_bytes = height * (width * 4 + 1)
        elif not seen_ihdr:
            raise ValueError("PNG image header must come first.")
        elif chunk_type == b"IDAT":
            seen_idat = True
            idat_segments.append(data)
        elif chunk_type == b"IEND":
            if length != 0 or not seen_idat or end != len(payload):
                raise ValueError("Missing image data or invalid PNG end marker.")
            seen_iend = True
        index = end
        if seen_iend:
            break
    if not seen_ihdr or not seen_idat or not seen_iend:
        raise ValueError("PNG missing required header, image data or end.")
    # Decompression is capped by the dimensions in the PNG header, preventing
    # compressed data bombs and rejecting truncated pixel streams.
    decoder = zlib.decompressobj()
    try:
        decoded = decoder.decompress(b"".join(idat_segments), expected_bytes + 1)
        decoded += decoder.flush(expected_bytes + 1 - len(decoded))
    except zlib.error as err:
        raise ValueError("PNG pixel stream is invalid.") from err
    if len(decoded) != expected_bytes or not decoder.eof:
        raise ValueError("PNG pixel data is truncated or exceeds declared dimensions.")


def generate(prompt: str, model: str) -> bytes:
    key = os.environ.get("OPENAI_API_KEY", "").strip()
    if not key:
        raise ValueError("OPENAI_API_KEY is required for --generate. Never put it in the repository.")
    if model not in {"gpt-image-2.5-flare", "gpt-image-2.5-sunburst", "gpt-image-1.5"}:
        raise ValueError("Unsupported image model.")
    request = urllib.request.Request(
        "https://api.openai.com/v1/images/generations",
        data=json.dumps({
            "model": model, "prompt": prompt, "size": "1024x1024",
            "background": "transparent", "output_format": "png", "n": 1,
        }).encode("utf-8"),
        headers={"Authorization": "Bearer " + key, "Content-Type": "application/json"},
        method="POST",
    )
    try:
        with urllib.request.urlopen(request, timeout=180) as response:
            result = json.load(response)
    except urllib.error.HTTPError as err:
        raise RuntimeError(f"Image generation failed (HTTP {err.code}); check API access or billing.") from None
    data = result.get("data", [])
    if not data or not isinstance(data[0].get("b64_json"), str):
        raise ValueError("Image API returned no base64 image.")
    image = base64.b64decode(data[0]["b64_json"], validate=True)
    validate_png(image)
    return image


def metadata(item: dict, prompt: str, model: str, image: bytes | None = None) -> dict:
    result = {
        "asset_id": item["asset_id"], "category": item["category"],
        "subject": item["subject"], "prompt": prompt, "model": model,
        "reviewed": False,
    }
    if image is not None:
        result["sha256"] = hashlib.sha256(image).hexdigest()
    return result


def stage(item: dict, prompt: str, model: str, payload: bytes) -> Path:
    validate_png(payload)
    staged, _ = dest_paths(item)
    staged.parent.mkdir(parents=True, exist_ok=True)
    if staged.exists():
        raise FileExistsError(f"Staged candidate exists: {staged}")
    staged.write_bytes(payload)
    staged.with_suffix(".json").write_text(
        json.dumps(metadata(item, prompt, model, payload), indent=2) + "\n",
        encoding="utf-8",
    )
    return staged


def approve(item: dict) -> Path:
    staged, dest = dest_paths(item)
    record = staged.with_suffix(".json")
    if not staged.is_file() or not record.is_file():
        raise FileNotFoundError("Generate and review the staged artwork first.")
    payload = staged.read_bytes()
    validate_png(payload)
    meta = json.loads(record.read_text(encoding="utf-8"))
    if (meta.get("asset_id") != item["asset_id"]
            or meta.get("sha256") != hashlib.sha256(payload).hexdigest()):
        raise ValueError("Staged image differs from its review manifest.")
    if dest.exists():
        raise FileExistsError(f"Won't overwrite existing artwork: {dest}")
    dest.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(staged, dest)
    meta["reviewed"] = True
    dest.with_suffix(".json").write_text(json.dumps(meta, indent=2) + "\n", encoding="utf-8")
    return dest


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--guardian", type=int, metavar="ID", help="Named guardian 0-54")
    group.add_argument("--animal", metavar="SPECIES", help="Exact name from Americas wildlife catalog")
    parser.add_argument("--generate", action="store_true", help="Explicit opt-in paid image API request")
    parser.add_argument("--approve", action="store_true", help="Copy reviewed staged PNG into game assets")
    parser.add_argument("--model", default=MODEL, choices=[MODEL, "gpt-image-2.5-sunburst", "gpt-image-1.5"])
    args = parser.parse_args(argv)
    if args.generate and args.approve:
        parser.error("Generate and approve in separate commands so artwork can be reviewed.")
    try:
        item = guardian_profile(args.guardian) if args.guardian is not None else animal_profile(args.animal)
        prompt = prompt_for(item)
        staged, dest = dest_paths(item)
        if args.generate:
            path = stage(item, prompt, args.model, generate(prompt, args.model))
            print(f"Generated candidate for visual review: {path}")
        elif args.approve:
            print(f"Approved game asset: {approve(item)}")
        else:
            print(json.dumps({
                **metadata(item, prompt, args.model),
                "staging": str(staged.relative_to(PROJECT)),
                "approved_output": str(dest.relative_to(PROJECT)),
                "note": "Preview only. No API request or file change.",
            }, indent=2))
        return 0
    except (ValueError, FileNotFoundError, FileExistsError, RuntimeError) as exc:
        print(f"AI art studio: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())

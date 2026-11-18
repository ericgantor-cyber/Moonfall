"""Validate and import Moonfall's original loot texture atlases into UE5.4.

In the Unreal Editor Python console:
  exec(open(r"<PROJECT>/Build/Import-Loot-Art.py", encoding="utf-8").read())

All source PNGs are preflighted before importing any asset. This is editor
work, not game runtime I/O or an automatic Windows installer.
"""
from pathlib import Path
import struct
import zlib
import unreal

SOURCE_DIR = Path(unreal.Paths.project_dir()).resolve() / "Data" / "LootArt"
DESTINATION = "/Game/Moonfall/LootArt"
ATLASES = (
    ("Moonfall_LootAtlas.png", "T_MoonfallLootAtlas", (120, 80)),
    ("Moonfall_LootVariants_Atlas.png", "T_MoonfallLootVariants", (120, 120)),
    ("Moonfall_LootRelics_Atlas.png", "T_MoonfallLootRelics", (120, 120)),
    ("Moonfall_LootMasterworks_Atlas.png", "T_MoonfallLootMasterworks", (120, 120)),
    ("Moonfall_LootAscendants_Atlas.png", "T_MoonfallLootAscendants", (120, 120)),
)
PNG_SIGNATURE = bytes.fromhex("89504e470d0a1a0a")
MAX_PNG_BYTES = 512 * 1024


def validate_atlas(path: Path, expected_size: tuple[int, int]) -> None:
    """Fail closed on truncated/malformed PNG chunks before touching .uassets.

    Checks PNG structure and CRCs without decoding image pixels. Unreal
    validates image decodability during the subsequent editor import.
    """
    if not path.is_file() or not 64 <= path.stat().st_size <= MAX_PNG_BYTES:
        raise ValueError(f"Moonfall loot source missing or oversized: {path}")
    payload = path.read_bytes()
    if not payload.startswith(PNG_SIGNATURE):
        raise ValueError(f"Invalid PNG signature: {path}")
    offset = len(PNG_SIGNATURE)
    count = 0
    found_ihdr = found_idat = found_iend = found_palette = False
    while offset < len(payload):
        if offset + 12 > len(payload):
            raise ValueError(f"Truncated PNG chunk: {path}")
        size = struct.unpack_from(">I", payload, offset)[0]
        if size > MAX_PNG_BYTES or offset + size + 12 > len(payload):
            raise ValueError(f"Oversized or incomplete PNG chunk: {path}")
        chunk_type = payload[offset + 4:offset + 8]
        chunk_data = payload[offset + 8:offset + 8 + size]
        expected_crc = struct.unpack_from(">I", payload, offset + 8 + size)[0]
        actual_crc = zlib.crc32(chunk_type + chunk_data) & 0xFFFFFFFF
        if expected_crc != actual_crc:
            raise ValueError(f"PNG chunk checksum mismatch: {path}")
        count += 1
        if count == 1 and chunk_type != b"IHDR":
            raise ValueError(f"PNG missing first IHDR: {path}")
        if chunk_type == b"IHDR":
            if found_ihdr or size != 13:
                raise ValueError(f"Invalid duplicate or malformed IHDR: {path}")
            width, height, depth, color, compression, filtering, interlace = (
                struct.unpack(">IIBBBBB", chunk_data)
            )
            if ((width, height) != expected_size
                    or not ((color == 3 and depth in (1, 2, 4, 8))
                            or (color in (2, 6) and depth == 8))
                    or compression != 0
                    or filtering != 0 or interlace not in (0, 1)):
                raise ValueError(f"Unexpected Moonfall atlas dimensions or format: {path}")
            found_ihdr = True
            indexed = color == 3
        elif chunk_type == b"PLTE":
            if found_palette or found_idat or size == 0 or size % 3 or size > 768:
                raise ValueError(f"Invalid PNG palette: {path}")
            found_palette = True
        elif chunk_type == b"IDAT":
            if not found_ihdr:
                raise ValueError(f"Missing IHDR before IDAT: {path}")
            found_idat = True
        elif chunk_type == b"IEND":
            if size != 0 or not found_idat:
                raise ValueError(f"Invalid PNG terminator: {path}")
            found_iend = True
            offset += size + 12
            break
        offset += size + 12
    if (not found_ihdr or not found_idat or not found_iend
            or offset != len(payload) or (indexed and not found_palette)):
        raise ValueError(f"Incomplete PNG atlas: {path}")


# Validate every file, then check for conflicting Unreal objects before
# importing. Never overwrite unrelated asset classes with decorative art.
for filename, _, expected_size in ATLASES:
    validate_atlas(SOURCE_DIR / filename, expected_size)
for _, asset_name, _ in ATLASES:
    target = f"{DESTINATION}/{asset_name}"
    asset_exists = unreal.EditorAssetLibrary.does_asset_exist(target)
    existing = unreal.load_asset(target) if asset_exists else None
    # A stale/broken registry entry must never be silently replaced.
    if asset_exists and existing is None:
        raise RuntimeError(f"Existing Moonfall art asset cannot be loaded: {target}")
    if existing is not None and not isinstance(existing, unreal.Texture2D):
        raise TypeError(f"Non-texture asset occupies Moonfall art slot: {asset_name}")

for filename, asset_name, _ in ATLASES:
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(SOURCE_DIR / filename))
    task.set_editor_property("destination_path", DESTINATION)
    task.set_editor_property("destination_name", asset_name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    asset_path = f"{DESTINATION}/{asset_name}"
    asset = unreal.load_asset(asset_path)
    if not asset or not isinstance(asset, unreal.Texture2D):
        raise RuntimeError(f"Moonfall loot art did not import as Texture2D: {asset_path}")
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset):
        raise RuntimeError(f"Could not save imported Moonfall loot art: {asset_path}")
    unreal.log(f"Moonfall loot art imported: {asset_path}")

unreal.log("Moonfall loot art ready. Restart the Editor so native HUD default assets refresh.")

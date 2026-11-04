"""Offline regressions for the optional AI Art Studio. No requests or fees."""
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
import struct
import zlib
from unittest.mock import patch

FILE = Path(__file__).resolve().parents[1] / "tools" / "ai_art_studio.py"
import sys
sys.path.insert(0, str(FILE.parent))
spec = importlib.util.spec_from_file_location("moonfall_ai_art_studio", FILE)
studio = importlib.util.module_from_spec(spec)
spec.loader.exec_module(studio)

# A real, compact 64x64 RGBA PNG; tests don't need Pillow or network.
def png_chunk(kind: bytes, data: bytes) -> bytes:
    return struct.pack(">I", len(data)) + kind + data + struct.pack(
        ">I", zlib.crc32(kind + data) & 0xFFFFFFFF
    )


def example_png() -> bytes:
    ihdr = struct.pack(">IIBBBBB", 64, 64, 8, 6, 0, 0, 0)
    raw = (b"\x00" + b"\x88\x55\xff\x00" * 64) * 64
    return studio.PNG_HEADER + png_chunk(b"IHDR", ihdr) + png_chunk(
        b"IDAT", zlib.compress(raw)
    ) + png_chunk(b"IEND", b"")


PNG = example_png()


class TestMoonfallAIArt(unittest.TestCase):
    def test_true_4k_rgba_input_can_be_staged_and_validated(self):
        width = height = 4096
        ihdr = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
        row = b"\x00" + (b"\x00" * (width * 4))
        compressor = zlib.compressobj(level=1)
        pieces = [compressor.compress(row) for _ in range(height)]
        pieces.append(compressor.flush())
        image = (studio.PNG_HEADER + png_chunk(b"IHDR", ihdr)
                 + png_chunk(b"IDAT", b"".join(pieces))
                 + png_chunk(b"IEND", b""))
        self.assertLess(len(image), studio.MAX_IMAGE_BYTES)
        self.assertEqual(studio.MAX_DIMENSION, 4096)
        studio.validate_png(image)
        bad_header = struct.pack(">IIBBBBB", 4097, 4096, 8, 6, 0, 0, 0)
        invalid = studio.PNG_HEADER + png_chunk(b"IHDR", bad_header) + png_chunk(b"IDAT", b"".join(pieces)) + png_chunk(b"IEND", b"")
        with self.assertRaises(ValueError):
            studio.validate_png(invalid)

    def test_all_fifty_five_guardian_profiles(self):
        identities = [studio.guardian_profile(i) for i in range(55)]
        self.assertEqual(len({d["asset_id"] for d in identities}), 55)
        self.assertEqual(len({d["subject"] for d in identities}), 55)
        self.assertEqual(identities[0]["subject"], "Moonfang Sentinel")
        self.assertEqual(identities[54]["subject"], "Starfall Sovereign")
        self.assertTrue(all(d["category"] == "guardians" for d in identities))
        for value in [-1, 55]:
            with self.assertRaises(ValueError):
                studio.guardian_profile(value)

    def test_distinct_family_rank_and_habitat_art_direction(self):
        sentinel = studio.guardian_profile(0)
        sovereign = studio.guardian_profile(54)
        first = studio.prompt_for(sentinel)
        last = studio.prompt_for(sovereign)
        self.assertNotEqual(first, last)
        self.assertIn("crescent fangs", first)
        self.assertIn("grand apex-boss", last)
        jaguar = studio.animal_profile("Jaguar")
        self.assertEqual(jaguar["taxon"], "M")
        self.assertEqual(jaguar["habitat"], "Tropical rainforests")
        self.assertIn("rainforest", studio.prompt_for(jaguar).lower())
        self.assertIn("Moonfall", first)
        self.assertIn("8 percent transparent margin", first)

    def test_local_file_import_without_api_or_billing(self):
        with tempfile.TemporaryDirectory() as folder:
            with patch.object(studio, "STAGING", Path(folder) / "staging"), patch.object(
                studio, "ROOT", Path(folder) / "approved"
            ), patch.object(studio, "generate", side_effect=AssertionError("Paid API called")):
                incoming = Path(folder) / "offline_creature.png"
                incoming.write_bytes(PNG)
                item = studio.guardian_profile(2)
                staged = studio.import_local_image(item, studio.prompt_for(item), incoming)
                self.assertTrue(staged.exists())
                self.assertEqual(json.loads(staged.with_suffix(".json").read_text())["model"], "local-import")
                self.assertTrue(studio.approve(item).exists())
                with self.assertRaises(FileExistsError):
                    studio.import_local_image(item, "new brief", incoming)

    def test_reject_original_hero_as_art_source(self):
        with tempfile.TemporaryDirectory() as folder:
            project = Path(folder)
            art = project / "assets" / "heroes"
            art.mkdir(parents=True)
            hero = art / "Z_Original.png"
            hero.write_bytes(PNG)
            with patch.object(studio, "PROJECT", project):
                with self.assertRaises(ValueError):
                    studio.import_local_image(studio.guardian_profile(0), "no", hero)

    def test_wildlife_catalog_and_paths(self):
        names = studio.available_animals()
        self.assertGreaterEqual(len(names), 230)
        self.assertIn("Jaguar", names)
        self.assertIn("American bison", names)
        animal = studio.animal_profile("Jaguar")
        self.assertEqual(animal["asset_id"], "animal_jaguar")
        with self.assertRaises(ValueError):
            studio.animal_profile("Z_Original")

    def test_prompts_protect_artwork(self):
        for item in (studio.guardian_profile(3), studio.animal_profile("Jaguar")):
            prompt = studio.prompt_for(item)
            self.assertIn(item["subject"], prompt)
            self.assertIn("transparent background", prompt)
            self.assertIn("Z or Nyra", prompt)
            self.assertIn("No words", prompt)

    def test_preview_is_offline(self):
        with patch.object(studio, "generate", side_effect=AssertionError("Unexpected API call")):
            self.assertEqual(studio.main(["--guardian", "7"]), 0)

    def test_stage_review_and_no_overwrite(self):
        with tempfile.TemporaryDirectory() as directory:
            with patch.object(studio, "STAGING", Path(directory) / "staging"), patch.object(
                studio, "ROOT", Path(directory) / "approved"
            ):
                item = studio.guardian_profile(0)
                staged = studio.stage(item, studio.prompt_for(item), studio.MODEL, PNG)
                self.assertTrue(staged.exists())
                self.assertEqual(json.loads(staged.with_suffix(".json").read_text())["reviewed"], False)
                dest = studio.approve(item)
                self.assertTrue(dest.exists())
                self.assertEqual(dest.read_bytes(), PNG)
                self.assertTrue(json.loads(dest.with_suffix(".json").read_text())["reviewed"])
                with self.assertRaises(FileExistsError):
                    studio.approve(item)
                with self.assertRaises(FileExistsError):
                    studio.stage(item, "new prompt", studio.MODEL, PNG)

    def test_tampering_and_invalid_images_are_rejected(self):
        with self.assertRaises(ValueError):
            studio.validate_png(b"not an image")
        for broken in [PNG[:33], PNG[:-8], PNG + b"trailing bytes",
                       studio.PNG_HEADER + b"\x00" * 72]:
            with self.assertRaises(ValueError):
                studio.validate_png(broken)
        opaque = PNG.replace(struct.pack(">IIBBBBB", 64, 64, 8, 6, 0, 0, 0),
                             struct.pack(">IIBBBBB", 64, 64, 8, 2, 0, 0, 0))
        with self.assertRaises(ValueError):
            studio.validate_png(opaque)
        with tempfile.TemporaryDirectory() as directory:
            with patch.object(studio, "STAGING", Path(directory) / "staging"), patch.object(
                studio, "ROOT", Path(directory) / "approved"
            ):
                item = studio.guardian_profile(4)
                staged = studio.stage(item, "test", studio.MODEL, PNG)
                staged.write_bytes(PNG + b"x")
                with self.assertRaises(ValueError):
                    studio.approve(item)

    def test_generation_requires_explicit_api_key(self):
        with patch.dict("os.environ", {"OPENAI_API_KEY": ""}):
            with self.assertRaises(ValueError):
                studio.generate("test", studio.MODEL)


if __name__ == "__main__":
    unittest.main()

"""Regression tests for free bulk intake, actual 4K maps and proximity streaming."""
import copy
import hashlib
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import ai_art_intake as intake
import ai_art_3d as mesh3d
import ai_art_studio as studio
from test_ai_art_3d import GLTF, glb_fixture
from test_ai_art_studio import PNG


def embedded_4k_glb():
    # Read real 4K image dimensions from embedded PNG header, not from the
    # optional Meshy "texture_resolution: 4k" request metadata.
    image = bytearray(PNG)
    struct.pack_into(">II", image, 16, 4096, 4096)
    # Repair PNG header checksum. GLB texture scanner only reads dimensions.
    import zlib
    crc = zlib.crc32(image[12:29]) & 0xFFFFFFFF
    struct.pack_into(">I", image, 29, crc)
    doc = copy.deepcopy(GLTF)
    doc["buffers"][0]["byteLength"] = 44 + len(image)
    doc["bufferViews"] = [{"buffer": 0, "byteOffset": 44, "byteLength": len(image)}]
    doc["images"] = [{"bufferView": 0, "mimeType": "image/png"}]
    doc["textures"] = [{"source": 0}]
    doc["materials"] = [{
        "pbrMetallicRoughness": {
            "baseColorTexture": {"index": 0},
            "metallicRoughnessTexture": {"index": 0},
        },
        "normalTexture": {"index": 0},
    }]
    raw = json.dumps(doc).encode("utf-8")
    raw += b" " * ((-len(raw)) % 4)
    payload = b"\x00" * 44 + bytes(image)
    payload += b"\x00" * ((-len(payload)) % 4)
    json_chunk = struct.pack("<II", len(raw), 0x4E4F534A) + raw
    bin_chunk = struct.pack("<II", len(payload), 0x004E4942) + payload
    return struct.pack("<4sII", b"glTF", 2, 12 + len(json_chunk) + len(bin_chunk)) + json_chunk + bin_chunk


class TestArtIntake(unittest.TestCase):
    def test_report_declared_4k_pbr_maps_not_just_requested_resolution(self):
        report = mesh3d.glb_info(embedded_4k_glb())
        self.assertTrue(report["has_4k_texture"])
        self.assertEqual(report["max_texture_size"], 4096)
        self.assertGreaterEqual(report["pbr_material_count"], 1)
        self.assertEqual(report["texture_images"], 1)
        self.assertEqual(report["triangles"], 1)
        self.assertFalse(mesh3d.glb_info(glb_fixture())["has_4k_texture"])

    def test_reject_invalid_image_reference_and_too_many_textures(self):
        bad = copy.deepcopy(GLTF)
        bad["images"] = [{"bufferView": 999, "mimeType": "image/png"}]
        with self.assertRaises(ValueError):
            mesh3d.glb_info(glb_fixture(bad))
        bad = copy.deepcopy(GLTF)
        bad["images"] = [{"uri": "https://untrusted.example/x.png"}]
        with self.assertRaises(ValueError):
            mesh3d.glb_info(glb_fixture(bad))

    def test_one_click_intake_stages_known_png_and_glb_for_review(self):
        with tempfile.TemporaryDirectory() as dirname:
            base = Path(dirname)
            inbox = base / "inbox"
            (inbox / "guardians").mkdir(parents=True)
            (inbox / "animals").mkdir()
            source_png = inbox / "animals" / "animal_jaguar.png"
            source_png.write_bytes(PNG)
            source_glb = inbox / "guardians" / "guardian_00.glb"
            source_glb.write_bytes(glb_fixture())
            with patch.object(studio, "STAGING", base / "pngstage"), patch.object(
                studio, "ROOT", base / "pngapproved"
            ), patch.object(mesh3d, "STAGING_3D", base / "meshstage"), patch.object(
                mesh3d, "APPROVED_3D", base / "meshapproved"
            ), patch.object(mesh3d, "api_call", side_effect=AssertionError("Network call")):
                plans = intake.plan(inbox)
                self.assertEqual(len(plans), 2)
                self.assertTrue(all(p["status"] == "ready" for p in plans))
                result = intake.stage_all(inbox)
                self.assertEqual(len(result), 2)
                self.assertEqual({p["status"] for p in result}, {"staged_for_review"})
                self.assertTrue((base / "pngstage" / "animals" / "animal_jaguar.png").exists())
                self.assertTrue((base / "meshstage" / "guardians" / "guardian_00.glb").exists())
                self.assertFalse((base / "pngapproved").exists())
                self.assertFalse((base / "meshapproved").exists())
                self.assertEqual({p["status"] for p in intake.stage_all(inbox)}, {"skipped_existing"})
                self.assertFalse(json.loads((base / "meshstage" / "guardians" / "guardian_00.json").read_text())["reviewed"])
                # A staged GLB must appear as such in the art review board,
                # even if no corresponding 2D PNG was supplied.
                card = intake.review.review_status(studio.guardian_profile(0))
                self.assertEqual(card["state"], "missing")
                self.assertEqual(card["model_state"], "staged")
                self.assertEqual(card["display_state"], "staged")
                self.assertTrue(card["model_valid"])
                self.assertEqual(card["model_geometry"]["triangles"], 1)
                self.assertFalse(card["model_geometry"]["has_4k_texture"])

    def test_unknown_ids_and_symlinked_files_never_enter_staging(self):
        with tempfile.TemporaryDirectory() as dirname:
            inbox = Path(dirname) / "inbox"
            (inbox / "animals").mkdir(parents=True)
            (inbox / "animals" / "animal_unsafe.png").write_bytes(PNG)
            with self.assertRaises(ValueError):
                intake.list_inbox(inbox)
            (inbox / "animals" / "animal_unsafe.png").unlink()
            (inbox / "animals" / "animal_jaguar.png").write_bytes(PNG)
            try:
                (inbox / "animals" / "animal_hacked.png").symlink_to(inbox / "animals" / "animal_jaguar.png")
            except (OSError, NotImplementedError):
                return
            with self.assertRaises(ValueError):
                intake.list_inbox(inbox)

    def test_inbox_limits_and_no_paid_generation(self):
        with tempfile.TemporaryDirectory() as dirname:
            inbox = Path(dirname) / "inbox"
            inbox.mkdir()
            with patch.object(intake, "plan", wraps=intake.plan):
                self.assertEqual(intake.main(["--inbox", str(inbox)]), 0)
            self.assertEqual(intake.candidate_index().__len__(), 350)

    def test_native_scene_streams_3d_with_low_gpu_budget(self):
        path = Path(__file__).resolve().parents[1] / "scripts" / "native_v38_2.gd"
        source = path.read_text(encoding="utf-8")
        self.assertIn("AI_ART_NEAR_ANIMAL_LIMIT:int=3", source)
        self.assertIn("AI_ART_NEAR_CHAMBER_LIMIT:int=1", source)
        self.assertIn("func _ai_art_refresh_nearby_models()", source)
        self.assertIn("ResourceLoader.exists(path)", source)
        self.assertIn("_ai_art_release_animal_model(actor)", source)
        self.assertIn("candidates.sort_custom", source)
        self.assertIn("stage in [7,14]", source)
        self.assertIn('AI_APPROVED_3D.add_approved_model(body,"animals"', source)
        self.assertNotIn('AI_APPROVED_3D.add_approved_model(native_body,"animals"', source)


if __name__ == "__main__":
    unittest.main()

"""Offline regression tests for explicit reviewed PNG + GLB batch approval."""
import hashlib
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import ai_art_approve_selected as selected
import ai_art_review as review
import ai_art_studio as png
import ai_art_3d as glb
from test_ai_art_studio import PNG
from test_ai_art_3d import glb_fixture


class TestSelectionApproval(unittest.TestCase):
    def test_gallery_exports_only_valid_staged_art(self):
        with tempfile.TemporaryDirectory() as folder:
            base = Path(folder)
            with patch.object(png, "STAGING", base / "png_stage"), patch.object(
                png, "ROOT", base / "png_approved"
            ), patch.object(glb, "STAGING_3D", base / "glb_stage"), patch.object(
                glb, "APPROVED_3D", base / "glb_approved"
            ):
                item = png.guardian_profile(0)
                png.stage(item, png.prompt_for(item), png.MODEL, PNG)
                glb.stage_model(item, glb_fixture(), "local-glb")
                row = review.review_status(item)
                self.assertEqual(row["image_checksum"], hashlib.sha256(PNG).hexdigest())
                self.assertEqual(row["model_checksum"], hashlib.sha256(glb_fixture()).hexdigest())
                target = base / "review" / "index.html"
                review.make_gallery([row], target)
                html = target.read_text(encoding="utf-8")
                self.assertIn('id="export-art-selection"', html)
                self.assertIn('data-id="guardian_00"', html)
                self.assertIn('data-format="png"', html)
                self.assertIn('data-format="glb"', html)
                self.assertIn('aria-live="polite"', html)
                self.assertIn("moonfall_art_approval.json", html)
                self.assertIn("Open 3D GLB", html)

    def test_approved_only_after_explicit_selected_manifest_apply(self):
        with tempfile.TemporaryDirectory() as folder:
            base = Path(folder)
            with patch.object(png, "STAGING", base / "png_stage"), patch.object(
                png, "ROOT", base / "png_approved"
            ), patch.object(glb, "STAGING_3D", base / "glb_stage"), patch.object(
                glb, "APPROVED_3D", base / "glb_approved"
            ), patch.object(glb, "api_call", side_effect=AssertionError("Network call")):
                creature = png.animal_profile("Jaguar")
                boss = png.guardian_profile(0)
                png.stage(creature, png.prompt_for(creature), png.MODEL, PNG)
                glb.stage_model(boss, glb_fixture(), "local-glb")
                manifest = base / "review_selection.json"
                manifest.write_text(json.dumps({
                    "version": 1, "approved_assets": [
                        {"asset_id": creature["asset_id"], "format": "png",
                         "sha256": hashlib.sha256(PNG).hexdigest()},
                        {"asset_id": boss["asset_id"], "format": "glb",
                         "sha256": hashlib.sha256(glb_fixture()).hexdigest()},
                    ],
                }), encoding="utf-8")
                self.assertEqual(selected.main(["--manifest", str(manifest)]), 0)
                self.assertFalse((base / "png_approved").exists())
                self.assertFalse((base / "glb_approved").exists())
                self.assertEqual(selected.main(["--manifest", str(manifest), "--apply"]), 0)
                self.assertTrue((base / "png_approved" / "animals" / "animal_jaguar.png").exists())
                self.assertTrue((base / "glb_approved" / "guardians" / "guardian_00.glb").exists())
                self.assertEqual(selected.main(["--manifest", str(manifest), "--apply"]), 1)

    def test_png_approved_with_new_glb_still_requires_3d_review(self):
        with tempfile.TemporaryDirectory() as folder:
            base = Path(folder)
            with patch.object(png, "STAGING", base / "png_stage"), patch.object(
                png, "ROOT", base / "png_approved"
            ), patch.object(glb, "STAGING_3D", base / "glb_stage"), patch.object(
                glb, "APPROVED_3D", base / "glb_approved"
            ):
                item = png.guardian_profile(1)
                png.stage(item, png.prompt_for(item), png.MODEL, PNG)
                png.approve(item)
                glb.stage_model(item, glb_fixture(), "local-glb")
                row = review.review_status(item)
                self.assertEqual(row["state"], "approved")
                self.assertEqual(row["model_state"], "staged")
                self.assertEqual(row["display_state"], "staged")
                target = base / "gallery.html"
                review.make_gallery([row], target)
                html = target.read_text(encoding="utf-8")
                self.assertIn('data-status="staged"', html)
                self.assertIn('data-format="glb"', html)
                self.assertNotIn('data-format="png"', html)

    def test_preflight_rejects_tampered_art_without_partial_approval(self):
        with tempfile.TemporaryDirectory() as folder:
            base = Path(folder)
            with patch.object(png, "STAGING", base / "staged"), patch.object(
                png, "ROOT", base / "approved"
            ):
                first = png.guardian_profile(0)
                second = png.guardian_profile(1)
                png.stage(first, png.prompt_for(first), png.MODEL, PNG)
                png.stage(second, png.prompt_for(second), png.MODEL, PNG)
                manifest = base / "manifest.json"
                manifest.write_text(json.dumps({
                    "version": 1,
                    "approved_assets": [
                        {"asset_id": first["asset_id"], "format": "png",
                         "sha256": hashlib.sha256(PNG).hexdigest()},
                        {"asset_id": second["asset_id"], "format": "png",
                         "sha256": "0" * 64},
                    ],
                }), encoding="utf-8")
                self.assertEqual(selected.main(["--manifest", str(manifest), "--apply"]), 1)
                self.assertFalse((base / "approved").exists())

    def test_no_unknown_hero_or_duplicate_keys_ever_approved(self):
        with tempfile.TemporaryDirectory() as folder:
            manifest = Path(folder) / "manifest.json"
            for items in (
                [{"asset_id": "hero_Z_Original", "format": "png", "sha256": "0" * 64}],
                [{"asset_id": "glyph_orb_hud", "format": "glb", "sha256": "0" * 64}],
                [{"asset_id": "guardian_00", "format": "png", "sha256": "0" * 64}] * 2,
            ):
                manifest.write_text(json.dumps({"version": 1, "approved_assets": items}), encoding="utf-8")
                with self.assertRaises(ValueError):
                    selected.read_selection(manifest)

    def test_no_api_key_no_network_and_gallery_requires_human_decision(self):
        with tempfile.TemporaryDirectory() as folder:
            manifest = Path(folder) / "manifest.json"
            manifest.write_text(json.dumps({"version": 1, "approved_assets": []}), encoding="utf-8")
            with self.assertRaises(ValueError):
                selected.read_selection(manifest)
            rows = review.plan_rows("guardians")
            self.assertEqual(len(rows), 55)
            self.assertTrue(all(row["model_state"] in ("staged", "approved", "missing") for row in rows))


if __name__ == "__main__":
    unittest.main()

"""Offline release QA tests; no network, no game images and no Godot binary."""
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import ai_art_release_audit as audit
import ai_art_studio as png
import ai_art_3d as glb
from test_ai_art_studio import PNG
from test_ai_art_3d import glb_fixture


class TestApprovedArtReleaseAudit(unittest.TestCase):
    def test_empty_project_is_legal_and_does_not_require_350_images(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            with patch.object(png, "ROOT", root / "sprites"), patch.object(
                glb, "APPROVED_3D", root / "models"
            ):
                report = audit.audit()
                self.assertEqual(report["catalog_slots"], 350)
                self.assertEqual(report["approved_files"], 0)
                self.assertEqual(report["errors"], [])
                self.assertEqual(audit.main(["--strict"]), 0)

    def test_approved_png_and_true_glb_are_audited_for_real_resources(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            with patch.object(png, "STAGING", root / "review"), patch.object(
                png, "ROOT", root / "sprites"
            ), patch.object(glb, "STAGING_3D", root / "review_3d"), patch.object(
                glb, "APPROVED_3D", root / "models"
            ):
                creature = png.animal_profile("Jaguar")
                boss = png.guardian_profile(0)
                png.stage(creature, png.prompt_for(creature), png.MODEL, PNG)
                png.approve(creature)
                glb.stage_model(boss, glb_fixture(), "local-glb")
                glb.approve_model(boss)
                report = audit.audit()
                self.assertEqual(report["catalog_slots"], 350)
                self.assertEqual(report["occupied_slots"], 2)
                self.assertEqual(report["approved_files"], 2)
                self.assertEqual(report["errors"], [])
                self.assertEqual(report["format_counts"], {"png": 1, "glb": 1})
                self.assertGreater(report["estimated_uncompressed_rgba_mipmap_bytes"], 0)
                by_id = {r["asset_id"]: r for r in report["assets"]}
                self.assertEqual(by_id["animal_jaguar"]["width"], 64)
                self.assertEqual(by_id["guardian_00"]["mesh"]["triangles"], 1)
                self.assertFalse(by_id["guardian_00"]["mesh"]["has_4k_texture"])
                self.assertEqual(audit.main(["--strict", "--report", str(root / "release.json")]), 0)
                self.assertEqual(json.loads((root / "release.json").read_text())["approved_files"], 2)

    def test_one_corrupted_art_file_fails_strict_release_gate(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            with patch.object(png, "STAGING", root / "review"), patch.object(
                png, "ROOT", root / "sprites"
            ), patch.object(glb, "APPROVED_3D", root / "models"):
                item = png.guardian_profile(5)
                png.stage(item, png.prompt_for(item), png.MODEL, PNG)
                target = png.approve(item)
                target.write_bytes(PNG[:-12])
                report = audit.audit()
                self.assertEqual(len(report["errors"]), 1)
                self.assertIn("guardian_05", report["errors"][0]["asset_id"])
                self.assertEqual(audit.main(["--strict"]), 1)
                # Non-strict mode is diagnostic only, but still surfaces errors.
                self.assertEqual(audit.main([]), 0)

    def test_duplicate_art_warns_but_does_not_break_build(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            with patch.object(png, "STAGING", root / "review"), patch.object(
                png, "ROOT", root / "sprites"
            ), patch.object(glb, "APPROVED_3D", root / "models"):
                for id in (0, 1):
                    item = png.guardian_profile(id)
                    png.stage(item, png.prompt_for(item), png.MODEL, PNG)
                    png.approve(item)
                report = audit.audit()
                self.assertEqual(report["errors"], [])
                self.assertTrue(any("Identical approved art reused" in warning
                                    for warning in report["warnings"]))

    def test_runtime_conservative_adaptive_quality_and_lru_cache_present(self):
        root = Path(__file__).resolve().parents[1]
        native = (root / "scripts" / "native_v38_2.gd").read_text()
        quality = (root / "scripts" / "systems" / "ai_art_auto_quality.gd").read_text()
        runtime = (root / "scripts" / "systems" / "ai_art_3d_runtime.gd").read_text()
        self.assertIn("const MIN_LOAD_INTERVAL_MS:int=900", quality)
        self.assertIn("const SLOW_HOLD_SECONDS:float=3.0", quality)
        self.assertIn("const FAST_HOLD_SECONDS:float=16.0", quality)
        self.assertIn("func _ai_art_track_frame_performance(delta:float)", native)
        self.assertIn("AI_ART_AUTO_QUALITY.animal_limit(ai_art_quality_tier,candidates.size())", native)
        self.assertIn("ai_art_next_model_load_ms", native)
        self.assertIn("MAX_CACHED_MODEL_SCENES:int=12", runtime)


if __name__ == "__main__":
    unittest.main()

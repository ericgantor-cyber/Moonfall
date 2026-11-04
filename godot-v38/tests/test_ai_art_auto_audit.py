"""Offline automatic art audit integration tests. No AI keys, web or paid calls."""
from contextlib import redirect_stdout
import hashlib
import io
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import ai_art_auto_audit as automatic
import ai_art_intake as intake
import ai_art_approve_selected as approval
import ai_art_3d as meshes
import ai_art_autorig as autorig
import ai_art_studio as studio
from test_ai_art_3d import glb_fixture
from test_ai_art_rig import animated_model
from test_ai_art_studio import PNG


class TestMoonfallArtAutoAudit(unittest.TestCase):
    def sandbox(self, root):
        from contextlib import ExitStack
        stack = ExitStack()
        stage = root / "stage"
        approved = root / "approved"
        stage_models = stage / "models"
        approved_models = approved / "models"
        stage_rigs = stage / "rigged_candidates"
        approved_rigs = approved / "rigged"
        stack.enter_context(patch.object(studio, "STAGING", stage))
        stack.enter_context(patch.object(studio, "ROOT", approved))
        stack.enter_context(patch.object(meshes, "STAGING_3D", stage_models))
        stack.enter_context(patch.object(meshes, "APPROVED_3D", approved_models))
        stack.enter_context(patch.object(autorig, "CANDIDATES", stage_rigs))
        stack.enter_context(patch.object(autorig, "APPROVED", approved_rigs))
        stack.enter_context(patch.object(automatic, "DEFAULT_REPORT_DIR", stage / "review"))
        stack.enter_context(patch.object(meshes, "api_call", side_effect=AssertionError("Paid network access")))
        return stack

    def test_empty_catalog_is_not_an_error(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            with self.sandbox(root):
                report = automatic.audit()
                self.assertEqual(report["catalog_slots"], 350)
                self.assertEqual(report["approved_files"], 0)
                self.assertEqual(report["staged_files"], 0)
                self.assertEqual(report["errors"], [])
                self.assertEqual(report["summary"], "PASS")
                self.assertTrue(report["read_only"])
                self.assertEqual(report["paid_api_calls"], 0)
                with redirect_stdout(io.StringIO()):
                    self.assertEqual(automatic.main(["--strict"]), 0)
                self.assertTrue((root / "stage" / "review" / "auto_audit.html").exists())
                self.assertTrue((root / "stage" / "review" / "auto_audit.json").exists())
                self.assertTrue((root / "stage" / "review" / "auto_audit.md").exists())

    def test_real_approved_png_static_glb_and_animated_rig_are_reported(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            with self.sandbox(root):
                animal = studio.animal_profile("Jaguar")
                guardian = studio.guardian_profile(0)
                studio.stage(animal, studio.prompt_for(animal), studio.MODEL, PNG)
                studio.approve(animal)
                meshes.stage_model(guardian, glb_fixture(), "local-glb")
                meshes.approve_model(guardian)
                autorig.stage(guardian, animated_model(), "combined", "local-rig")
                autorig.approve(guardian, "combined")
                report = automatic.audit()
                self.assertEqual(report["approved_slots"], 2)
                self.assertEqual(report["approved_files"], 3)
                self.assertEqual(report["animated_rigs"], 1)
                self.assertEqual(len(report["errors"]), 0)
                self.assertTrue(any(x["format"] == "glb" for x in report["assets"]))
                self.assertTrue(any(x["format"] == "rigged" and x["bones"] == 2
                                    and x["clips"] == 5 for x in report["assets"]))
                self.assertTrue(any("4K texture not verified" in x["message"] for x in report["warnings"]))
                rendered = automatic.html_dashboard(report)
                self.assertIn('id="search"', rendered)
                self.assertIn('id="flag"', rendered)
                self.assertIn("guardian_00", rendered)
                self.assertIn("5 clips", rendered)

    def test_missing_manifest_and_checksum_tampering_fail_strict_audit(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            with self.sandbox(root):
                boss = studio.guardian_profile(3)
                target = studio.stage(boss, studio.prompt_for(boss), studio.MODEL, PNG)
                target.write_bytes(target.read_bytes()[:-13])
                report = automatic.audit()
                self.assertEqual(report["summary"], "FAIL")
                self.assertEqual(len(report["errors"]), 1)
                with redirect_stdout(io.StringIO()):
                    self.assertEqual(automatic.main(["--strict"]), 1)
                target.write_bytes(PNG)
                target.with_suffix(".json").unlink()
                report = automatic.audit()
                self.assertEqual(len(report["errors"]), 1)
                self.assertIn("manifest", report["errors"][0]["error"].lower())

    def test_unregistered_art_and_hero_files_are_rejected_not_approved(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            with self.sandbox(root):
                unexpected = root / "approved" / "guardians" / "hero_z_remixed.png"
                unexpected.parent.mkdir(parents=True)
                unexpected.write_bytes(PNG)
                result = automatic.audit()
                self.assertTrue(any("Unregistered" in x["error"] for x in result["errors"]))
                self.assertTrue(unexpected.exists())
                self.assertFalse((root / "stage" / "review").exists())
                with redirect_stdout(io.StringIO()):
                    self.assertEqual(automatic.main(["--strict"]), 1)

    def test_inbox_validation_does_not_stage_or_approve(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            with self.sandbox(root):
                incoming = root / "drop" / "animals"
                incoming.mkdir(parents=True)
                image = incoming / "animal_jaguar.png"
                image.write_bytes(PNG)
                report = automatic.audit(True, root / "drop")
                self.assertEqual(report["inbox_files"], 1)
                self.assertEqual(report["errors"], [])
                self.assertFalse((root / "stage").exists())
                image.write_bytes(b"bad")
                report = automatic.audit(True, root / "drop")
                self.assertEqual(len(report["errors"]), 1)
                self.assertEqual(report["summary"], "FAIL")

    def test_auto_audit_runs_after_one_click_intake(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            with self.sandbox(root):
                inbox = root / "inbox" / "animals"
                inbox.mkdir(parents=True)
                (inbox / "animal_jaguar.png").write_bytes(PNG)
                with redirect_stdout(io.StringIO()):
                    self.assertEqual(intake.main(["--inbox", str(root / "inbox"), "--stage"]), 0)
                local_report = root / "stage" / "review" / "auto_audit.json"
                self.assertTrue(local_report.exists())
                data = json.loads(local_report.read_text())
                self.assertEqual(data["staged_files"], 1)
                self.assertEqual(data["inbox_files"], 1)
                self.assertEqual(data["summary"], "PASS")
                self.assertFalse((root / "approved").exists())

    def test_auto_audit_runs_after_explicit_checksum_approval(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            with self.sandbox(root):
                item = studio.guardian_profile(0)
                studio.stage(item, studio.prompt_for(item), studio.MODEL, PNG)
                selection = root / "selection.json"
                selection.write_text(json.dumps({
                    "version": 1,
                    "approved_assets": [{"asset_id": item["asset_id"], "format": "png",
                                         "sha256": hashlib.sha256(PNG).hexdigest()}],
                }), encoding="utf-8")
                with redirect_stdout(io.StringIO()):
                    self.assertEqual(approval.main(["--manifest", str(selection), "--apply"]), 0)
                report = json.loads((root / "stage" / "review" / "auto_audit.json").read_text())
                self.assertEqual(report["approved_files"], 1)
                self.assertEqual(report["errors"], [])
                self.assertTrue((root / "approved" / "guardians" / "guardian_00.png").exists())

    def test_html_escapes_user_supplied_names_and_reports(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            with self.sandbox(root):
                bad = automatic.audit()
                bad["assets"].append({
                    "asset_id": "<script>alert(1)</script>", "category": "x", "format": "png",
                    "state": "staged", "triangles": None, "bones": None,
                    "clips": None, "verified_4k": None, "dimensions": None,
                    "errors": ["<img src=x onerror=alert(1)>"], "warnings": [],
                })
                html = automatic.html_dashboard(bad)
                self.assertNotIn("<script>alert(1)</script>", html)
                self.assertIn("&lt;script&gt;", html)
                self.assertNotIn("<img src=x onerror=alert(1)>", html)
                self.assertIn("&lt;img", html)


if __name__ == "__main__":
    unittest.main()

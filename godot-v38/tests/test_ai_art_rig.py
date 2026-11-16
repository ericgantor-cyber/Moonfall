"""Offline tests for real skin bindings, animations and optional Meshy rigging."""
import copy
import hashlib
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import ai_art_3d as models
import ai_art_autorig as autorig
import ai_art_rig as rig
import ai_art_review as review
import ai_art_approve_selected as approvals
import ai_art_release_audit as release
import ai_art_studio as studio
from test_ai_art_3d import GLTF, glb_fixture


def animated_model(names=("Idle", "Walk", "Attack", "Hurt", "Death")):
    doc = copy.deepcopy(GLTF)
    doc["nodes"] = [{"mesh": 0, "skin": 0}, {"name": "Hips"}, {"name": "Spine"}]
    doc["skins"] = [{"joints": [1, 2], "skeleton": 1, "inverseBindMatrices": 2}]
    doc["accessors"].extend([
        {"componentType": 5126, "count": 2, "type": "MAT4"},  # index 2: IBM
        {"componentType": 5126, "count": 2, "type": "SCALAR", "min": [0.0], "max": [2.0]},
        {"componentType": 5126, "count": 2, "type": "VEC4"},
    ])
    doc["animations"] = [
        {"name": name, "samplers": [{"input": 3, "output": 4}],
         "channels": [{"sampler": 0, "target": {"node": 1, "path": "rotation"}}]}
        for name in names
    ]
    return glb_fixture(doc)


class TestMoonfallRigging(unittest.TestCase):
    def test_all_five_native_actions_map_to_real_skeletal_glb_clips(self):
        report = rig.rig_report(animated_model())
        self.assertTrue(report["rigged"])
        self.assertEqual(report["bone_count"], 2)
        self.assertEqual(report["skinned_meshes"], 1)
        self.assertEqual(report["animation_count"], 5)
        self.assertEqual(set(report["mapped_clips"]), set(rig.PREFERRED_CLIPS))
        self.assertEqual(report["missing_preferred"], [])
        self.assertEqual(rig.canonical_clip("Armature|Walking_001"), "move")
        self.assertEqual(rig.canonical_clip("Hero/Attack_2"), "attack")
        self.assertEqual(rig.canonical_clip("Die"), "death")

    def test_invalid_skin_bindings_and_animation_channels_are_rejected(self):
        base = copy.deepcopy(GLTF)
        base["nodes"] = [{"mesh": 0, "skin": 0}, {"name": "Root"}]
        base["skins"] = [{"joints": [999]}]
        with self.assertRaises(ValueError):
            rig.rig_report(glb_fixture(base))
        base["skins"] = [{"joints": [1, 1]}]
        with self.assertRaises(ValueError):
            rig.rig_report(glb_fixture(base))
        base["skins"] = [{"joints": [1]}]
        base["animations"] = [{"name": "idle", "channels": [], "samplers": []}]
        with self.assertRaises(ValueError):
            rig.rig_report(glb_fixture(base))
        base["animations"] = [{
            "name": "idle", "channels": [
                {"sampler": 0, "target": {"node": 999, "path": "rotation"}}
            ], "samplers": [{"input": 0, "output": 1}]
        }]
        with self.assertRaises(ValueError):
            rig.rig_report(glb_fixture(base))

    def test_missing_clips_warn_and_static_fallback_remains_supported(self):
        static = rig.rig_report(glb_fixture())
        self.assertFalse(static["rigged"])
        self.assertEqual(static["animation_count"], 0)
        with self.assertRaises(ValueError):
            rig.rig_report(glb_fixture(), require_rig=True)
        partial = rig.rig_report(animated_model(("Walk",)), require_rig=True)
        self.assertTrue(partial["rigged"])
        self.assertEqual(partial["mapped_clips"], {"move": "Walk"})
        self.assertIn("attack", partial["missing_preferred"])

    def test_real_rigged_glb_approval_preserves_original_static_model(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            with patch.object(autorig, "CANDIDATES", root / "staging"), patch.object(
                autorig, "APPROVED", root / "approved_rigs"
            ), patch.object(models, "APPROVED_3D", root / "static"
            ), patch.object(studio, "ROOT", root / "png"):
                item = studio.guardian_profile(0)
                incoming = root / "my_rig.glb"
                incoming.write_bytes(animated_model())
                staged = autorig.import_local(item, incoming)
                self.assertTrue(staged.exists())
                self.assertEqual(autorig.paths(item)[1].name, "guardian_00.glb")
                self.assertFalse(autorig.paths(item)[1].exists())
                row = review.review_status(item)
                self.assertEqual(row["rigged_state"], "staged")
                self.assertTrue(row["rigged_valid"])
                self.assertEqual(row["rigged_rig"]["animation_count"], 5)
                selection = root / "approval.json"
                selection.write_text(json.dumps({"version": 1, "approved_assets": [{
                    "asset_id": "guardian_00", "format": "rigged",
                    "sha256": hashlib.sha256(animated_model()).hexdigest()
                }]}), encoding="utf-8")
                self.assertEqual(approvals.main(["--manifest", str(selection)]), 0)
                self.assertEqual(approvals.main(["--manifest", str(selection), "--apply"]), 0)
                self.assertTrue(autorig.paths(item)[1].exists())
                self.assertFalse(models.paths(item)[1].exists())
                report = release.audit()
                self.assertEqual(report["format_counts"], {"rigged": 1})
                self.assertEqual(report["assets"][0]["rig"]["bone_count"], 2)

    def test_auto_rig_requires_explicit_humanoid_confirm_and_no_preview_fees(self):
        item = studio.guardian_profile(0)
        with patch.object(autorig, "verified_static_source", return_value=(glb_fixture(), "1" * 64)), patch.object(
            models, "api_call", side_effect=AssertionError("Should not spend credits")
        ):
            with self.assertRaises(ValueError):
                autorig.submit(item, False, 1.8)
            with self.assertRaises(ValueError):
                autorig.submit(item, True, -2.0)
        with tempfile.TemporaryDirectory() as folder:
            with patch.object(autorig, "TASKS", Path(folder) / "tasks"), patch.object(
                autorig, "APPROVED", Path(folder) / "approved"), patch.object(
                autorig, "verified_static_source", return_value=(glb_fixture(), "1" * 64)
            ), patch.object(models, "api_call", return_value={"result": "12345678-9999"}) as api:
                self.assertEqual(autorig.submit(item, True, 1.8), "12345678-9999")
                self.assertEqual(api.call_count, 1)
                self.assertEqual(api.call_args.args[1], autorig.RIG_API)
                self.assertEqual(api.call_args.args[2]["height_meters"], 1.8)
                with self.assertRaises(FileExistsError):
                    autorig.submit(item, True, 1.8)

    def test_animation_api_requires_finished_rig_and_selected_real_action_ids(self):
        item = studio.guardian_profile(3)
        with patch.object(autorig, "status", return_value={"id": "rig-task-123", "status": "PENDING"}):
            with self.assertRaises(ValueError):
                autorig.submit_animation(item, [1])
        with patch.object(autorig, "status", return_value={"id": "rig-task-123", "status": "SUCCEEDED"}):
            for invalid in ([], [1] * 2, list(range(1, 12)), [-5]):
                with self.assertRaises(ValueError):
                    autorig.submit_animation(item, invalid)
            with tempfile.TemporaryDirectory() as folder:
                with patch.object(autorig, "ANIMATION_TASKS", Path(folder) / "tasks"), patch.object(
                    models, "api_call", return_value={"result": "animation-task-1234"}
                ) as api:
                    self.assertEqual(autorig.submit_animation(item, [17, 92]), "animation-task-1234")
                    self.assertEqual(api.call_args.args[1], autorig.ANIMATION_API)
                    self.assertEqual(api.call_args.args[2]["action_ids"], [17, 92])
                    self.assertEqual(api.call_args.args[2]["rig_task_id"], "rig-task-123")
                    with self.assertRaises(FileExistsError):
                        autorig.submit_animation(item, [17, 92])

    def test_completed_animation_output_is_staged_not_approved(self):
        item = studio.guardian_profile(4)
        with tempfile.TemporaryDirectory() as folder:
            with patch.object(autorig, "CANDIDATES", Path(folder) / "review"), patch.object(
                autorig, "APPROVED", Path(folder) / "approved"), patch.object(
                autorig, "animation_status", return_value={
                    "status": "SUCCEEDED", "result": {
                        "animation_glb_url": "https://assets.meshy.ai/test.glb"
                    }
                }), patch.object(models, "download_glb", return_value=animated_model()):
                target = autorig.fetch_animation(item)
                self.assertTrue(target.exists())
                self.assertEqual(target.name, "guardian_04.glb")
                self.assertFalse(autorig.paths(item)[1].exists())
                self.assertEqual(json.loads(target.with_suffix(".json").read_text())["origin"], "meshy-animation")

    def test_no_nonhumanoid_service_assumption_and_gameplay_visual_only(self):
        source = (Path(__file__).resolve().parents[1] / "scripts" /
                  "systems" / "ai_art_animation_runtime.gd").read_text(encoding="utf-8")
        native = (Path(__file__).resolve().parents[1] / "scripts" /
                  "native_v38_2.gd").read_text(encoding="utf-8")
        render = (Path(__file__).resolve().parents[1] / "scripts" /
                  "systems" / "ai_art_3d_runtime.gd").read_text(encoding="utf-8")
        self.assertIn("Skeleton3D", source)
        self.assertIn("AnimationPlayer", source)
        self.assertIn("state_for_actor", source)
        self.assertIn("attack_swing_until", source)
        self.assertIn("AI_ART_ANIMATION.apply_state", native)
        self.assertIn("ai_art_live_animal_models", native)
        self.assertIn("ai_art_anim_clock<0.08", native)
        self.assertIn("RIGGED_ROOT", render)
        self.assertIn("ResourceLoader.exists(rigged_path)", render)


if __name__ == "__main__":
    unittest.main()

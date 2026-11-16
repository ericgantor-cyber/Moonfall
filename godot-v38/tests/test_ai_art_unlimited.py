"""No arbitrary Moonfall AI-art creative caps, with bounded file safety."""
import copy
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import ai_art_3d as mesh3d
import ai_art_rig as rig
import ai_art_intake as intake
import ai_art_autorig as autorig
from test_ai_art_3d import GLTF, glb_fixture
from test_ai_art_rig import animated_model


def large_model(index_count=300_003):
    """Real, padded GLB binary large enough to contain a >100k triangle mesh."""
    doc = copy.deepcopy(GLTF)
    blob = b"\0" * (index_count * 2)
    blob += b"\0" * ((-len(blob)) % 4)
    doc["buffers"][0]["byteLength"] = len(blob)
    doc["accessors"][1]["count"] = index_count
    raw = json.dumps(doc).encode("utf-8")
    raw += b" " * ((-len(raw)) % 4)
    chunk1 = struct.pack("<II", len(raw), 0x4E4F534A) + raw
    chunk2 = struct.pack("<II", len(blob), 0x004E4942) + blob
    return struct.pack("<4sII", b"glTF", 2, 12 + len(chunk1) + len(chunk2)) + chunk1 + chunk2


class TestNoArtificialArtLimits(unittest.TestCase):
    def test_many_clips_and_long_animations_preserve_real_skin_integrity(self):
        many = animated_model(tuple("Idle" if i == 0 else f"OriginalArtAnimation_{i}"
                                    for i in range(40)))
        report = rig.rig_report(many, require_rig=True)
        self.assertEqual(report["animation_count"], 40)
        self.assertTrue(report["rigged"])
        self.assertEqual(len(report["clips"]), 40)
        doc = copy.deepcopy(GLTF)
        doc["nodes"] = [{"mesh": 0, "skin": 0}] + [{"name": f"Bone_{i}"} for i in range(180)]
        doc["skins"] = [{"joints": list(range(1, 181)), "skeleton": 1}]
        doc["animations"] = []
        bone_report = rig.rig_report(glb_fixture(doc))
        self.assertEqual(bone_report["bone_count"], 180)
        self.assertTrue(bone_report["rigged"])
        # A long cinematic clip is allowed when its time range is valid.
        doc2 = copy.deepcopy(GLTF)
        doc2["nodes"] = [{"mesh": 0, "skin": 0}, {"name": "Root"}]
        doc2["skins"] = [{"joints": [1], "skeleton": 1}]
        doc2["accessors"].extend([
            {"componentType": 5126, "count": 2, "type": "SCALAR",
             "min": [0.0], "max": [240.0]},
            {"componentType": 5126, "count": 2, "type": "VEC4"},
        ])
        doc2["animations"] = [{
            "name": "Idle", "samplers": [{"input": 2, "output": 3}],
            "channels": [{"sampler": 0, "target": {"node": 1, "path": "rotation"}}]
        }]
        long_report = rig.rig_report(glb_fixture(doc2), require_rig=True)
        self.assertEqual(long_report["clips"][0]["duration_seconds"], 240.0)

    def test_many_meshes_nodes_and_more_than_100k_triangles(self):
        complex_model = copy.deepcopy(GLTF)
        complex_model["meshes"] = [copy.deepcopy(GLTF["meshes"][0]) for _ in range(130)]
        complex_model["nodes"] = [{"mesh": 0}] + [{"name": f"Part_{i}"} for i in range(850)]
        result = mesh3d.glb_info(glb_fixture(complex_model))
        self.assertEqual(result["mesh_count"], 130)
        self.assertEqual(result["nodes"], 851)
        high_poly = mesh3d.glb_info(large_model())
        self.assertGreater(high_poly["triangles"], 100_000)
        # Fake indices cannot claim a giant mesh without data in the file.
        invalid = copy.deepcopy(GLTF)
        invalid["accessors"][1]["count"] = 300_003
        with self.assertRaises(ValueError):
            mesh3d.glb_info(glb_fixture(invalid))

    def test_offline_intake_accepts_animated_rig_from_separate_folder(self):
        with tempfile.TemporaryDirectory() as dirname:
            root = Path(dirname)
            inbox = root / "inbox"
            rig_folder = inbox / "rigged"
            rig_folder.mkdir(parents=True)
            (rig_folder / "guardian_00.glb").write_bytes(animated_model())
            with patch.object(autorig, "CANDIDATES", root / "review"), patch.object(
                autorig, "APPROVED", root / "approved"
            ):
                report = intake.plan(inbox)
                self.assertEqual(len(report), 1)
                self.assertEqual(report[0]["format"], "rigged")
                self.assertEqual(report[0]["status"], "ready")
                imported = intake.stage_all(inbox)
                self.assertEqual(imported[0]["status"], "staged_for_review")
                staged = root / "review" / "guardians" / "guardian_00.glb"
                self.assertTrue(staged.exists())
                self.assertFalse((root / "approved").exists())
                self.assertEqual(intake.plan(inbox)[0]["status"], "staged")

    def test_adaptive_quality_is_not_locked_to_three_animals_or_one_room(self):
        root = Path(__file__).resolve().parents[1]
        quality = (root / "scripts" / "systems" / "ai_art_auto_quality.gd").read_text()
        native = (root / "scripts" / "native_v38_2.gd").read_text()
        runtime = (root / "scripts" / "systems" / "ai_art_3d_runtime.gd").read_text()
        self.assertIn("return count", quality)
        self.assertIn("eligible_count", quality)
        self.assertIn("animal_limit(ai_art_quality_tier,candidates.size())", native)
        self.assertIn("chamber_limit(ai_art_quality_tier,rooms.size())", native)
        self.assertNotIn("AI_ART_NEAR_ANIMAL_LIMIT:int=3", native)
        self.assertNotIn("AI_ART_NEAR_CHAMBER_LIMIT:int=1", native)
        self.assertNotIn("MAX_VISIBLE_MESHES:int=160", runtime)
        animations = (root / "scripts" / "systems" /
                      "ai_art_animation_runtime.gd").read_text()
        review = (root / "tools" / "ai_art_review.py").read_text()
        self.assertNotIn("MAX_SEARCH_NODES:int=240", animations)
        self.assertNotIn("clip_names[:24]", review)
        self.assertNotIn("names[:24]", review)

    def test_batch_intake_and_approval_no_artificial_selection_count(self):
        source = (Path(__file__).resolve().parents[1] / "tools" /
                  "ai_art_approve_selected.py").read_text()
        inbox = (Path(__file__).resolve().parents[1] / "tools" /
                 "ai_art_intake.py").read_text()
        self.assertNotIn("MAX_INBOX_FILES = 900", inbox)
        self.assertNotIn("MAX_INBOX_BYTES = 512", inbox)
        self.assertNotIn("MAX_SELECTION_ENTRIES = 700", source)
        self.assertIn("len(available)", source)
        self.assertIn("autorig.import_local(item, path)", inbox)


if __name__ == "__main__":
    unittest.main()

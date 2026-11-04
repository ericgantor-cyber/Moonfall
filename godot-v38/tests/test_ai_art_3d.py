"""Real 3D GLB and 4K PBR pipeline offline regressions. No actual API calls."""
import copy
import io
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from contextlib import redirect_stdout
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import ai_art_3d as mesh3d
import ai_art_studio as studio
from test_ai_art_studio import PNG

GLTF = {
    "asset": {"version": "2.0"},
    "scene": 0,
    "scenes": [{"nodes": [0]}],
    "nodes": [{"mesh": 0}],
    "meshes": [{"primitives": [{"attributes": {"POSITION": 0}, "indices": 1}]}],
    "accessors": [
        {"componentType": 5126, "count": 3, "type": "VEC3"},
        {"componentType": 5123, "count": 3, "type": "SCALAR"},
    ],
    "buffers": [{"byteLength": 44}],
}


def glb_fixture(document=None):
    raw = json.dumps(GLTF if document is None else document).encode("utf-8")
    raw += b" " * ((-len(raw)) % 4)
    json_chunk = struct.pack("<II", len(raw), 0x4E4F534A) + raw
    binary = b"\0" * 44
    bin_chunk = struct.pack("<II", len(binary), 0x004E4942) + binary
    return struct.pack("<4sII", b"glTF", 2, 12 + len(json_chunk) + len(bin_chunk)) + json_chunk + bin_chunk


class TestMoonfall3DArt(unittest.TestCase):
    def test_request_is_real_3d_with_4096_textures_and_game_topology(self):
        request = mesh3d.mesh_request(PNG)
        self.assertEqual(request["ai_model"], "meshy-7.1")
        self.assertEqual(request["texture_model"], "meshy-7")
        self.assertEqual(request["texture_resolution"], "4k")
        self.assertEqual(request["geometry_resolution"], "standard")
        self.assertEqual(request["target_polycount"], 12000)
        self.assertEqual(request["target_formats"], ["glb"])
        self.assertTrue(request["enable_pbr"])
        self.assertTrue(request["image_url"].startswith("data:image/png;base64,"))

    def test_approved_slots_reject_ui_and_original_hero_art(self):
        for id in [0, 54]:
            item = studio.guardian_profile(id)
            self.assertIn(f"guardian_{id:02}", str(mesh3d.paths(item)[1]))
        self.assertEqual(mesh3d.choose_item(self._args(game_art="gear:crest"))["asset_id"], "gear_crest")
        self.assertEqual(mesh3d.choose_item(self._args(game_art="chambers:14"))["asset_id"], "chamber_14")
        for slot in ("glyphs:orb_hud", "spells:fire", "regions:00", "heroes:z", "scenery:../../heroes"):
            with self.assertRaises(ValueError):
                mesh3d.choose_item(self._args(game_art=slot))

    @staticmethod
    def _args(game_art=None, guardian=None, animal=None):
        from argparse import Namespace
        return Namespace(guardian=guardian, animal=animal, game_art=game_art)

    def test_glb_requires_real_3d_scene_and_bounded_triangles(self):
        good = glb_fixture()
        info = mesh3d.glb_info(good)
        self.assertEqual(info["triangles"], 1)
        self.assertEqual(info["mesh_count"], 1)
        for bad in (b"glTF" + b"\0" * 40, good[:-6], good + b"anything"):
            with self.assertRaises(ValueError):
                mesh3d.glb_info(bad)
        huge = copy.deepcopy(GLTF)
        huge["accessors"][1]["count"] = 400_000
        with self.assertRaises(ValueError):
            mesh3d.glb_info(glb_fixture(huge))
        external = copy.deepcopy(GLTF)
        external["buffers"][0]["uri"] = "https://example.com/sneaky.bin"
        with self.assertRaises(ValueError):
            mesh3d.glb_info(glb_fixture(external))
        no_mesh = copy.deepcopy(GLTF)
        no_mesh["meshes"] = []
        with self.assertRaises(ValueError):
            mesh3d.glb_info(glb_fixture(no_mesh))

    def test_offline_model_staging_and_automatic_godot_import_path(self):
        with tempfile.TemporaryDirectory() as directory:
            folder = Path(directory)
            with patch.object(mesh3d, "STAGING_3D", folder / "review"), patch.object(
                mesh3d, "APPROVED_3D", folder / "game"
            ), patch.object(mesh3d, "TASKS_3D", folder / "tasks"), patch.object(
                mesh3d, "api_call", side_effect=AssertionError("Paid call")
            ):
                item = studio.guardian_profile(4)
                incoming = folder / "source.glb"
                incoming.write_bytes(glb_fixture())
                staged = mesh3d.import_local(item, incoming)
                self.assertTrue(staged.is_file())
                self.assertFalse((folder / "game").exists())
                approved = mesh3d.approve_model(item)
                self.assertTrue(approved.exists())
                self.assertEqual(approved, folder / "game" / "guardians" / "guardian_04.glb")
                self.assertTrue(json.loads(approved.with_suffix(".json").read_text())["reviewed"])
                with self.assertRaises(FileExistsError):
                    mesh3d.approve_model(item)
                with redirect_stdout(io.StringIO()):
                    self.assertEqual(mesh3d.main(["--guardian", "4"]), 0)

    def test_corrupt_model_cannot_be_approved(self):
        with tempfile.TemporaryDirectory() as directory:
            folder = Path(directory)
            with patch.object(mesh3d, "STAGING_3D", folder / "review"), patch.object(
                mesh3d, "APPROVED_3D", folder / "game"
            ):
                item = studio.guardian_profile(1)
                staged = mesh3d.stage_model(item, glb_fixture(), "local-glb")
                staged.write_bytes(staged.read_bytes()[:-1])
                with self.assertRaises(ValueError):
                    mesh3d.approve_model(item)

    def test_paid_3d_requests_require_explicit_submit_and_reviewed_png(self):
        with tempfile.TemporaryDirectory() as directory:
            folder = Path(directory)
            with patch.object(mesh3d, "STAGING_3D", folder / "models"), patch.object(
                mesh3d, "APPROVED_3D", folder / "approved"
            ), patch.object(mesh3d, "TASKS_3D", folder / "tasks"), patch.object(
                studio, "ROOT", folder / "png"
            ):
                item = studio.guardian_profile(0)
                with patch.object(mesh3d, "api_call", side_effect=AssertionError("Network")):
                    with redirect_stdout(io.StringIO()):
                        self.assertEqual(mesh3d.main(["--guardian", "0"]), 0)
                    with self.assertRaises(FileNotFoundError):
                        mesh3d.submit(item)
                source = folder / "png" / "guardians" / "guardian_00.png"
                source.parent.mkdir(parents=True)
                source.write_bytes(PNG)
                import hashlib
                source.with_suffix(".json").write_text(json.dumps({
                    "asset_id": "guardian_00", "sha256": hashlib.sha256(PNG).hexdigest(), "reviewed": True
                }))
                with patch.object(mesh3d, "api_call", return_value={"result": "valid-task-001"}):
                    task_id = mesh3d.submit(item)
                self.assertEqual(task_id, "valid-task-001")
                with self.assertRaises(FileExistsError):
                    mesh3d.submit(item)

    def test_no_remote_3d_download_from_unapproved_host(self):
        for url in ("http://assets.meshy.ai/model.glb",
                    "https://assets.meshy.ai.evil.example/foo.glb",
                    "https://malicious.example/model.glb"):
            with self.assertRaises(ValueError):
                mesh3d.download_glb(url)

    def test_game_runtime_uses_optional_packed_scene_without_touching_combat(self):
        script = (Path(__file__).resolve().parents[1] / "scripts" / "systems" / "ai_art_3d_runtime.gd").read_text()
        native = (Path(__file__).resolve().parents[1] / "scripts" / "native_v38_2.gd").read_text()
        self.assertIn("PackedScene", script)
        self.assertIn("MAX_VISIBLE_MESHES", script)
        self.assertIn("add_approved_model", script)
        self.assertIn("CollisionShape3D", script)
        self.assertIn("body", native)
        self.assertIn('add_approved_model(body,"animals"', native)
        self.assertIn('add_approved_model(guardian_art_body,"guardians"', native)
        self.assertIn('stage in [7,14]', native)
        self.assertIn('add_approved_model(node,"gear"', native)


if __name__ == "__main__":
    unittest.main()

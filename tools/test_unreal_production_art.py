"""Validate committed source meshes without installing Unreal."""
from pathlib import Path
import json
import math
import unittest

ROOT = Path(__file__).resolve().parents[1] / "MoonfallUnreal"
ART = ROOT / "Art" / "Production"
class ProductionArtTests(unittest.TestCase):
    def test_meshes_have_valid_uvs_faces_and_material_slots(self):
        manifest = json.loads((ART / "manifest.json").read_text())
        self.assertEqual(manifest["destination"], "/Game/MoonfallProduction/Meshes")
        self.assertEqual(len(manifest["assets"]), 3)
        materials = {l.split()[1] for l in (ART / "Meshes" / "moonfall_relics.mtl").read_text().splitlines() if l.startswith("newmtl ")}
        for asset in manifest["assets"]:
            name = asset["name"]
            self.assertEqual(asset["file"], name + ".obj")
            self.assertTrue(name.startswith("SM_MF_"))
            vertices, uv, faces = [], [], []
            active = None
            for line in (ART / "Meshes" / asset["file"]).read_text().splitlines():
                parts = line.split()
                if not parts: continue
                if parts[0] == "v": vertices.append(tuple(map(float, parts[1:])))
                elif parts[0] == "vt": uv.append(tuple(map(float, parts[1:])))
                elif parts[0] == "usemtl":
                    active = parts[1]
                    self.assertIn(active, materials)
                elif parts[0] == "f":
                    self.assertIn(active, materials)
                    self.assertEqual(len(parts), 4)
                    faces.append([tuple(map(int, x.split("/"))) for x in parts[1:]])
            self.assertEqual(len(faces), asset["triangles"])
            self.assertGreater(len(vertices), 100)
            self.assertTrue(all(len(v)==3 and all(math.isfinite(x) for x in v) for v in vertices))
            self.assertTrue(all(len(t)==2 and all(0<=x<=1 for x in t) for t in uv))
            for face in faces:
                self.assertEqual(len({v for v,_ in face}),3)
                for v,t in face:
                    self.assertTrue(1<=v<=len(vertices))
                    self.assertTrue(1<=t<=len(uv))
    def test_importer_is_allowlisted_non_destructive(self):
        source = (ROOT / "Content/Python/import_production_art.py").read_text()
        for token in ('"replace_existing", False','"auto_generate_collision", False','/Game/MoonfallProduction/Meshes','does_asset_exist(target)','isinstance(mesh, unreal.StaticMesh)'):
            self.assertIn(token,source)
        self.assertNotIn("delete_asset(",source)

if __name__ == "__main__":
    unittest.main()

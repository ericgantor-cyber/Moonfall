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
        self.assertEqual(len(manifest["assets"]), 6)
        self.assertEqual({a["name"] for a in manifest["assets"]}, {
            "SM_MF_FirstlightSword", "SM_MF_LunarReliquary", "SM_MF_GothicColumn",
            "SM_MF_FirstlightShield", "SM_MF_FirstlightCuirass", "SM_MF_EclipseAmulet"})
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
    def test_world_mesh_bindings_preserve_fallbacks_and_collision(self):
        src = ROOT / "Source" / "MoonfallUnreal"
        shrine = (src / "MoonfallCheckpointShrine.cpp").read_text()
        arena = (src / "MoonfallArena.cpp").read_text()
        for token in ("SM_MF_LunarReliquary", "Bounds.IsValid", "FMath::IsFinite(Span)",
                      "ShrineBase->SetStaticMesh(Imported)", "ShrineBeacon->SetVisibility(false)",
                      "DiscoveryRange->InitSphereRadius(220.f)"):
            self.assertIn(token, shrine)
        self.assertLess(shrine.index("ShrineBase->SetStaticMesh(Imported)"), shrine.index("ShrineBeacon->SetVisibility(false)"))
        for token in ("SM_MF_GothicColumn", "ColumnBounds.IsValid", "Size.GetMin()>0.1f",
                      "ImportedColumns->SetCollisionEnabled(ECollisionEnabled::NoCollision)",
                      "ImportedColumns->SetCanEverAffectNavigation(false)",
                      "Pieces==MoonfallSanctuaryArchitecture::GothicStone",
                      "Piece.ScaleZ>=4.9f", "ImportedColumns->AddInstance(",
                      "Details->AddInstance(FTransform(", "DungeonRoad->SetStaticMesh(Cube.Object)"):
            self.assertIn(token, arena)
        self.assertLess(arena.index("ImportedColumns->AddInstance("), arena.index("ImportedColumns->RegisterComponent()"))
        self.assertIn("!=INDEX_NONE)\n     continue;", arena)
        
    def test_shield_heraldry_is_display_only(self):
        arena = (ROOT / "Source" / "MoonfallUnreal" / "MoonfallArena.cpp").read_text()
        art = arena.split("// Imported heraldry is display-only",1)[1].split("// Four soft moon-blue beacons",1)[0]
        for token in ("SanctuaryShieldHeraldry", "IsValid(ProductionShield)",
                      "Bounds.IsValid", "Shields->SetStaticMesh(ProductionShield)",
                      "Shields->SetCollisionEnabled(ECollisionEnabled::NoCollision)",
                      "Shields->SetCanEverAffectNavigation(false)",
                      "Shields->SetCastShadow(false)", "Shields->SetGenerateOverlapEvents(false)",
                      "{-760.f,760.f}", "{2670.f,3170.f,3870.f,4370.f}"):
            self.assertIn(token, art)
        self.assertLess(art.index("Shields->AddInstance("), art.index("Shields->RegisterComponent()"))
        self.assertNotIn("SpawnActor", art)

    def test_pbr_profiles_and_non_destructive_material_installer(self):
        catalog = json.loads((ART / "materials.json").read_text())
        slots = {"bronze", "leather", "steel", "stone", "moonstone"}
        self.assertEqual(set(catalog["materials"]), slots)
        for key, profile in catalog["materials"].items():
            self.assertEqual(len(profile["base_color"]), 3)
            self.assertTrue(all(math.isfinite(v) and 0 <= v <= 1 for v in
                                profile["base_color"] + [profile["metallic"], profile["roughness"]]))
            self.assertEqual(profile["metallic"], float(key in {"bronze", "steel"}))
        script = (ROOT / "Content/Python/install_production_materials.py").read_text()
        for token in ("MP_BASE_COLOR", "MP_METALLIC", "MP_ROUGHNESS",
                      "found != expected", "current is None or current.get_path_name() == DEFAULT",
                      "mesh.set_material(index, materials[slot])", "does_asset_exist(target)"):
            self.assertIn(token, script)
        self.assertLess(script.index("found != expected"), script.index("mesh.set_material(index"))
        self.assertNotIn("delete_asset(", script)

    def test_importer_is_allowlisted_non_destructive(self):
        source = (ROOT / "Content/Python/import_production_art.py").read_text()
        for token in ('"replace_existing", False','"auto_generate_collision", False','/Game/MoonfallProduction/Meshes','does_asset_exist(target)','isinstance(mesh, unreal.StaticMesh)'):
            self.assertIn(token,source)
        self.assertNotIn("delete_asset(",source)

if __name__ == "__main__":
    unittest.main()

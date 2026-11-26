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

    def test_textured_material_graph_uses_correct_channels_and_fallbacks(self):
        script = (ROOT / "Content/Python/install_production_materials.py").read_text()
        for token in ('("BaseColor", "RGB", unreal.MaterialProperty.MP_BASE_COLOR)',
                      '("Normal", "RGB", unreal.MaterialProperty.MP_NORMAL)',
                      '("ORM", "R", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)',
                      '("ORM", "G", unreal.MaterialProperty.MP_ROUGHNESS)',
                      '("ORM", "B", unreal.MaterialProperty.MP_METALLIC)',
                      "SAMPLERTYPE_COLOR", "SAMPLERTYPE_NORMAL", "SAMPLERTYPE_MASKS",
                      '("_Textured" if textures else "")',
                      "len(textures) != len(SLOTS)*len(kinds)",
                      "elif textures and current.get_path_name() == previous:"):
            self.assertIn(token, script)
        self.assertLess(script.index("textures = load_texture_pack()"), script.index("mesh.set_material(index"))
        self.assertNotIn("delete_asset(", script)

    def test_texture_generation_is_repeatable_and_periodic(self):
        import importlib.util
        import struct
        import tempfile
        import zlib
        spec = importlib.util.spec_from_file_location("mf_texture", ART / "generate_texture_maps.py")
        gen = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(gen)
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory)
            self.assertEqual(gen.generate(64, output), 15)
            files = sorted(output.glob("*.png"))
            self.assertEqual(len(files), 15)
            for path in files:
                data = path.read_bytes()
                self.assertEqual(data[:8], b"\x89PNG\r\n\x1a\n")
                self.assertEqual(struct.unpack(">II", data[16:24]), (64, 64))
                pos, compressed = 8, bytearray()
                while pos < len(data):
                    count = struct.unpack(">I", data[pos:pos+4])[0]
                    kind = data[pos+4:pos+8]
                    payload = data[pos+8:pos+8+count]
                    crc = struct.unpack(">I", data[pos+8+count:pos+12+count])[0]
                    self.assertEqual(crc, zlib.crc32(kind+payload)&0xffffffff)
                    if kind == b"IDAT": compressed.extend(payload)
                    pos += 12+count
                raw = zlib.decompress(compressed)
                self.assertEqual(len(raw), 64*(64*3+1))
                if path.stem.endswith("_ORM"):
                    pixels = [raw[y*193+1:y*193+193] for y in range(64)]
                    metallic = 255 if "Steel" in path.stem or "Bronze" in path.stem else 0
                    self.assertTrue(all(row[i] == metallic for row in pixels for i in range(2,192,3)))
            original = {p.name:p.read_bytes() for p in files}
            gen.generate(64, output)
            self.assertEqual(original, {p.name:p.read_bytes() for p in files})
            files[0].write_bytes(b"protected")
            with self.assertRaises(FileExistsError): gen.generate(64, output)
            self.assertEqual(files[0].read_bytes(), b"protected")
        for name in gen.NAMES:
            self.assertAlmostEqual(gen.height_at(name,.123,.456,101), gen.height_at(name,1.123,.456,101))
            self.assertAlmostEqual(gen.height_at(name,.123,.456,101), gen.height_at(name,.123,1.456,101))
        script = (ROOT / "Content/Python/import_production_textures.py").read_text()
        for token in ('"replace_existing", False', '"srgb", kind == "BaseColor"',
                      "TC_NORMALMAP", "TC_MASKS", "isinstance(texture, unreal.Texture2D)"):
            self.assertIn(token, script)

    def test_optional_niagara_spell_effect_budget_and_fallback(self):
        src = ROOT / "Source" / "MoonfallUnreal"
        pawn = (src / "MoonfallHeroPawn.cpp").read_text()
        header = (src / "MoonfallHeroPawn.h").read_text()
        config = json.loads((ROOT / "MoonfallUnreal.uproject").read_text())
        self.assertIn({"Name":"Niagara","Enabled":True}, config["Plugins"])
        self.assertIn('PrivateDependencyModuleNames.Add("Niagara")', (src / "MoonfallUnreal.Build.cs").read_text())
        self.assertIn("SpellImpactSystems", header)
        fx = pawn.split("bool AMoonfallHeroPawn::TryPlayAuthoredSpellImpact",1)[1].split("void AMoonfallHeroPawn::CastLeft()",1)[0]
        for token in ("bReduceMotion", "NM_DedicatedServer", "Point.ContainsNaN()",
                      "School==3", "SpellImpactSystems.IsValidIndex(School)",
                      "ActiveSpellImpactFX.Num()>=4", "SpawnSystemAtLocation(",
                      "ENCPoolMethod::None", "WeakFX", "DestroyComponent()", "2.f,false"):
            self.assertIn(token, fx)
        self.assertLess(fx.index("ActiveSpellImpactFX.Num()>=4"), fx.index("SpawnSystemAtLocation("))
        self.assertNotIn("TakeSpellDamage", fx)
        self.assertNotIn("Mana -=", fx)
        self.assertIn("if (!TryPlayAuthoredSpellImpact(School, Hit.ImpactPoint, Multiplier))", pawn)
        self.assertIn("DrawDebugSphere(GetWorld(), Hit.ImpactPoint", pawn)

    def test_cinematic_scalability_and_isolated_lighting(self):
        import configparser
        ini = configparser.ConfigParser()
        ini.read(ROOT / "Config/DefaultScalability.ini")
        for level, bloom in enumerate((1,2,4,5)):
            group = ini["PostProcessQuality@" + str(level)]
            for key in ("r.MotionBlurQuality", "r.DepthOfFieldQuality",
                        "r.SceneColorFringeQuality", "r.LensFlareQuality"):
                self.assertEqual(group.getint(key), 0)
            self.assertEqual(group.getint("r.BloomQuality"), bloom)
        script = (ROOT / "Tools/install_cinematic_preview_lighting.py").read_text()
        for token in ("/Game/NorthernExpanse/Maps/NorthernExpansePreview",
                      "DynamicGlobalIlluminationMethod.LUMEN", "ReflectionMethod.LUMEN",
                      '"motion_blur_amount": 0.0', '"unbound", True',
                      "ScopedEditorTransaction", "existing actors preserved"):
            self.assertIn(token, script)
        self.assertLess(script.index('world.get_path_name()'), script.index("spawn_actor_from_class("))
        self.assertNotIn("save_current_level(", script)

    def test_nanite_benchmarks_leave_runtime_meshes_untouched(self):
        script = (ROOT / "Tools/create_nanite_benchmark_assets.py").read_text()
        for token in ('DEST = "/Game/MoonfallProduction/NanitePreview"',
                      'NAMES = ("SM_MF_GothicColumn", "SM_MF_LunarReliquary")',
                      "duplicate_asset(source, target)", 'set_editor_property("enabled", True)',
                      'set_editor_property("keep_percent_triangles", 1.0)',
                      'set_editor_property("fallback_percent_triangles", 1.0)',
                      "NaniteFallbackTarget.PERCENT_TRIANGLES",
                      "editor.set_nanite_settings(mesh, settings, True)"):
            self.assertIn(token, script)
        self.assertLess(script.index("does_asset_exist(target)"), script.index("duplicate_asset(source, target)"))
        self.assertLess(script.index("duplicate_asset(source, target)"), script.index("editor.set_nanite_settings("))
        self.assertNotIn("delete_asset(", script)

    def test_warden_skin_and_animation_source(self):
        import base64
        import struct
        asset = json.loads((ART / "Rigs/SK_MF_FirstlightWarden.gltf").read_text())
        self.assertEqual(asset["asset"]["version"], "2.0")
        buffer = asset["buffers"][0]
        raw = base64.b64decode(buffer["uri"].split(",",1)[1], validate=True)
        self.assertEqual(len(raw), buffer["byteLength"])
        dimensions = {"SCALAR":1, "VEC2":2, "VEC3":3, "VEC4":4, "MAT4":16}
        formats = {5126:"f", 5123:"H"}
        def read(index):
            a = asset["accessors"][index]
            view = asset["bufferViews"][a["bufferView"]]
            count = a["count"] * dimensions[a["type"]]
            fmt = "<" + str(count) + formats[a["componentType"]]
            self.assertEqual(view["byteOffset"] % 4, 0)
            self.assertEqual(struct.calcsize(fmt), view["byteLength"])
            self.assertLessEqual(view["byteOffset"] + view["byteLength"], len(raw))
            values = struct.unpack_from(fmt, raw, view["byteOffset"])
            self.assertTrue(all(math.isfinite(v) for v in values))
            return values
        for index in range(len(asset["accessors"])):
            read(index)
        skin = asset["skins"][0]
        self.assertEqual(len(skin["joints"]), 17)
        self.assertEqual(len(set(skin["joints"])), 17)
        self.assertEqual(len(read(skin["inverseBindMatrices"])), 17*16)
        total_triangles = 0
        for primitive in asset["meshes"][0]["primitives"]:
            attrs = primitive["attributes"]
            pos = read(attrs["POSITION"])
            joints = read(attrs["JOINTS_0"])
            weights = read(attrs["WEIGHTS_0"])
            count = len(pos)//3
            self.assertEqual(len(joints), count*4)
            self.assertEqual(len(weights), count*4)
            self.assertTrue(all(0 <= j < 17 for j in joints))
            for i in range(0,len(weights),4):
                self.assertAlmostEqual(sum(weights[i:i+4]), 1.0)
                self.assertTrue(all(0 <= w <= 1 for w in weights[i:i+4]))
            indices = read(primitive["indices"])
            self.assertEqual(len(indices)%3, 0)
            self.assertTrue(all(0 <= i < count for i in indices))
            for i in range(0,len(indices),3):
                self.assertEqual(len(set(indices[i:i+3])), 3)
            total_triangles += len(indices)//3
        self.assertEqual(total_triangles, 1296)
        self.assertEqual({a["name"] for a in asset["animations"]},
                         {"Idle","Walk","Attack","Guard","HitReact"})
        for clip in asset["animations"]:
            for channel in clip["channels"]:
                self.assertIn(channel["target"]["node"], skin["joints"])
                sampler = clip["samplers"][channel["sampler"]]
                times, rotations = read(sampler["input"]), read(sampler["output"])
                self.assertEqual(times[0], 0)
                self.assertTrue(all(b > a for a,b in zip(times,times[1:])))
                self.assertEqual(len(rotations), len(times)*4)
                for i in range(0,len(rotations),4):
                    self.assertAlmostEqual(sum(q*q for q in rotations[i:i+4]), 1.0, places=5)

    def test_pc_audit_is_local_read_only_and_does_not_claim_gpu_support(self):
        script = (ROOT / "Build/Get-MoonfallPCAudit.ps1").read_text()
        for token in ("Win32_Processor", "Win32_OperatingSystem", "Win32_VideoController",
                      "TotalVisibleMemorySize", "NumberOfLogicalProcessors", "DriverVersion",
                      "DirectX12FeatureSupport = 'Not tested'", "DedicatedVRAM = 'Not determined'",
                      "MeasuredFPS = 'Not tested'", "ConvertTo-Json"):
            self.assertIn(token, script)
        for token in ("Invoke-WebRequest", "Invoke-RestMethod", "Set-CimInstance", "Set-ItemProperty",
                      "Out-File", "Set-Content", "Start-Process", "RunAs"):
            self.assertNotIn(token, script)
        hud = (ROOT / "Source/MoonfallUnreal/MoonfallCombatHUD.cpp").read_text()
        audit = hud.split("// Static hardware inventory only",1)[1].split("DrawText(HelpText",1)[0]
        for token in ("MenuWidth>=480.f", "MenuHeight>=460.f", "static const uint64 RAMGiB",
                      "FPlatformMemory::GetConstants().TotalPhysical", "NumberOfCoresIncludingHyperthreads()",
                      "PC AUDIT"):
            self.assertIn(token, audit)
        self.assertNotIn("ApplySettings", audit)

    def test_pc_audit_reports_active_renderer_without_guessing_capabilities(self):
        source = ROOT / "Source/MoonfallUnreal"
        hud = (source / "MoonfallCombatHUD.cpp").read_text()
        self.assertIn('PrivateDependencyModuleNames.Add("RHI")', (source / "MoonfallUnreal.Build.cs").read_text())
        block = hud.split("// Report the active RHI",1)[1].split("DrawText(HelpText",1)[0]
        for token in ("GRHIAdapterName.IsEmpty()", "GRHIAdapterName.Left(40)",
                      "GDynamicRHI?FString(GDynamicRHI->GetName())", 'TEXT("Unknown GPU")',
                      "MenuHeight>=540.f", "MenuTop+42.f"):
            self.assertIn(token, block)
        for token in ("ApplySettings", "SetOverallScalabilityLevel", "ExecuteConsoleCommand"):
            self.assertNotIn(token, block)

    def test_importer_is_allowlisted_non_destructive(self):
        source = (ROOT / "Content/Python/import_production_art.py").read_text()
        for token in ('"replace_existing", False','"auto_generate_collision", False','/Game/MoonfallProduction/Meshes','does_asset_exist(target)','isinstance(mesh, unreal.StaticMesh)'):
            self.assertIn(token,source)
        self.assertNotIn("delete_asset(",source)

if __name__ == "__main__":
    unittest.main()

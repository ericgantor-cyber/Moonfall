"""Prevent accidental publication of blank or uncooked Moonfall UE5.6 Windows builds.

Static checks only: no Unreal, Windows or runtime validation is performed.
"""
from pathlib import Path
import ast
import unittest

ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "engine-comparison/unreal/MoonfallPrototype/Build/Package-Windows.ps1"
WORKFLOW = ROOT / ".github/workflows/ue56-prototype-package-windows.yml"

class PackageContract(unittest.TestCase):
    def test_workflow_is_manual_and_restricted(self):
        s = WORKFLOW.read_text(encoding="utf-8")
        for expected in ("workflow_dispatch:", "runs-on: [self-hosted, Windows, X64, unreal-5-6]",
                         "persist-credentials: false", "permissions:", "contents: read"):
            self.assertIn(expected, s)
        self.assertNotIn("pull_request:", s)
        self.assertIn("if-no-files-found: error", s)
        self.assertIn("UNTESTED", s)

    def test_packaging_requires_actual_startup_map(self):
        s = SCRIPT.read_text(encoding="utf-8")
        for expected in ("Content\\Maps\\$MapName.umap", "GameDefaultMap=",
                         "DefaultEngine.ini", "UE5.6 is required", "BuildCookRun",
                         "'-cook'", "'-stage'", "'-pak'", "'-archive'",
                         "MoonfallPrototype.exe", "LASTEXITCODE"):
            self.assertIn(expected, s)

    def test_generator_validates_existing_level_without_overwriting(self):
        script = (ROOT / "engine-comparison/unreal/MoonfallPrototype/Content/Python/generate_startup_world.py").read_text(encoding="utf-8")
        ast.parse(script)
        packaging = SCRIPT.read_text(encoding="utf-8")
        for expected in ("assets.does_asset_exist(LEVEL_PATH)", "levels.load_level(LEVEL_PATH)",
                         "levels.new_level(LEVEL_PATH, False)", "subsystem.get_all_level_actors()",
                         "verify_actor(world_actors, unreal.PlayerStart)",
                         "if not levels.save_current_level():"):
            self.assertIn(expected, script)
        self.assertIn("if existing:", script)
        self.assertIn('MOONFALL_WORLD_VALIDATION_SENTINEL', script)
        self.assertIn('sentinel_path.write_text(', script)
        self.assertIn('$ValidationSentinel', packaging)
        self.assertIn('if (-not (Test-Path -LiteralPath $ValidationSentinel', packaging)
        self.assertIn("MoonfallWorld validated", packaging)
        self.assertIn("Refusing packaging", packaging)
        self.assertIn("Get-FileHash -LiteralPath $MapAsset -Algorithm SHA256", packaging)
        self.assertIn("$UpdatedMapHash -ne $ExistingMapHash", packaging)
        self.assertIn("Refusing packaging", packaging)

    def test_package_requires_fresh_archive_and_hashes_real_files(self):
        script = SCRIPT.read_text(encoding="utf-8")
        workflow = WORKFLOW.read_text(encoding="utf-8")
        for item in (
            "if ($OldEntries.Count -gt 0)",
            "avoid stale build artifacts",
            "Where-Object { $_.Length -gt 0 }",
            "$HasPak -or ($HasToc -and $HasCas)",
            "MoonfallPackageManifest.json",
            "Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256",
            "tested_in_game = $false",
            "installer = $false",
            "startup_map = $MapLongName",
            "ConvertTo-Json -Depth 6",
        ):
            self.assertIn(item, script)
        self.assertIn("Verify build manifest exists before artifact upload", workflow)
        self.assertLess(workflow.index("Verify build manifest exists before artifact upload"),
                        workflow.index("Upload packaged game only after cook succeeds"))
        self.assertIn("if (@($data.files).Count -eq 0)", workflow)

    def test_project_configures_generated_startup_map(self):
        ini = (ROOT / "engine-comparison/unreal/MoonfallPrototype/Config/DefaultEngine.ini").read_text(encoding="utf-8")
        self.assertIn("GameDefaultMap=/Game/Maps/MoonfallWorld", ini)
        script = SCRIPT.read_text(encoding="utf-8")
        self.assertIn("generate_startup_world.py", script)
        self.assertIn("UnrealEditor-Cmd.exe", script)
        self.assertIn("-ExecutePythonScript=$Generator", script)
        self.assertIn("Unreal editor did not produce a startup level", script)

if __name__ == "__main__":
    unittest.main()

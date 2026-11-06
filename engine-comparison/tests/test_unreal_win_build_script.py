"""Static checks for the optional Unreal Win64 build verification entrypoint.

These tests do not substitute for running UnrealBuildTool on Windows.
"""
from pathlib import Path
import unittest

SCRIPT = Path(__file__).resolve().parents[1] / "unreal" / "verify-windows-build.ps1"
PROJECT = Path(__file__).resolve().parents[1] / "unreal" / "MoonfallPrototype"


class UnrealWindowsBuildScriptTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.script = SCRIPT.read_text(encoding="utf-8")

    def test_project_and_targets_exist(self):
        self.assertTrue((PROJECT / "MoonfallPrototype.uproject").is_file())
        self.assertTrue((PROJECT / "Source" / "MoonfallPrototype.Target.cs").is_file())
        self.assertTrue((PROJECT / "Source" / "MoonfallPrototypeEditor.Target.cs").is_file())
        self.assertIn('Invoke-UnrealBuild "MoonfallPrototypeEditor" "Development"', self.script)
        self.assertIn('Invoke-UnrealBuild "MoonfallPrototype" "Development"', self.script)

    def test_external_engine_path_is_mandatory(self):
        self.assertIn("[Parameter(Mandatory = $true)]", self.script)
        self.assertIn("[string]$EngineRoot", self.script)
        self.assertIn("Test-Path -LiteralPath $BuildBat -PathType Leaf", self.script)
        self.assertIn("Unreal Build.bat not found", self.script)

    def test_exit_codes_and_output_are_checked(self):
        self.assertIn("if ($LASTEXITCODE -ne 0)", self.script)
        self.assertIn("throw \"UnrealBuildTool failed", self.script)
        self.assertIn("MoonfallPrototype.exe", self.script)
        self.assertIn("Build command returned success but", self.script)

    def test_packaging_requires_level_and_reports_unverified_gameplay(self):
        self.assertIn("[switch]$Package", self.script)
        self.assertIn('-Filter "*.umap"', self.script)
        self.assertIn("if ($Maps.Count -eq 0)", self.script)
        self.assertIn("Create and configure a startup map", self.script)
        self.assertIn("Config\\DefaultEngine.ini", self.script)
        self.assertIn("GameDefaultMap", self.script)
        self.assertIn("if (-not $ConfiguredMap.Success)", self.script)
        self.assertIn("$ConfiguredMap.Groups[1].Value", self.script)
        self.assertIn("if (-not (Test-Path -LiteralPath $StartupMap -PathType Leaf))", self.script)
        self.assertLess(self.script.index("if (-not (Test-Path -LiteralPath $StartupMap -PathType Leaf))"),
                        self.script.index("    & $RunUAT BuildCookRun "))
        self.assertIn("BuildCookRun", self.script)
        self.assertIn("Manually launch and test", self.script)


if __name__ == "__main__":
    unittest.main()

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

    def test_build_requires_exact_unreal_56_engine(self):
        self.assertIn('Engine\\Build\\Build.version', self.script)
        self.assertIn("Get-Content -LiteralPath $VersionFile -Raw | ConvertFrom-Json", self.script)
        self.assertIn("$EngineVersion.MajorVersion -ne 5 -or $EngineVersion.MinorVersion -ne 6", self.script)
        self.assertIn("Moonfall prototype requires Unreal 5.6", self.script)
        self.assertLess(self.script.index("if ($EngineVersion.MajorVersion"),
                        self.script.index('Invoke-UnrealBuild "MoonfallPrototypeEditor"'))

    def test_exit_codes_and_output_are_checked(self):
        self.assertIn("if ($LASTEXITCODE -ne 0)", self.script)
        self.assertIn("throw \"UnrealBuildTool failed", self.script)
        self.assertIn("MoonfallPrototype.exe", self.script)
        self.assertIn("Build command returned success but", self.script)
        self.assertIn("function Test-WindowsExecutable {", self.script)
        self.assertIn("$reader.ReadUInt16() -ne 0x5A4D", self.script)
        self.assertIn("$stream.Position = 0x3C", self.script)
        self.assertIn("$peOffset -lt 64 -or $peOffset -gt $stream.Length - 26", self.script)
        self.assertIn("$reader.ReadUInt32() -ne 0x00004550", self.script)
        self.assertIn("$reader.ReadUInt16() -ne 0x8664", self.script)
        self.assertIn("$reader.ReadUInt16() -eq 0x20B", self.script)
        self.assertIn("if (-not (Test-WindowsExecutable -Path $GameExe))", self.script)
        self.assertIn("not a Windows PE file", self.script)

    def test_game_executable_must_match_latest_build_inputs(self):
        self.assertIn('Get-ChildItem -LiteralPath $SourceRoot -Recurse -File', self.script)
        self.assertIn('$_.Extension -in @(".cpp", ".h", ".cs")', self.script)
        self.assertIn('Get-Item -LiteralPath $Project', self.script)
        self.assertIn('$BuildInputs = @($SourceFiles) + @(Get-Item -LiteralPath $Project)', self.script)
        self.assertIn('Sort-Object LastWriteTimeUtc -Descending', self.script)
        self.assertIn('$LatestInputUtc.AddSeconds(-2)', self.script)
        self.assertIn('Win64 game executable is older than project sources or build rules', self.script)
        self.assertLess(self.script.index('if (-not (Test-WindowsExecutable -Path $GameExe))'),
                        self.script.index('if ((Get-Item -LiteralPath $GameExe).LastWriteTimeUtc'))

    def test_editor_map_creator_uses_unreal_api_without_overwriting_assets(self):
        import json
        project = json.loads((PROJECT / "MoonfallPrototype.uproject").read_text(encoding="utf-8"))
        self.assertIn({"Name": "PythonScriptPlugin", "Enabled": True}, project["Plugins"])
        self.assertIn({"Name": "EditorScriptingUtilities", "Enabled": True}, project["Plugins"])
        ini = (PROJECT / "Config" / "DefaultEngine.ini").read_text(encoding="utf-8")
        self.assertIn("GameDefaultMap=/Game/Maps/MoonfallPrototype", ini)
        self.assertIn("EditorStartupMap=/Engine/Maps/Entry", ini)
        self.assertNotIn("EditorStartupMap=/Game/Maps/MoonfallPrototype", ini)
        creator = (PROJECT / "Content" / "Python" / "create_startup_map.py").read_text(encoding="utf-8")
        self.assertIn("import unreal", creator)
        self.assertIn('LEVEL_PATH = "/Game/Maps/MoonfallPrototype"', creator)
        self.assertIn("if assets.does_asset_exist(LEVEL_PATH):", creator)
        self.assertIn("unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)", creator)
        self.assertIn("levels.new_level(LEVEL_PATH, is_partitioned_world=False)", creator)
        self.assertIn("if not assets.does_asset_exist(LEVEL_PATH)", creator)

    def test_create_map_opt_in_propagates_errors_and_checks_map(self):
        self.assertIn("[switch]$CreateMap", self.script)
        self.assertIn("UnrealEditor-Cmd.exe", self.script)
        self.assertIn("create_startup_map.py", self.script)
        self.assertIn("-ExecutePythonScript=$MapScript", self.script)
        self.assertIn("if ($LASTEXITCODE -ne 0)", self.script)
        self.assertIn("Unreal Editor exited without creating the required .umap", self.script)
        self.assertLess(self.script.index("if ($CreateMap) {"), self.script.index("if ($Package) {"))

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
        self.assertEqual(self.script.count("& $RunUAT BuildCookRun"), 1)
        self.assertEqual(self.script.count("$ConfiguredMap = [regex]::Match"), 1)
        self.assertIn("BuildCookRun", self.script)
        self.assertIn('$RunArchiveDirectory = Join-Path $ArchiveDirectory ("run-" + [guid]::NewGuid().ToString("N"))', self.script)
        self.assertIn("if (Test-Path -LiteralPath $RunArchiveDirectory)", self.script)
        self.assertIn('"-archivedirectory=$RunArchiveDirectory"', self.script)
        self.assertIn("Test-Path -LiteralPath $RunArchiveDirectory -PathType Container", self.script)
        self.assertIn('Get-ChildItem -LiteralPath $RunArchiveDirectory -File -Recurse -Filter "MoonfallPrototype.exe"', self.script)
        self.assertIn("Test-WindowsExecutable -Path $_.FullName", self.script)
        self.assertNotIn("$PackageStartedUtc", self.script)
        self.assertIn("if ($PackagedExecutables.Count -eq 0)", self.script)
        self.assertLess(self.script.index("    & $RunUAT BuildCookRun "),
                        self.script.index("if ($PackagedExecutables.Count -eq 0)"))
        self.assertIn("Manually launch and test", self.script)


if __name__ == "__main__":
    unittest.main()

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
                         "'-cook'", "'-stage'", "'-package'", "'-pak'", "'-archive'",
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

    def test_local_end_to_end_verification_command(self):
        verify = (ROOT / "engine-comparison/unreal/MoonfallPrototype/Build/Verify-Windows.ps1").read_text(encoding="utf-8")
        for required in ("Check-WindowsRunner.ps1", "Package-Windows.ps1",
                         "SmokeTest-Windows.ps1", "MoonfallPrototypeEditor",
                         "'MoonfallPrototype'", "'Win64'", "'Development'",
                         "-OutputDirectory $OutputDirectory", "-BootSeconds $BootSeconds",
                         "Refusing to reuse a nonempty archive directory",
                         "Unreal $Target C++ compilation failed"):
            self.assertIn(required, verify)
        self.assertLess(verify.index("& $Preflight"), verify.index("& $Build"))
        self.assertLess(verify.index("& $Build"), verify.index("& $Packager"))
        self.assertLess(verify.index("& $Packager"), verify.index("& $Smoke"))

    def test_trusted_windows_runner_preflight_before_engine_actions(self):
        preflight = (ROOT / "engine-comparison/unreal/MoonfallPrototype/Build/Check-WindowsRunner.ps1").read_text(encoding="utf-8")
        compile_ci = (ROOT / ".github/workflows/ue56-prototype-compile-windows.yml").read_text(encoding="utf-8")
        package_ci = WORKFLOW.read_text(encoding="utf-8")
        for required in ("Build.version", "UE56_ROOT", "UnrealEditor-Cmd.exe", "RunUAT.bat",
                         "vswhere.exe", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
                         "'[17.0,18.0)'", "Windows Kits", "um\\x64", "EngineAssociation"):
            self.assertIn(required, preflight)
        for workflow, build_step in (
            (compile_ci, "Compile MoonfallPrototypeEditor Win64 Development"),
            (package_ci, "Cook and package real UE5.6 Win64 game"),
        ):
            self.assertIn("Check-WindowsRunner.ps1 -UnrealRoot $env:UE56_ROOT", workflow)
            self.assertLess(workflow.index("Check-WindowsRunner.ps1"), workflow.index(build_step))
            self.assertIn("runs-on: [self-hosted, Windows, X64, unreal-5-6]", workflow)

    def test_run_isolated_windows_archive(self):
        workflow = WORKFLOW.read_text(encoding="utf-8")
        self.assertIn("MOONFALL_PACKAGE_DIR: ${{ runner.temp }}/MoonfallUE56Package/${{ github.run_id }}-${{ github.run_attempt }}", workflow)
        self.assertIn("-OutputDirectory $env:MOONFALL_PACKAGE_DIR", workflow)
        self.assertEqual(workflow.count("$root = $env:MOONFALL_PACKAGE_DIR"), 2)
        self.assertIn("path: ${{ env.MOONFALL_PACKAGE_DIR }}/", workflow)
        self.assertIn("${{ runner.temp }}/MoonfallSmokeDiagnostics/", workflow)
        self.assertNotIn("${{ env.MOONFALL_PACKAGE_DIR }}/SmokeDiagnostics/", workflow)
        self.assertNotIn("Saved/WindowsPackage/", workflow)
        script = SCRIPT.read_text(encoding="utf-8")
        self.assertIn("if ($OldEntries.Count -gt 0)", script)

    def test_smoke_logs_never_mutate_manifest_locked_game_package(self):
        smoke = (ROOT / "engine-comparison/unreal/MoonfallPrototype/Build/SmokeTest-Windows.ps1").read_text(encoding="utf-8")
        workflow = WORKFLOW.read_text(encoding="utf-8")
        self.assertIn("[string]$DiagnosticsDirectory = ''", smoke)
        self.assertIn("Join-Path ([System.IO.Path]::GetTempPath())", smoke)
        self.assertIn("$SmokeDir.StartsWith($PackagePrefix", smoke)
        self.assertIn("Smoke diagnostics must be outside the cooked package directory", smoke)
        self.assertNotIn("$SmokeDir = Join-Path $PackageDirectory 'SmokeDiagnostics'", smoke)
        self.assertIn("-DiagnosticsDirectory (Join-Path $env:RUNNER_TEMP 'MoonfallSmokeDiagnostics')", workflow)
        self.assertIn("${{ runner.temp }}/MoonfallSmokeDiagnostics/", workflow)

    def test_installer_requires_real_hashed_game_and_desktop_shortcut(self):
        script = (ROOT / "engine-comparison/unreal/MoonfallPrototype/Build/Create-Installer-Windows.ps1").read_text(encoding="utf-8")
        workflow = (ROOT / ".github/workflows/ue56-prototype-installer-windows.yml").read_text(encoding="utf-8")
        for required in (
            "MoonfallPackageManifest.json", "Get-FileHash -LiteralPath $Target -Algorithm SHA256",
            "MoonfallPrototype.exe", "CookedContainers", "Unlisted file",
            "Inno Setup 6", "ISCC.exe", "MoonfallPrototype-UE56-Win64-UNTESTED-Setup",
            "desktopicon", "{autodesktop}", "InstallerBuildStatus.json",
            "installation_tested = $false", "gameplay_tested = $false", "signed = $false",
        ):
            self.assertIn(required.lower(), script.lower())
        for required in ("workflow_dispatch:", "persist-credentials: false",
                         "runs-on: [self-hosted, Windows, X64, unreal-5-6]",
                         "Package-Windows.ps1", "SmokeTest-Windows.ps1",
                         "Create-Installer-Windows.ps1", "if-no-files-found: error", "UNTESTED"):
            self.assertIn(required, workflow)
        self.assertNotIn("pull_request:", workflow)
        self.assertLess(workflow.index("Compile cook and package"),
                        workflow.index("Headless boot smoke"))
        self.assertLess(workflow.index("Headless boot smoke"),
                        workflow.index("Create unsigned desktop-shortcut installer"))
        self.assertLess(workflow.index("Create unsigned desktop-shortcut installer"),
                        workflow.index("Upload installer only after successful build"))

    def test_boot_smoke_rejects_unlisted_and_duplicate_archive_files(self):
        smoke = (ROOT / "engine-comparison/unreal/MoonfallPrototype/Build/SmokeTest-Windows.ps1").read_text(encoding="utf-8")
        for guard in (
            "HashSet[string]",
            "Package manifest has an empty file inventory",
            "Empty or duplicate file path in package manifest",
            "Package file count mismatch",
            "Unlisted file present in Windows package",
            "Get-FileHash -LiteralPath $Target -Algorithm SHA256",
        ):
            self.assertIn(guard, smoke)
        self.assertLess(smoke.index("Package file count mismatch"), smoke.index("Start-Process -FilePath"))

    def test_headless_boot_smoke_gates_windows_artifact_upload(self):
        workflow = WORKFLOW.read_text(encoding="utf-8")
        smoke = (ROOT / "engine-comparison/unreal/MoonfallPrototype/Build/SmokeTest-Windows.ps1").read_text(encoding="utf-8")
        self.assertIn("Verify packaged game boots into MoonfallWorld headlessly", workflow)
        self.assertLess(workflow.index("Verify packaged game boots into MoonfallWorld headlessly"),
                        workflow.index("Upload packaged game only after cook succeeds"))
        self.assertIn("SmokeDiagnostics/", workflow)
        for guard in (
            "MoonfallPackageManifest.json",
            "Get-FileHash -LiteralPath $Target -Algorithm SHA256",
            "Packaged file SHA256 mismatch",
            "Start-Process -FilePath $Exe.FullName",
            "Start-Sleep -Seconds $BootSeconds",
            "if ($Process.HasExited)",
            "MoonfallWorld",
            "Fatal error:",
            "Unhandled Exception:",
            "Stop-Process -Id $Process.Id -Force",
            "-nullrhi",
            "-unattended",
            "not a display, input, quest, save-load, audio or performance test",
        ):
            self.assertIn(guard.lower(), smoke.lower())
        self.assertIn("BootSeconds must be between 10 and 120 seconds", smoke)
        self.assertIn("$QuotedLogPath = '\"' + $LogPath + '\"'", smoke)
        self.assertIn('"-abslog=$QuotedLogPath"', smoke)
        self.assertIn("(?m)^.*MOONFALL_BOOT_READY", smoke)
        pawn = (ROOT / "engine-comparison/unreal/MoonfallPrototype/Source/MoonfallPrototype/MoonfallPawn.cpp").read_text(encoding="utf-8")
        ready = "MOONFALL_BOOT_READY pawn=MoonfallPawn regions=%d chambers=%d"
        self.assertIn(ready, pawn)
        self.assertIn("MoonfallWorldRules::RegionCount, MoonfallWorldRules::TotalChambers", pawn)
        self.assertIn("MOONFALL_BOOT_READY pawn=MoonfallPawn regions=6 chambers=30", smoke)
        self.assertIn("gameplay pawn did not confirm", smoke)

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

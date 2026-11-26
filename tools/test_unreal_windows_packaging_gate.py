"""Source regression tests for the shared UE5 packaged-content gate."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / "MoonfallUnreal/Build"
WORKFLOW = ROOT / ".github/workflows/unreal-windows-build.yml"
HOSTED = ROOT / ".github/workflows/unreal-hosted-windows.yml"


class PackagedContentGateTests(unittest.TestCase):
    def test_windows_ci_rejects_empty_cooked_content_before_publishing(self):
        workflow = WORKFLOW.read_text(encoding="utf-8")
        packaging = workflow.split("- name: Package Windows game using AutomationTool", 1)[1].split(
            "- name: Smoke-test packaged Unreal game launch", 1
        )[0]
        self.assertIn("Assert-CookedContent.ps1 -GameRoot $exe.Directory.FullName", packaging)
        self.assertLess(packaging.index("BuildCookRun"), packaging.index("Assert-CookedContent.ps1"))
        self.assertLess(packaging.index("Assert-CookedContent.ps1"), packaging.index('Write-Host "Packaged executable found'))

    def test_all_build_paths_require_shared_cooked_archive_validator(self):
        for path in (BUILD / "Build-Windows.ps1", WORKFLOW, HOSTED):
            self.assertIn("Assert-CookedContent.ps1", path.read_text(encoding="utf-8"))
        validator = (BUILD / "Assert-CookedContent.ps1").read_text(encoding="utf-8")
        for token in ("MoonfallUnreal/Content/Paks", "*.pak", "*.utoc", ".ucas",
                      "Length -gt 0", "$pakFiles.Count -eq 0 -and -not $hasIoStorePair"):
            self.assertIn(token, validator)
        fixtures = (BUILD / "Test-FindPackagedGame.Tests.ps1").read_text(encoding="utf-8")
        self.assertIn("Assert-CookedContent.ps1", fixtures)
        self.assertIn("mismatched-pair", fixtures)
        self.assertIn("misplaced-pak", fixtures)


if __name__ == "__main__":
    unittest.main()

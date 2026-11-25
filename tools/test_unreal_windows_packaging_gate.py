"""Source regression tests for fail-closed UE5 packaged-content verification."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
WORKFLOW = ROOT / ".github/workflows/unreal-windows-build.yml"
LOCAL_BUILD = ROOT / "MoonfallUnreal/Build/Build-Windows.ps1"


class PackagedContentGateTests(unittest.TestCase):
    def test_windows_ci_rejects_empty_cooked_content_before_publishing(self):
        workflow = WORKFLOW.read_text(encoding="utf-8")
        packaging = workflow.split("- name: Package Windows game using AutomationTool", 1)[1].split(
            "- name: Smoke-test packaged Unreal game launch", 1
        )[0]
        for required in (
            "MoonfallUnreal\\Content\\Paks", '"*.pak"', '"*.utoc"', '".ucas"',
            "$pakFiles.Count -eq 0 -and -not $pairedIoStore",
            "throw \"Cook produced no non-empty .pak or matching .utoc/.ucas",
        ):
            self.assertIn(required, packaging)
        self.assertLess(packaging.index("BuildCookRun"), packaging.index("$pakDir"))
        self.assertLess(packaging.index("$pakDir"), packaging.index('Write-Host "Packaged executable found'))

    def test_local_and_ci_builds_require_cooked_archives(self):
        build = LOCAL_BUILD.read_text(encoding="utf-8")
        workflow = WORKFLOW.read_text(encoding="utf-8")
        for source in (build, workflow):
            self.assertIn("*.pak", source)
            self.assertIn("*.utoc", source)
            self.assertIn(".ucas", source)
            self.assertIn("Length -gt 0", source)


if __name__ == "__main__":
    unittest.main()

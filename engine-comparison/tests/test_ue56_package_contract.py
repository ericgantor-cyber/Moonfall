"""Prevent accidental publication of blank or uncooked Moonfall UE5.6 Windows builds.

Static checks only: no Unreal, Windows or runtime validation is performed.
"""
from pathlib import Path
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

    def test_project_currently_missing_startup_map_configuration(self):
        ini = (ROOT / "engine-comparison/unreal/MoonfallPrototype/Config/DefaultEngine.ini").read_text(encoding="utf-8")
        # Do not mislabel source-only successful checks as a playable package.
        self.assertNotIn("GameDefaultMap=/Game/Maps/MoonfallWorld", ini)

if __name__ == "__main__":
    unittest.main()

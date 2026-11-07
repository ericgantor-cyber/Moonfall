"""Static guardrails for the opt-in UE5.6 Windows editor and game target build workflow.

These tests are NOT Unreal compilation or Windows runtime verification.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
WORKFLOW = ROOT / ".github/workflows/ue56-prototype-compile-windows.yml"
PROJECT = ROOT / "engine-comparison/unreal/MoonfallPrototype/MoonfallPrototype.uproject"

class UE56CompileGate(unittest.TestCase):
    def test_manual_trusted_runner_only(self):
        text = WORKFLOW.read_text(encoding="utf-8")
        self.assertIn("workflow_dispatch:", text)
        self.assertNotIn("pull_request:", text)
        self.assertIn("runs-on: [self-hosted, Windows, X64, unreal-5-6]", text)
        self.assertIn("persist-credentials: false", text)
        self.assertIn("permissions:", text)
        self.assertIn("contents: read", text)

    def test_actual_ue56_prototype_editor_target(self):
        text = WORKFLOW.read_text(encoding="utf-8")
        project = PROJECT.read_text(encoding="utf-8")
        self.assertIn('"EngineAssociation": "5.6"', project)
        self.assertIn("UE56_ROOT", text)
        self.assertIn("MoonfallPrototype.uproject", text)
        self.assertIn("MoonfallPrototypeEditor", text)
        self.assertIn("Compile MoonfallPrototype Win64 Development game target", text)
        self.assertIn("& $build 'MoonfallPrototype' 'Win64' 'Development'", text)
        self.assertIn("'-WaitMutex'", text)
        self.assertIn("Build.bat", text)
        self.assertIn("MajorVersion", text)
        self.assertIn("MinorVersion", text)

    def test_no_false_release_claim(self):
        text = WORKFLOW.read_text(encoding="utf-8")
        self.assertIn("No Windows game export", text)
        self.assertIn("not a cooked or packaged Windows release", text)
        self.assertIn("if: failure()", text)

if __name__ == "__main__":
    unittest.main()

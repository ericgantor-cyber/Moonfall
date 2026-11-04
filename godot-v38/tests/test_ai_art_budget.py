"""Offline regression checks for Moonfall AI art budget planner."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import ai_art_budget as budget


def asset(**overrides):
    item = {"asset_id": "guardian_00", "category": "guardians", "format": "rigged",
            "state": "approved", "valid": True, "errors": [], "bytes": 1000,
            "estimated_uncompressed_mipmap_bytes": 0, "triangles": 1000,
            "bones": 10, "clips": 2}
    item.update(overrides)
    return item


class ArtBudgetTests(unittest.TestCase):
    def test_priorities_and_unreviewed_excluded(self):
        report = {"schema": "moonfall.ai_art.auto_audit.v1", "assets": [
            asset(asset_id="fine"),
            asset(asset_id="huge", triangles=110000,
                  estimated_uncompressed_mipmap_bytes=70 * budget.MIB),
            asset(asset_id="broken", valid=False, errors=["bad hash"]),
            asset(asset_id="unreviewed", state="staged", triangles=500000)]}
        plan = budget.make_plan(report)
        self.assertEqual(plan["approved_assets"], 3)
        self.assertEqual([x["asset_id"] for x in plan["assets"]], ["broken", "huge", "fine"])
        self.assertEqual((plan["needs_repair"], plan["needs_profiling"]), (1, 1))
        self.assertIn("LOD", plan["assets"][1]["action"])

    def test_bad_schema_and_threshold(self):
        with self.assertRaises(ValueError):
            budget.make_plan({"assets": []})
        with self.assertRaises(ValueError):
            budget.make_plan({"schema": "moonfall.ai_art.auto_audit.v1", "assets": []}, 0)

    def test_csv_json_markdown_cli(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            input_path = root / "audit.json"
            input_path.write_text(json.dumps({
                "schema": "moonfall.ai_art.auto_audit.v1", "assets": [asset()]}))
            cmd = [sys.executable, str(Path(budget.__file__)), "--audit", str(input_path),
                   "--output-dir", str(root / "out")]
            run = subprocess.run(cmd, capture_output=True, text=True, check=False)
            self.assertEqual(run.returncode, 0, run.stderr)
            for filename in ("art_budget.csv", "art_budget.json", "art_budget.md"):
                self.assertTrue((root / "out" / filename).exists())
            self.assertEqual(json.loads((root / "out" / "art_budget.json").read_text())["approved_assets"], 1)


if __name__ == "__main__":
    unittest.main()

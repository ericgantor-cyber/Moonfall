"""Review-board tests run entirely offline, without any generated game art."""
from contextlib import redirect_stdout
import io
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import ai_art_studio as studio
import ai_art_review as review
from test_ai_art_studio import PNG


class TestMoonfallArtReview(unittest.TestCase):
    def test_full_plan_covers_55_guardians_and_234_animals(self):
        rows = review.plan_rows()
        self.assertEqual(len([r for r in rows if r["category"] == "guardians"]), 55)
        self.assertEqual(len([r for r in rows if r["category"] == "animals"]), 234)
        self.assertEqual(len(rows), 289)
        self.assertEqual(len({r["id"] for r in rows}), 289)
        self.assertTrue(all(r["state"] == "missing" or r["valid"] for r in rows))

    def test_filter_and_offline_gallery(self):
        with tempfile.TemporaryDirectory() as folder:
            with patch.object(studio, "STAGING", Path(folder) / "staging"), patch.object(
                studio, "ROOT", Path(folder) / "approved"
            ), patch.object(studio, "generate", side_effect=AssertionError("API request")):
                item = studio.guardian_profile(0)
                studio.stage(item, studio.prompt_for(item), studio.MODEL, PNG)
                self.assertEqual(review.main(["--category", "guardians", "--gallery"]), 0)
                directory = studio.STAGING / "review"
                content = (directory / "index.html").read_text(encoding="utf-8")
                self.assertIn("Moonfang Sentinel", content)
                self.assertIn("Staged artwork", content)
                self.assertIn("55 planned creature artworks", content)
                self.assertIn("Generation prompt", content)
                manifest = json.loads((directory / "art_plan.json").read_text(encoding="utf-8"))
                self.assertEqual(len(manifest), 55)
                self.assertEqual(manifest[0]["state"], "staged")
                self.assertEqual(manifest[0]["valid"], True)

    def test_invalid_staged_asset_is_flagged_by_audit(self):
        with tempfile.TemporaryDirectory() as folder:
            with patch.object(studio, "STAGING", Path(folder) / "staging"), patch.object(
                studio, "ROOT", Path(folder) / "approved"
            ):
                item = studio.guardian_profile(1)
                image = studio.stage(item, "test", studio.MODEL, PNG)
                image.write_bytes(PNG + b"x")
                with redirect_stdout(io.StringIO()):
                    self.assertEqual(review.main(["--category", "guardians", "--audit"]), 1)
                image.write_bytes(PNG)
                with redirect_stdout(io.StringIO()):
                    self.assertEqual(review.main(["--category", "guardians", "--audit"]), 0)


if __name__ == "__main__":
    unittest.main()

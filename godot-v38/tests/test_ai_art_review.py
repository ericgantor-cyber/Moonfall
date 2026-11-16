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
from ai_art_game_catalog import all_game_items, count_game_items, parse_slot, SLOTS
from test_ai_art_studio import PNG


class TestMoonfallArtReview(unittest.TestCase):
    def test_full_plan_covers_55_guardians_and_234_animals(self):
        rows = review.plan_rows()
        self.assertEqual(len([r for r in rows if r["category"] == "guardians"]), 55)
        self.assertEqual(len([r for r in rows if r["category"] == "animals"]), 234)
        self.assertEqual(len(rows), 289 + count_game_items())
        self.assertEqual(count_game_items(), 61)
        self.assertEqual(len([r for r in rows if r["category"] == "chambers"]), 14)
        self.assertEqual(len([r for r in rows if r["category"] == "regions"]), 6)
        self.assertEqual(len([r for r in rows if r["category"] == "glyphs"]), 9)
        self.assertEqual(len({r["id"] for r in rows}), 350)
        self.assertTrue(all(r["state"] == "missing" or r["valid"] for r in rows))

    def test_game_art_slots_are_safe_and_complete(self):
        ids = [item["asset_id"] for item in all_game_items()]
        self.assertEqual(len(ids), 61)
        self.assertEqual(len(set(ids)), 61)
        self.assertEqual(parse_slot("glyphs:orb_hud")["asset_id"], "glyph_orb_hud")
        self.assertEqual(parse_slot("regions:00")["asset_id"], "region_00")
        self.assertEqual(parse_slot("chambers:14")["asset_id"], "chamber_14")
        for bad in ["heroes:z", "regions:06", "chambers:15", "glyphs:missing", "glyphs", "gear:../../heroes"]:
            with self.assertRaises(ValueError):
                parse_slot(bad)
        for item in all_game_items():
            prompt = studio.prompt_for(item)
            self.assertIn("Moonfall", prompt)
            self.assertIn(item["subject"], prompt)
            self.assertIn("transparent", prompt.lower())
            self.assertIn("Z or Nyra", prompt)
        self.assertIn("red HP, blue mana, purple XP", studio.prompt_for(parse_slot("glyphs:orb_hud")))

    def test_game_art_gallery_filter_and_preview_never_generate(self):
        with tempfile.TemporaryDirectory() as folder:
            with patch.object(studio, "STAGING", Path(folder) / "staging"), patch.object(
                studio, "ROOT", Path(folder) / "approved"
            ), patch.object(studio, "generate", side_effect=AssertionError("Paid API called")):
                self.assertEqual(studio.main(["--game-art", "glyphs:orb_hud"]), 0)
                self.assertEqual(review.main(["--category", "glyphs", "--gallery"]), 0)
                manifest = json.loads((studio.STAGING / "review" / "art_plan.json").read_text())
                self.assertEqual(len(manifest), 9)
                self.assertEqual(len(review.plan_rows("all")), 350)
                html = (studio.STAGING / "review" / "index.html").read_text()
                self.assertIn("HUD & glyphs", html)

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
                self.assertIn('id="art-search"', content)
                self.assertIn('id="art-status"', content)
                self.assertIn('id="art-category"', content)
                self.assertIn('aria-live="polite"', content)
                self.assertIn('data-status="staged"', content)
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

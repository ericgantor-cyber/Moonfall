#!/usr/bin/env python3
"""Source contract for native classless talent UI and additive checkpoint data."""
import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
SCREEN = ROOT / "godot-v38/scripts/native_v38_0.gd"
MODEL = ROOT / "godot-v38/scripts/systems/moonfall_talent_tree.gd"


class ClasslessTalentUIScreen(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.screen = SCREEN.read_text(encoding="utf-8")
        cls.model = MODEL.read_text(encoding="utf-8")

    def test_existing_keyboard_panel_uses_classless_tree(self):
        self.assertIn("event.keycode==KEY_K", self.screen)
        self.assertIn("CLASSLESS TALENTS", self.screen)
        self.assertIn("CLASSLESS_TALENTS.evaluate(", self.screen)
        self.assertIn("CLASSLESS_TALENTS.spend(", self.screen)
        self.assertIn("button.disabled=not bool(preview.get(\"ok\",false))", self.screen)

    def test_save_restores_classless_without_clobbering_legacy(self):
        self.assertIn('"classless_ranks":v406_ranks.duplicate(true)', self.screen)
        self.assertIn('resources.get("classless_ranks",{})', self.screen)
        self.assertIn('resources.get("celestial_ranks",{})', self.screen)
        self.assertIn('v406_ranks=restored["allocations"].duplicate(true)', self.screen)
        self.assertIn('if bool(restored.get("ok",false)):', self.screen)
        self.assertIn('v406_ranks={}', self.screen)
        self.assertIn('_save_game()', self.screen)

    def test_full_tree_respec_not_individual_refund(self):
        self.assertIn("CLASSLESS_TALENTS.respec(", self.screen)
        self.assertIn("RESET ALL TALENT POINTS", self.screen)
        self.assertNotIn("CLASSLESS_TALENTS.refund(", self.screen)

    def test_original_art_assets_still_preloaded(self):
        self.assertIn('Z_Original.webp', (ROOT / "godot-v38/scripts/main.gd").read_text(encoding="utf-8"))
        self.assertIn('Nyra_Original.png', (ROOT / "godot-v38/scripts/main.gd").read_text(encoding="utf-8"))


if __name__ == "__main__":
    unittest.main()

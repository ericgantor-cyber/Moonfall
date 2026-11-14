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

    def test_keyboard_panel_uses_interactive_classless_web(self):
        self.assertIn("event.keycode==KEY_K", self.screen)
        self.assertIn("CLASSLESS TALENTS", self.screen)
        self.assertIn("CLASSLESS_TALENTS.evaluate(", self.screen)
        self.assertIn("CLASSLESS_TALENTS.spend(", self.screen)
        self.assertIn("v406_purchase_button.disabled=not bool(preview.get(", self.screen)
        self.assertIn('V406_WEB.new()', self.screen)
        self.assertIn('v406_web.talent_selected.connect(_v406_web_selected)', self.screen)

    def test_web_has_pan_zoom_keyboard_and_large_controls(self):
        web = (ROOT / "godot-v38/scripts/systems/moonfall_talent_web.gd").read_text(encoding="utf-8")
        for token in ("_draw()", "draw_line(", "draw_circle(",
                      "MOUSE_BUTTON_WHEEL_UP", "MOUSE_BUTTON_WHEEL_DOWN",
                      "drag_distance", "KEY_LEFT", "KEY_HOME", "KEY_EQUAL",
                      "talent_selected.emit(", "func focus_branch(", "func fit_all("):
            self.assertIn(token, web)
        self.assertIn('v406_web.custom_minimum_size=Vector2(760,565)', self.screen)
        self.assertIn('focus.custom_minimum_size=Vector2(148,52)', self.screen)
        self.assertIn('v406_purchase_button.custom_minimum_size.y=56', self.screen)
        self.assertIn('index_button.custom_minimum_size.y=42.0', self.screen)
        self.assertIn('v406_respec_button.disabled=v406_ranks.is_empty()', self.screen)

    def test_constellation_cursor_zoom_and_hover_inspection(self):
        web = (ROOT / "godot-v38/scripts/systems/moonfall_talent_web.gd").read_text(encoding="utf-8")
        self.assertIn("func zoom_at(factor: float, screen_point: Vector2)", web)
        self.assertIn('zoom_at(1.18, mouse.position)', web)
        self.assertIn('zoom_at(1.0 / 1.18, mouse.position)', web)
        self.assertIn('mouse_down_on_node', web)
        self.assertIn('hovered_id', web)
        self.assertIn('tooltip_text =', web)
        self.assertIn('elif id == hovered_id:', web)
        self.assertIn("func focus_node(talent_id: String)", web)
        self.assertIn("v406_web.focus_node(id)", self.screen)
        self.assertIn('v406_active_branch=str(node.get("branch",v406_active_branch))', self.screen)
        self.assertIn('v406_selected_node=id', self.screen)
        self.assertIn("_v406_index_selected.bind(node_id)", self.screen)

    def test_unlockable_navigation_and_prerequisite_highlights(self):
        web = (ROOT / "godot-v38/scripts/systems/moonfall_talent_web.gd").read_text(encoding="utf-8")
        for token in ('func next_available(', 'func available_talent_ids(',
                      'func _update_prerequisite_path()', 'prerequisite_path.has(id)',
                      'KEY_N', 'func select_node('):
            self.assertIn(token, web)
        self.assertIn('next_button.text="NEXT AVAILABLE (N)"', self.screen)
        self.assertIn('v406_web.next_available(v406_selected_node)', self.screen)
        self.assertIn('v406_web.select_node(v406_selected_node)', self.screen)
        self.assertNotIn('key.keycode == KEY_N or key.keycode == KEY_TAB', web)

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
